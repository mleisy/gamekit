/**********************************************************************

Copyright (c) 2014-2019 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/


#include "WorldRender.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <d3d11shader.h>

namespace FlexKit
{	/************************************************************************************************/


    ID3D12PipelineState* CreateLightPassPSO(RenderSystem* RS)
    {
        auto lightPassShader = LoadShader("tiledLightCulling", "tiledLightCulling", "cs_5_0", "assets\\lightPass.hlsl");

        D3D12_COMPUTE_PIPELINE_STATE_DESC PSO_desc = {};
        PSO_desc.CS				= lightPassShader;
        PSO_desc.pRootSignature = RS->Library.ComputeSignature;

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateComputePipelineState(&PSO_desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    ID3D12PipelineState* CreateForwardDrawPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\forwardRender.hlsl");
        auto DrawRectPShader = LoadShader("Forward_PS", "Forward_PS", "ps_5_0",	"assets\\forwardRender.hlsl");

        FINALLY
            
        Release(&DrawRectVShader);
        Release(&DrawRectPShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.PS                    = DrawRectPShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateDepthPrePassPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("DepthPass_VS", "DepthPass_VS", "vs_5_0",	"assets\\forwardRender.hlsl");

        EXITSCOPE(Release(&DrawRectVShader));
            

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateGBufferPassPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS",           "vs_5_0",	"assets\\forwardRender.hlsl");
        auto DrawRectPShader = LoadShader("GBufferFill_PS", "GBufferFill_PS",   "ps_5_0",	"assets\\forwardRender.hlsl");

        FINALLY
         Release(&DrawRectVShader);
         Release(&DrawRectPShader);
        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.PS                    = DrawRectPShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 5;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Albedo
            PSO_Desc.RTVFormats[1]         = DXGI_FORMAT_R8G8B8A8_UNORM; // Specular
            PSO_Desc.RTVFormats[2]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normal
            PSO_Desc.RTVFormats[3]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // Tangent
            PSO_Desc.RTVFormats[4]         = DXGI_FORMAT_R16G16_FLOAT; // IOR_ANISO
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateDeferredShadingPassPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("ShadingPass_VS",     "DeferredShade_VS",   "vs_5_0",	"assets\\deferredRender.hlsl");
        auto PShader = LoadShader("DeferredShade_PS",   "DeferredShade_VS",   "ps_5_0",	"assets\\deferredRender.hlsl");

        FINALLY
         Release(&VShader);
         Release(&PShader);
        FINALLYOVER


        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateEnvironmentPassPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("passthrough_VS", "FORWARDShade_VS", "vs_5_0", "assets\\DeferredRender.hlsl");
        auto PShader = LoadShader("environment_PS", "FORWARDShade_VS", "ps_5_0", "assets\\DeferredRender.hlsl");

        FINALLY
            Release(&VShader);
            Release(&PShader);
        FINALLYOVER


        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RSDefault;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT; // backBuffer
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements) / sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;

            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = true;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp     = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;

            PSO_Desc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND::D3D12_BLEND_ONE;

            PSO_Desc.BlendState.RenderTarget[0].SrcBlendAlpha   = D3D12_BLEND::D3D12_BLEND_ONE;
            PSO_Desc.BlendState.RenderTarget[0].DestBlendAlpha  = D3D12_BLEND::D3D12_BLEND_ONE;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateForwardDrawInstancedPSO(RenderSystem* RS)
    {
        auto VShader = LoadShader("VMain",		"InstanceForward_VS",	"vs_5_0", "assets\\DrawInstancedVShader.hlsl");
        auto PShader = LoadShader("FlatWhite",	"FlatWhite",			"ps_5_0", "assets\\forwardRender.hlsl");

        EXITSCOPE(
            Release(&VShader);
            Release(&PShader);
        );

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
                { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",		0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",    0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,	 3, 0,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

                { "INSTANCEWT",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 0,   D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 16,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 32,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
                { "INSTANCEWT",	3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,	4, 48,  D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = VShader;
            PSO_Desc.PS                    = PShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 1;
            PSO_Desc.RTVFormats[0]         = DXGI_FORMAT_R16G16B16A16_FLOAT;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
            PSO_Desc.BlendState.RenderTarget[0].BlendEnable = false;
            PSO_Desc.BlendState.RenderTarget[0].DestBlend	= D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
            PSO_Desc.BlendState.RenderTarget[0].SrcBlend	= D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
            PSO_Desc.BlendState.RenderTarget[0].BlendOp		= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    ID3D12PipelineState* CreateOcclusionDrawPSO(RenderSystem* RS)
    {
        auto DrawRectVShader = LoadShader("Forward_VS", "Forward_VS", "vs_5_0",	"assets\\forwardRender.hlsl");

        FINALLY
            
        Release(&DrawRectVShader);

        FINALLYOVER

        /*
        typedef struct D3D12_INPUT_ELEMENT_DESC
        {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
        } 	D3D12_INPUT_ELEMENT_DESC;
        */

        D3D12_INPUT_ELEMENT_DESC InputElements[] = {
            { "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };


        D3D12_RASTERIZER_DESC		Rast_Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        D3D12_DEPTH_STENCIL_DESC	Depth_Desc	= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
        Depth_Desc.DepthEnable	= true;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC	PSO_Desc = {}; {
            PSO_Desc.pRootSignature        = RS->Library.RS6CBVs4SRVs;
            PSO_Desc.VS                    = DrawRectVShader;
            PSO_Desc.RasterizerState       = Rast_Desc;
            PSO_Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            PSO_Desc.SampleMask            = UINT_MAX;
            PSO_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            PSO_Desc.NumRenderTargets      = 0;
            PSO_Desc.SampleDesc.Count      = 1;
            PSO_Desc.SampleDesc.Quality    = 0;
            PSO_Desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
            PSO_Desc.InputLayout           = { InputElements, sizeof(InputElements)/sizeof(*InputElements) };
            PSO_Desc.DepthStencilState     = Depth_Desc;
        }

        ID3D12PipelineState* PSO = nullptr;
        auto HR = RS->pDevice->CreateGraphicsPipelineState(&PSO_Desc, IID_PPV_ARGS(&PSO));
        FK_ASSERT(SUCCEEDED(HR));

        return PSO;
    }


    /************************************************************************************************/


    DepthPass& WorldRender::DepthPrePass(
        UpdateDispatcher&   dispatcher,
        FrameGraph&         frameGraph,
        const CameraHandle  camera,
        GatherTask&         pvs,
        const ResourceHandle depthBufferTarget,
        iAllocator*         allocator)
    {
        const size_t MaxEntityDrawCount = 10000;

        auto& pass = frameGraph.AddNode<DepthPass>(
                pvs.GetData().solid,
                [&, camera](FrameGraphNodeBuilder& builder, DepthPass& data)
                {
                    const size_t localBufferSize = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                    
                    data.entityConstantsBuffer  = std::move(Reserve(ConstantBuffer, sizeof(ForwardDrawConstants), MaxEntityDrawCount, frameGraph.Resources));
                    data.passConstantsBuffer    = std::move(Reserve(ConstantBuffer, localBufferSize, 2, frameGraph.Resources));
                    data.depthBufferObject      = builder.WriteDepthBuffer(depthBufferTarget);
                    data.depthPassTarget        = depthBufferTarget;

                    data.Heap.Init(
                        frameGraph.Resources.renderSystem,
                        frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                        allocator);

                    data.Heap.NullFill(frameGraph.Resources.renderSystem);

                    builder.AddDataDependency(pvs);
                },
                [=](DepthPass& data, const FrameResources& resources, Context* ctx)
                {
                    const auto cameraConstants = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };

                    ctx->SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
                    ctx->SetPipelineState(resources.GetPipelineState(DEPTHPREPASS));

                    ctx->SetScissorAndViewports({ data.depthPassTarget });
                    ctx->SetRenderTargets(
                        {},
                        true,
                        resources.GetRenderTargetObject(data.depthBufferObject));

                    ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
                    ctx->SetGraphicsDescriptorTable(0, data.Heap);
                    ctx->SetGraphicsConstantBufferView(1, cameraConstants);
                    ctx->SetGraphicsConstantBufferView(3, cameraConstants);
                    ctx->NullGraphicsConstantBufferView(6);

                    TriMesh* prevMesh = nullptr;
                    for (const auto& drawable : data.drawables)
                    {
                        auto* const triMesh = GetMeshResource(drawable.D->MeshHandle);
                        if (triMesh != prevMesh)
                        {
                            prevMesh = triMesh;

                            ctx->AddIndexBuffer(triMesh);
                            ctx->AddVertexBuffers(triMesh,
                                {   VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                });
                        }

                        ctx->SetGraphicsConstantBufferView(2, ConstantBufferDataSet{ drawable.D->GetConstants(), data.entityConstantsBuffer });
                        ctx->DrawIndexedInstanced(triMesh->IndexCount);
                    }
                });

        return pass;
    }


    /************************************************************************************************/


    BackgroundEnvironmentPass& WorldRender::BackgroundPass(
        UpdateDispatcher&       dispatcher,
        FrameGraph&             frameGraph,
        const CameraHandle      camera,
        const ResourceHandle     renderTarget,
        const ResourceHandle     hdrMap,
        VertexBufferHandle      vertexBuffer,
        iAllocator*             allocator)
    {
        auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
            BackgroundEnvironmentPass{},
            [&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
            {
                const size_t localBufferSize        = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                auto& renderSystem                  = frameGraph.GetRenderSystem();
                data.renderTargetObject             = builder.WriteRenderTarget(renderTarget);


                data.passConstants  = CBPushBuffer(ConstantBuffer, 6 * KILOBYTE, renderSystem);
                data.passVertices   = VBPushBuffer(vertexBuffer, sizeof(float4) * 6, renderSystem);

                data.descHeap.Init2(renderSystem, renderSystem.Library.RSDefault.GetDescHeap(0), 20, allocator);
                data.descHeap.NullFill(renderSystem, 20);
                data.HDRMap     = hdrMap;
            },
            [=](BackgroundEnvironmentPass& data, const FrameResources& frameResources, Context* ctx)
            {
                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(-1,  -1,  1, 1),

                    float4(-1,  -1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(1,   -1,  1, 1),
                };

                RenderTargetList renderTargets = {
                    frameResources.GetRenderTargetObjects({ data.renderTargetObject })
                };

                struct
                {
                    float2 WH;
                }passConstants = { float2(WH[0], WH[1]) };

                data.descHeap.SetSRV(renderSystem, 6, data.HDRMap);

                ctx->SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx->SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
                ctx->SetGraphicsDescriptorTable(4, data.descHeap);

                ctx->SetScissorAndViewports({ renderTarget });
                ctx->SetRenderTargets(renderTargets, false, {});
                ctx->SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx->SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx->SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });

                ctx->Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    BackgroundEnvironmentPass& WorldRender::RenderPBR_IBL_Deferred(
        UpdateDispatcher&       dispatcher,
        FrameGraph&             frameGraph,
        const CameraHandle      camera,
        const ResourceHandle     renderTarget,
        const ResourceHandle     depthTarget,
        const ResourceHandle     hdrMap,
        GBuffer&                gbuffer,
        VertexBufferHandle      vertexBuffer,
        iAllocator*             allocator)
    {
        auto& pass = frameGraph.AddNode<BackgroundEnvironmentPass>(
            BackgroundEnvironmentPass{},
            [&](FrameGraphNodeBuilder& builder, BackgroundEnvironmentPass& data)
            {
                const size_t localBufferSize    = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));
                auto& renderSystem              = frameGraph.GetRenderSystem();
                data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);

                data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.Albedo);
                data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.Normal);
                data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.Tangent);
                data.SpecularTargetObject       = builder.ReadShaderResource(gbuffer.Specular);
                data.IOR_ANISOTargetObject      = builder.ReadShaderResource(gbuffer.IOR_ANISO);
                data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);

                data.passConstants  = CBPushBuffer(ConstantBuffer, 6 * KILOBYTE, renderSystem);
                data.passVertices   = VBPushBuffer(vertexBuffer, sizeof(float4) * 6, renderSystem);

                data.descHeap.Init2(renderSystem, renderSystem.Library.RSDefault.GetDescHeap(0), 20, allocator);
                data.descHeap.NullFill(renderSystem, 20);
                data.HDRMap     = hdrMap;
            },
            [=](BackgroundEnvironmentPass& data, const FrameResources& frameResources, Context* ctx)
            {
                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(-1,  -1,  1, 1),

                    float4(-1,  -1,  1, 1),
                    float4(1,    1,  1, 1),
                    float4(1,   -1,  1, 1),
                };

                RenderTargetList renderTargets = {
                    frameResources.GetRenderTargetObjects({ data.renderTargetObject })
                };

                struct
                {
                    float2 WH;
                }passConstants = { float2(WH[0], WH[1]) };

                data.descHeap.SetSRV(renderSystem, 0, frameResources.GetTexture(data.AlbedoTargetObject));
                data.descHeap.SetSRV(renderSystem, 1, frameResources.GetTexture(data.SpecularTargetObject));
                data.descHeap.SetSRV(renderSystem, 2, frameResources.GetTexture(data.NormalTargetObject));
                data.descHeap.SetSRV(renderSystem, 3, frameResources.GetTexture(data.TangentTargetObject));
                data.descHeap.SetSRV(renderSystem, 4, frameResources.GetTexture(data.IOR_ANISOTargetObject));
                data.descHeap.SetSRV(renderSystem, 5, frameResources.GetTexture(data.depthBufferTargetObject), FORMAT_2D::R32_FLOAT);
                data.descHeap.SetSRV(renderSystem, 6, data.HDRMap);

                ctx->SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx->SetPipelineState(frameResources.GetPipelineState(ENVIRONMENTPASS));
                ctx->SetGraphicsDescriptorTable(4, data.descHeap);

                ctx->SetScissorAndViewports({ renderTarget });
                ctx->SetRenderTargets(renderTargets, false, {});
                ctx->SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx->SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx->SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });

                ctx->Draw(6);
            });

        return pass;
    }


    /************************************************************************************************/


    ForwardPlusPass& WorldRender::RenderPBR_ForwardPlus(
        UpdateDispatcher&			dispatcher,
        FrameGraph&					frameGraph, 
        const DepthPass&			depthPass,
        const CameraHandle			camera,
        const WorldRender_Targets&	Targets,
        const SceneDescription&	    desc,
        const float                 t,
        ResourceHandle              environmentMap,
        iAllocator*					allocator)
    {
        const size_t MaxEntityDrawCount = 10000;

        typedef Vector<ForwardPlusPass> ForwardDrawableList;
        

        auto& pass = frameGraph.AddNode<ForwardPlusPass>(
            ForwardPlusPass{
                desc.lights.GetData().pointLights,
                depthPass.drawables,
                depthPass.entityConstantsBuffer },
            [&](FrameGraphNodeBuilder& builder, ForwardPlusPass& data)
            {
                builder.AddDataDependency(desc.PVS);
                builder.AddDataDependency(desc.cameras);

                data.BackBuffer			= builder.WriteRenderTarget	(Targets.RenderTarget);
                data.DepthBuffer        = builder.WriteDepthBuffer (Targets.DepthTarget);
                size_t localBufferSize  = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));

                data.passConstantsBuffer    = std::move(Reserve(ConstantBuffer, localBufferSize, 2, frameGraph.Resources));

                data.lightMap			= builder.ReadWriteUAV(lightMap,			DRS_ShaderResource);
                data.lightLists			= builder.ReadWriteUAV(lightLists,			DRS_ShaderResource);
                data.pointLightBuffer	= builder.ReadWriteUAV(pointLightBuffer,	DRS_ShaderResource);

                data.Heap.Init(
                    frameGraph.Resources.renderSystem,
                    frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    allocator);

                data.Heap.NullFill(frameGraph.Resources.renderSystem);
            },
            [=](ForwardPlusPass& data, const FrameResources& resources, Context* ctx)
            {
                const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstantsBuffer };
                const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ (float)data.pointLights.size(), t }, data.passConstantsBuffer };

                ctx->SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
                ctx->SetPipelineState(resources.GetPipelineState(FORWARDDRAW));

                // Setup Initial Shading State
                ctx->SetScissorAndViewports({Targets.RenderTarget});
                ctx->SetRenderTargets(
                    { resources.GetRenderTargetObject(data.BackBuffer) },
                    true,
                    resources.GetRenderTargetObject(data.DepthBuffer));


                data.Heap.SetSRV(resources.renderSystem, 0, lightMap);
                data.Heap.SetSRV(resources.renderSystem, 1, lightLists);
                data.Heap.SetSRV(resources.renderSystem, 2, pointLightBuffer);
                data.Heap.SetSRV(resources.renderSystem, 3, environmentMap);

                ctx->SetPrimitiveTopology			(EInputTopology::EIT_TRIANGLE);
                ctx->SetGraphicsDescriptorTable		(0, data.Heap);
                ctx->SetGraphicsConstantBufferView	(1, cameraConstants);
                ctx->SetGraphicsConstantBufferView	(3, passConstants);
                ctx->NullGraphicsConstantBufferView	(6);

                TriMesh* prevMesh = nullptr;

                for (size_t itr = 0; itr < data.drawables.size(); ++itr)
                {
                    auto& drawable = data.drawables[itr];
                    TriMesh* triMesh = GetMeshResource(drawable.D->MeshHandle);

                    if (triMesh != prevMesh)
                    {
                        prevMesh = triMesh;

                        ctx->AddIndexBuffer(triMesh);
                        ctx->AddVertexBuffers(triMesh,
                            {	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                                VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                            });
                    }

                    auto proxy = CreateCBIterator<decltype(drawable.D->GetConstants())>(data.entityConstants)[itr];

                    ctx->SetGraphicsConstantBufferView(
                        2,
                        proxy);
                    ctx->DrawIndexedInstanced(triMesh->IndexCount, 0, 0, 1, 0);
                }
            });

        return pass;
    }


    /************************************************************************************************/


    LightBufferUpdate& WorldRender::UpdateLightBuffers(
        UpdateDispatcher&		dispatcher,
        FrameGraph&				graph,
        const CameraHandle	    camera,
        const GraphicScene&	    scene,
        const SceneDescription& sceneDescription,
        iAllocator*				tempMemory, 
        LighBufferDebugDraw*	drawDebug)
    {
        graph.Resources.AddUAVResource(lightMap,			0, graph.GetRenderSystem().GetObjectState(lightMap));
        graph.Resources.AddUAVResource(lightLists,			0, graph.GetRenderSystem().GetObjectState(lightLists));
        graph.Resources.AddUAVResource(pointLightBuffer,	0, graph.GetRenderSystem().GetObjectState(pointLightBuffer));

        auto& lightBufferData = graph.AddNode<LightBufferUpdate>(
            LightBufferUpdate{
                    Vector<GPUPointLight>(tempMemory, 1024),
                    &sceneDescription.lights.GetData().pointLights,
                    ReserveUploadBuffer(1024 * sizeof(GPUPointLight), graph.GetRenderSystem()),// max point light count of 1024
                    camera,
                    CBPushBuffer(ConstantBuffer, 2 * KILOBYTE, graph.GetRenderSystem()),
            },
            [&, this](FrameGraphNodeBuilder& builder, LightBufferUpdate& data)
            {
                auto& renderSystem      = graph.GetRenderSystem();
                data.lightMapObject		= builder.ReadWriteUAV(lightMap,		 DRS_UAV);
                data.lightListObject	= builder.ReadWriteUAV(lightLists,		 DRS_UAV);
                data.lightBufferObject	= builder.ReadWriteUAV(pointLightBuffer, DRS_Write);
                data.camera				= camera;

                data.descHeap.Init(renderSystem, renderSystem.Library.ComputeSignature.GetDescHeap(0), tempMemory);
                data.descHeap.NullFill(renderSystem);

                builder.AddDataDependency(sceneDescription.lights);
                builder.AddDataDependency(sceneDescription.cameras);
            },
            [XY = lightMapWH](LightBufferUpdate& data, FrameResources& resources, Context* ctx)
            {
                const auto cameraConstants = GetCameraConstants(data.camera);

                struct ConstantsLayout
                {
                    float4x4 iproj;
                    float4x4 view;
                    uint2    LightMapWidthHeight[2];
                    uint32_t lightCount;
                }constantsValues = {
                    XMMatrixToFloat4x4(DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(cameraConstants.Proj))),
                    cameraConstants.View,
                    XY,
                    (uint32_t)data.pointLightHandles->size()
                };

                ConstantBufferDataSet constants{ constantsValues, data.constants };
                PointLightComponent& pointLights = PointLightComponent::GetComponent();

                for (const auto light : *data.pointLightHandles)
                {
                    PointLight pointLight	= pointLights[light];
                    float3 position			= GetPositionW(pointLight.Position);

                    data.pointLights.push_back(
                        {	{ pointLight.K, 10	},
                            { position, 20		} });
                }

                const size_t uploadSize = data.pointLights.size() * sizeof(GPUPointLight);
                MoveBuffer2UploadBuffer(data.lightBuffer, (byte*)data.pointLights.begin(), uploadSize);
                ctx->CopyBuffer(data.lightBuffer, uploadSize, resources.WriteUAV(data.lightBufferObject, ctx));

                ctx->SetPipelineState(resources.GetPipelineState(LIGHTPREPASS));
                ctx->SetComputeRootSignature(resources.renderSystem.Library.ComputeSignature);
                ctx->SetComputeDescriptorTable(0, data.descHeap);

                data.descHeap.SetUAV(resources.renderSystem, 0, resources.GetUAVTextureResource	(data.lightMapObject));
                data.descHeap.SetUAV(resources.renderSystem, 1, resources.GetUAVBufferResource	(data.lightListObject));
                data.descHeap.SetSRV(resources.renderSystem, 2, resources.ReadUAVBuffer			(data.lightBufferObject, DRS_ShaderResource, ctx));
                data.descHeap.SetCBV(resources.renderSystem, 6, constants.Handle(), constants.Offset(), sizeof(ConstantsLayout));

                ctx->Dispatch({ XY[0], XY[1], 1 });
            });

        return lightBufferData;
    }


    /************************************************************************************************/


    GBufferPass& WorldRender::RenderPBR_GBufferPass(
        UpdateDispatcher&   dispatcher,
        FrameGraph&         frameGraph,
        const CameraHandle  camera,
        GatherTask&         pvs,
        GBuffer&            gbuffer,
        ResourceHandle       depthTarget,
        iAllocator*         allocator)
    {
        constexpr size_t MaxEntityDrawCount = 10000;
        constexpr size_t localBufferSize    = std::max(sizeof(Camera::ConstantBuffer), sizeof(ForwardDrawConstants));


        auto& pass = frameGraph.AddNode<GBufferPass>(
            GBufferPass{
                gbuffer, pvs.GetData().solid,
                Reserve(ConstantBuffer, sizeof(ForwardDrawConstants), MaxEntityDrawCount, frameGraph.Resources),
                Reserve(ConstantBuffer, localBufferSize, MaxEntityDrawCount, frameGraph.Resources) },
            [&](FrameGraphNodeBuilder& builder, GBufferPass& data)
            {
                builder.AddDataDependency(pvs);

                data.AlbedoTargetObject      = builder.WriteRenderTarget(gbuffer.Albedo);
                data.IOR_ANISOTargetObject   = builder.WriteRenderTarget(gbuffer.IOR_ANISO);
                data.NormalTargetObject      = builder.WriteRenderTarget(gbuffer.Normal);
                data.SpecularTargetObject    = builder.WriteRenderTarget(gbuffer.Specular);
                data.TangentTargetObject     = builder.WriteRenderTarget(gbuffer.Tangent);
                data.depthBufferTargetObject = builder.WriteDepthBuffer(depthTarget);

                data.Heap.Init(
                    frameGraph.Resources.renderSystem,
                    frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    allocator);

                data.Heap.NullFill(frameGraph.Resources.renderSystem);
            },
            [camera](GBufferPass& data, FrameResources& frameResources, Context* ctx)
            {
                const auto cameraConstants  = ConstantBufferDataSet{ GetCameraConstants(camera), data.passConstants };
                const auto passConstants    = ConstantBufferDataSet{ ForwardDrawConstants{ 1, 1 }, data.passConstants };

                ctx->SetRootSignature(frameResources.renderSystem.Library.RS6CBVs4SRVs);
                ctx->SetPipelineState(frameResources.GetPipelineState(GBUFFERPASS));
                ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);

                // Setup pipeline resources
                ctx->SetScissorAndViewports(
                    std::tuple{
                        data.gbuffer.Albedo,
                        data.gbuffer.IOR_ANISO,
                        data.gbuffer.Normal,
                        data.gbuffer.Specular,
                        data.gbuffer.Tangent
                    });

                RenderTargetList renderTargets = {
                    frameResources.GetRenderTargetObject(data.AlbedoTargetObject),
                    frameResources.GetRenderTargetObject(data.SpecularTargetObject),
                    frameResources.GetRenderTargetObject(data.NormalTargetObject),
                    frameResources.GetRenderTargetObject(data.TangentTargetObject),
                    frameResources.GetRenderTargetObject(data.IOR_ANISOTargetObject),
                };

                ctx->SetGraphicsDescriptorTable(0, data.Heap);

                ctx->SetRenderTargets(
                    renderTargets,
                    true,
                    frameResources.GetRenderTargetObject(data.depthBufferTargetObject));

                // Setup Constants
                ctx->SetGraphicsConstantBufferView(1, cameraConstants);
                ctx->SetGraphicsConstantBufferView(3, passConstants);

                // submit draw calls
                for (auto& drawable : data.pvs)
                {
                    const auto constants  = drawable.D->GetConstants();
                    const auto triMesh    = GetMeshResource(drawable.D->MeshHandle);

                    ctx->AddIndexBuffer(triMesh);
                    ctx->AddVertexBuffers(
                        triMesh,
                        {
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT,
                            VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,
                        }
                    );

                    ctx->SetGraphicsConstantBufferView(2, ConstantBufferDataSet(constants, data.entityConstants));
                    ctx->DrawIndexed(triMesh->IndexCount);
                }
            }
            );

        return pass;
    }



    /************************************************************************************************/


    TiledDeferredShade& WorldRender::RenderPBR_DeferredShade(
        UpdateDispatcher&       dispatcher,
        FrameGraph&             frameGraph,
        const CameraHandle      camera,
        PointLightGatherTask&   gather,
        GBuffer&                gbuffer,
        ResourceHandle           depthTarget,
        ResourceHandle           renderTarget,
        ResourceHandle           environmentMap,
        VertexBufferHandle      vertexBuffer,
        float                   t,
        iAllocator*             allocator)
    {
        struct PointLight
        {
            float4 KI;	// Color + intensity in W
            float4 PR;	// XYZ + radius in W
        };

        auto& pass = frameGraph.AddNode<TiledDeferredShade>(
            TiledDeferredShade{
                gbuffer,
                gather
            },
            [&](FrameGraphNodeBuilder& builder, TiledDeferredShade& data)
            {
                auto& renderSystem = frameGraph.GetRenderSystem();

                data.AlbedoTargetObject         = builder.ReadShaderResource(gbuffer.Albedo);
                data.NormalTargetObject         = builder.ReadShaderResource(gbuffer.Normal);
                data.TangentTargetObject        = builder.ReadShaderResource(gbuffer.Tangent);
                data.SpecularTargetObject       = builder.ReadShaderResource(gbuffer.Specular);
                data.IOR_ANISOTargetObject      = builder.ReadShaderResource(gbuffer.IOR_ANISO);
                data.depthBufferTargetObject    = builder.ReadShaderResource(depthTarget);
                data.renderTargetObject         = builder.WriteRenderTarget(renderTarget);

                data.descHeap.Init2(renderSystem, renderSystem.Library.RSDefault.GetDescHeap(0), 20, allocator);
                data.descHeap.NullFill(renderSystem, 20);

                data.passConstants = CBPushBuffer(ConstantBuffer, 6 * KILOBYTE, renderSystem);
                data.passVertices  = VBPushBuffer(vertexBuffer, sizeof(float4) * 6, renderSystem);
            },
            [camera, renderTarget, environmentMap, t](TiledDeferredShade& data, FrameResources& frameResources, Context* ctx)
            {
                auto& renderSystem          = frameResources.renderSystem;
                const auto WH               = frameResources.renderSystem.GetTextureWH(renderTarget);
                const auto cameraConstants  = GetCameraConstants(camera);

                RenderTargetList renderTargets = {
                    frameResources.GetRenderTargetObjects({ data.renderTargetObject })
                };

                struct
                {
                    float2  WH;
                    float   Seed;
                }passConstants = { float2(WH[0], WH[1]), t };

                struct
                {
                    float4 Position;
                }   vertices[] =
                {
                    float4(-1,   1, 0, 1),
                    float4(1,   1, 0, 1),
                    float4(-1,  -1, 0, 1),

                    float4(-1,  -1, 0, 1),
                    float4(1,   1, 0, 1),
                    float4(1,  -1, 0, 1),
                };

                ctx->SetRootSignature(frameResources.renderSystem.Library.RSDefault);
                ctx->SetPipelineState(frameResources.GetPipelineState(SHADINGPASS));
                ctx->SetGraphicsDescriptorTable(4, data.descHeap);

                ctx->SetScissorAndViewports({ renderTarget });
                ctx->SetRenderTargets(renderTargets, false, {});
                ctx->SetVertexBuffers({ VertexBufferDataSet{ vertices, data.passVertices } });
                ctx->SetGraphicsConstantBufferView(0, ConstantBufferDataSet{ cameraConstants, data.passConstants });
                ctx->SetGraphicsConstantBufferView(1, ConstantBufferDataSet{ passConstants, data.passConstants });

                data.descHeap.SetSRV(renderSystem, 0, frameResources.GetTexture(data.AlbedoTargetObject));
                data.descHeap.SetSRV(renderSystem, 1, frameResources.GetTexture(data.SpecularTargetObject));
                data.descHeap.SetSRV(renderSystem, 2, frameResources.GetTexture(data.NormalTargetObject));
                data.descHeap.SetSRV(renderSystem, 3, frameResources.GetTexture(data.TangentTargetObject));
                data.descHeap.SetSRV(renderSystem, 4, frameResources.GetTexture(data.IOR_ANISOTargetObject));
                data.descHeap.SetSRV(renderSystem, 5, frameResources.GetTexture(data.depthBufferTargetObject), FORMAT_2D::R32_FLOAT);
                data.descHeap.SetSRV(renderSystem, 6, environmentMap);

                auto& lights        = data.lights.GetData().pointLights;
                auto& pointLights   = PointLightComponent::GetComponent();
                auto& transforms    = SceneNodeComponent::GetComponent();

                for (auto& light : lights)
                {
                    PointLight lightConstants;

                    const auto lightData = pointLights[light];

                    lightConstants.KI = float4(lightData.K, lightData.I);
                    lightConstants.PR = float4(GetPositionW(lightData.Position), lightData.R);

                    ctx->SetGraphicsConstantBufferView(2, ConstantBufferDataSet{ lightConstants, data.passConstants });
                    ctx->Draw(6);
                }
            });

        return pass;
    }


}	/************************************************************************************************/
