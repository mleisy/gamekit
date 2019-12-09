/**********************************************************************

Copyright (c) 2014-2018 Robert May

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


#ifdef _WIN32
#pragma once
#endif


#ifndef WORLDRENDER_H_INCLUDED
#define WORLDRENDER_H_INCLUDED


#include "../buildsettings.h"
#include "../coreutilities/Components.h"
#include "../graphicsutilities/FrameGraph.h"
#include "../graphicsutilities/graphics.h"
#include "../graphicsutilities/CoreSceneObjects.h"


namespace FlexKit
{	/************************************************************************************************/


	class FLEXKITAPI ReadBack
	{
	public:
	};


	/************************************************************************************************/


	struct StreamingTextureDesc
	{
		static_vector<ResourceHandle> resourceHandles;
	};

	class FLEXKITAPI StreamingTexture
	{
	public:
		uint16_t GetHighestResidentMip()
		{
			return -1;
		}


		bool isMipLoadinProgress(uint16_t mip)
		{
			return false;
		}


		void QueueMipLoad(ThreadManager& threads)
		{
		}


	private:
		bool							evicted		= true;
		ID3D12Resource*					tiledResource;
		static_vector<char*>			buffers;	// Nulls for unmapped buffers
		uint16_t						highestMappedMip = -1;
		static_vector<ResourceHandle>	backingResources;
	};


	/************************************************************************************************/


	class FLEXKITAPI TiledResource
	{
	public:
		TiledResource(iAllocator* IN_allocator) {}

	};


	/************************************************************************************************/


	constexpr size_t GetMinBlockSize()
	{
		return ct_sqrt(64 * KILOBYTE / sizeof(uint8_t[4])); // assuming RGBA pixel, and a 64 KB texture alignment
	}

	struct TextureCacheDesc
	{
		const size_t textureCacheSize	= MEGABYTE * 16; // Very small for debugging purposes forces resource eviction and prioritisation, should be changed to a reasonable value
		const size_t blockSize			= GetMinBlockSize();
	};


	/************************************************************************************************/


	class TextureStreamingEngine;

	class TextureBlockAllocator
	{
	public:
		TextureBlockAllocator(iAllocator* IN_allocator) : 
			allocator{ IN_allocator } {}

		iAllocator* allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI TextureStreamingEngine
	{
	public:
		TextureStreamingEngine(RenderSystem& IN_renderSystem, iAllocator* IN_allocator, const TextureCacheDesc& desc = {}) : 
			allocator		{ IN_allocator		},
			mappedTextures	{ IN_allocator		},
			renderSystem	{ IN_renderSystem	},
			settings		{ desc				} 
		{
			FK_ASSERT(desc.textureCacheSize % desc.blockSize == 0, "INVALID POOL SIZE! MUST BE MULTIPLE OF BLOCK SIZE!");

			renderSystem.pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(desc.textureCacheSize),
				D3D12_RESOURCE_STATE_COMMON,
				nullptr, IID_PPV_ARGS(&resourcePool));
		}


		UpdateTask& update(UpdateDispatcher& dispatcher)
		{
			struct _update
			{

			};

			return dispatcher.Add<_update>(
				[&](auto& threadBuilder, _update& data)
				{
				},
				[this](auto&) 
				{
				}
			);
		}


		StreamingTexture2DHandle CreateStreamingTexture()
		{
			return InvalidHandle_t;
		}

	private:
		RenderSystem&				renderSystem;
		ID3D12Resource*				resourcePool	= nullptr;
		Vector<StreamingTexture>	mappedTextures;
		Vector<StreamingTexture*>	loadList; // updated every frame
		TextureBlockAllocator		allocator;
		const TextureCacheDesc		settings;
	};


	/************************************************************************************************/

	const PSOHandle FORWARDDRAW				= PSOHandle(GetTypeGUID(FORWARDDRAW));
	const PSOHandle GBUFFERPASS             = PSOHandle(GetTypeGUID(GBUFFERPASS));
	const PSOHandle LIGHTPREPASS			= PSOHandle(GetTypeGUID(LIGHTPREPASS));
	const PSOHandle DEPTHPREPASS            = PSOHandle(GetTypeGUID(DEPTHPREPASS));
	const PSOHandle FORWARDDRAWINSTANCED	= PSOHandle(GetTypeGUID(FORWARDDRAWINSTANCED));
	const PSOHandle FORWARDDRAW_OCCLUDE		= PSOHandle(GetTypeGUID(FORWARDDRAW_OCCLUDE));

	ID3D12PipelineState* CreateForwardDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateForwardDrawInstancedPSO	(RenderSystem* RS);
	ID3D12PipelineState* CreateOcclusionDrawPSO			(RenderSystem* RS);
	ID3D12PipelineState* CreateLightPassPSO				(RenderSystem* RS);
    ID3D12PipelineState* CreateDepthPrePassPSO          (RenderSystem* RS);
    ID3D12PipelineState* CreateGBufferPassPSO           (RenderSystem* RS);


	struct WorldRender_Targets
	{
		TextureHandle RenderTarget;
		TextureHandle DepthTarget;
	};


	struct ObjectDrawState
	{
		bool transparent	: 1;
		bool posed			: 1;
	};


	struct ObjectDraw
	{
		TriMeshHandle	mesh;
		TriMeshHandle	occluder;
		ObjectDrawState states;
		byte*			constantBuffers[4];
	};


	/************************************************************************************************/

	struct GPUPointLightLayout
	{
		float4 KI;	// Color + intensity in W
		float4 P;	//
	};

	struct RG16LightMap
	{
		uint16_t offset = 0;
		uint16_t count = 0;
	};


	struct LighBufferCPUUpdate
	{
		struct visableLightEntry
		{
			uint2				pixel;
			PointLightHandle	light;
			uint32_t			ID(uint2 wh) { return pixel[0] + pixel[1] * wh[0]; };
		};

		Vector<visableLightEntry> samples;

		uint2			viewSplits;
		float2			splitSpan;
		size_t			sceneLightCount;
		CameraHandle	camera;
		GraphicScene*	scene;

		TextureBuffer				lightMapBuffer;
		Vector<uint32_t>			lightLists;
		Vector<GPUPointLightLayout>	pointLights;

		StackAllocator	tempMemory;
	};

	struct GPUPointLight
	{
		float4 KI;
		float4 PositionR;
	};


	struct LightBufferUpdate 
	{
		Vector<GPUPointLight>		    pointLights;
		const Vector<PointLightHandle>*	pointLightHandles;
        UploadSegment			        lightBuffer;	// immediate update

		CameraHandle			camera;

		CBPushBuffer			constants;
		TextureHandle			lightMap;
		TextureHandle			lightListBuffer;

		FrameResourceHandle		lightMapObject;
		FrameResourceHandle		lightListObject;
		FrameResourceHandle		lightBufferObject;


		FlexKit::DescriptorHeap	descHeap;
	};


	struct LighBufferDebugDraw
	{
		VertexBufferHandle		vertexBuffer;
		ConstantBufferHandle	constantBuffer;
		TextureHandle			renderTarget;
	};


	struct SceneDescription
	{
		UpdateTaskTyped<PointLightGather>&	lights;
		UpdateTask&							transforms;
		UpdateTask&							cameras;
		UpdateTaskTyped<GetPVSTaskData>&	PVS;
	};

    struct DepthPass
    {
        DepthPass(const PVS& IN_drawables) :
            drawables{ IN_drawables } {}

        const PVS&          drawables;
        TextureHandle       depthPassTarget;
        FrameResourceHandle depthBufferObject;

        ConstantBufferDataSet passConstants;
        ConstantBufferDataSet cameraConstants;
        ConstantBufferDataSet entityConstants;

        CBPushBuffer passConstantsBuffer;
        CBPushBuffer entityConstantsBuffer;

        DescriptorHeap Heap; // Null Filled
    };

    using PointLightHandleList = Vector<PointLightHandle>;


    struct ForwardDrawConstants
    {
        float LightCount;
        float t;
    };

    struct ForwardPlusPass
    {
        ForwardPlusPass(
            const PointLightHandleList& IN_lights,
            const PVS&                  IN_PVS,
            const ConstantBufferDataSet& IN_passConstants,
            const ConstantBufferDataSet& IN_cameraConstants,
            const ConstantBufferDataSet& IN_entityConstants) :
                pointLights     { IN_lights },
                drawables       { IN_PVS    },
                cameraConstants { IN_cameraConstants },
                entityConstants { IN_entityConstants } {}


        FrameResourceHandle			BackBuffer;
        FrameResourceHandle			DepthBuffer;
        FrameResourceHandle			OcclusionBuffer;
        FrameResourceHandle			lightMap;
        FrameResourceHandle			lightLists;
        FrameResourceHandle			pointLightBuffer;
        VertexBufferHandle			VertexBuffer;

        CBPushBuffer passConstantsBuffer;
        CBPushBuffer entityConstantsBuffer;

        const ConstantBufferDataSet& cameraConstants;
        const ConstantBufferDataSet& entityConstants;

        const PointLightHandleList& pointLights;
        const PVS&                  drawables;

        DescriptorHeap				Heap; // Null Filled
    };


	/************************************************************************************************/


    class GBuffer
    {
    public:
        GBuffer(const uint2 WH, RenderSystem& RS_IN) :
            RS          { RS_IN },
            Albedo      { RS_IN.CreateUAVTextureResource(WH, FORMAT_2D::R8G8B8A8_UNORM, true)     },
            Specular    { RS_IN.CreateUAVTextureResource(WH, FORMAT_2D::R8G8B8A8_UNORM, true)     },
            Normal      { RS_IN.CreateUAVTextureResource(WH, FORMAT_2D::R16G16B16A16_FLOAT, true) },
            Tangent     { RS_IN.CreateUAVTextureResource(WH, FORMAT_2D::R16G16B16A16_FLOAT, true) },
            IOR_ANISO   { RS_IN.CreateUAVTextureResource(WH, FORMAT_2D::R16G16_FLOAT, true)       }
        {
            RS.SetDebugName(Albedo,     "Albedo");
            RS.SetDebugName(Specular,   "Specular");
            RS.SetDebugName(Normal,     "Normal");
            RS.SetDebugName(Tangent,    "Tangent");
            RS.SetDebugName(IOR_ANISO,  "IOR_ANISO");
        }

        ~GBuffer()
        {
            RS.ReleaseUAV(Albedo);
            RS.ReleaseUAV(Specular);
            RS.ReleaseUAV(Normal);
            RS.ReleaseUAV(Tangent);
            RS.ReleaseUAV(IOR_ANISO);
        }

        UAVTextureHandle Albedo;	// rgba_UNORM, Albedo + Metal
        UAVTextureHandle Specular;	// rgba_UNORM, Specular + roughness
        UAVTextureHandle Normal;	// float16_RGBA
        UAVTextureHandle Tangent;	// float16_RGBA
        UAVTextureHandle IOR_ANISO;	// float16_RG

        RenderSystem&    RS;
    };


    struct GBufferPass
    {
        GBuffer&        gbuffer;
        const PVS&      pvs;

        CBPushBuffer    entityConstants;
        CBPushBuffer    passConstants;

        DescriptorHeap	Heap;

        FrameResourceHandle AlbedoTargetObject;     // RGBA8
        FrameResourceHandle NormalTargetObject;     // RGBA16Float
        FrameResourceHandle SpecularTargetObject;
        FrameResourceHandle TangentTargetObject;
        FrameResourceHandle IOR_ANISOTargetObject;  // RGBA8

        FrameResourceHandle depthBufferTargetObject;
    };


    struct TiledDeferredShade
    {
        GBuffer& gbuffer;
    };


    /************************************************************************************************/


	class FLEXKITAPI WorldRender
	{
	public:
		WorldRender(iAllocator* Memory, RenderSystem& RS_IN, TextureStreamingEngine& IN_streamingEngine, const uint2 WH) :

			RS                  { RS_IN                                                             },
			ConstantBuffer		{ RS.CreateConstantBuffer(64 * MEGABYTE, false)						},
			OcclusionCulling	{ false																},
			lightLists			{ RS.CreateUAVBufferResource(sizeof(uint32_t) * 108 * 192 * 1024)   },
			pointLightBuffer	{ RS.CreateUAVBufferResource(sizeof(GPUPointLight) * 1024)          },
			lightMap			{ RS.CreateUAVTextureResource(WH / 10, FORMAT_2D::R32G32_UINT)      },
			streamingEngine		{ IN_streamingEngine											    },
			lightMapWH			{ WH / 10                                                           }
		{
			RS_IN.RegisterPSOLoader(FORWARDDRAW,			{ &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawPSO,			});
			RS_IN.RegisterPSOLoader(FORWARDDRAWINSTANCED,	{ &RS_IN.Library.RS6CBVs4SRVs,		CreateForwardDrawInstancedPSO	});
			RS_IN.RegisterPSOLoader(FORWARDDRAW_OCCLUDE,	{ &RS_IN.Library.RS6CBVs4SRVs,	    CreateOcclusionDrawPSO			});
			RS_IN.RegisterPSOLoader(LIGHTPREPASS,			{ &RS_IN.Library.ComputeSignature,  CreateLightPassPSO				});
			RS_IN.RegisterPSOLoader(DEPTHPREPASS,			{ &RS_IN.Library.RS6CBVs4SRVs,      CreateDepthPrePassPSO           });
			RS_IN.RegisterPSOLoader(GBUFFERPASS,			{ &RS_IN.Library.RS6CBVs4SRVs,      CreateGBufferPassPSO            });

            RS_IN.QueuePSOLoad(GBUFFERPASS);
            RS_IN.QueuePSOLoad(DEPTHPREPASS);
            RS_IN.QueuePSOLoad(LIGHTPREPASS);
            RS_IN.QueuePSOLoad(FORWARDDRAW);
            RS_IN.QueuePSOLoad(FORWARDDRAWINSTANCED);

            RS.SetDebugName(lightLists,        "lightLists");
            RS.SetDebugName(pointLightBuffer,  "pointLightBuffer");
            RS.SetDebugName(lightMap,          "lightMap");
		}


		~WorldRender()
		{
			RS.ReleaseCB(ConstantBuffer);
			RS.ReleaseUAV(lightMap);
			RS.ReleaseUAV(lightLists);
            RS.ReleaseUAV(pointLightBuffer);


		}


        DepthPass& DepthPrePass(
                UpdateDispatcher&   dispatcher,
                FrameGraph&         graph,
                const CameraHandle  camera,
                const PVS&          viewableObjects,
                const TextureHandle depthBufferTarget,
                iAllocator*         allocator);


        ForwardPlusPass& RenderPBR_ForwardPlus(
                UpdateDispatcher&           dispatcher,
                FrameGraph&                 graph,
                const DepthPass&            depthPass,
                const CameraHandle          camera,
                const WorldRender_Targets&  Target,
                const SceneDescription&     desc,
                const float                 t,
                iAllocator*                 allocator);


		LightBufferUpdate& UpdateLightBuffers(
                UpdateDispatcher&       dispatcher,
                FrameGraph&             graph,
                const CameraHandle      camera,
                const GraphicScene&     scene,
                const SceneDescription& desc,
                iAllocator*             tempMemory,
                LighBufferDebugDraw*    drawDebug = nullptr);

        GBufferPass&        RenderPBR_GBufferPass(
            UpdateDispatcher&   dispatcher,
            FrameGraph&         graph,
            const CameraHandle  camera,
            GatherTask&         pvs,
            GBuffer&            gbuffer,
            TextureHandle       depthTarget,
            iAllocator*         allocator);

        //TiledDeferredShade& RenderPBR_ShadeTiledDeferred();

	private:
		RenderSystem&			RS;
		ConstantBufferHandle	ConstantBuffer;
		//QueryHandle			OcclusionQueries;
		//TextureHandle			OcclusionBuffer;


		UAVTextureHandle		lightMap;			// GPU
        UAVResourceHandle		lightLists;			// GPU
		UAVResourceHandle		pointLightBuffer;	// GPU

		uint2					WH;					// Output Size
		uint2					lightMapWH;			// Output Size

		TextureStreamingEngine&	streamingEngine;
		bool                    OcclusionCulling;

		// GBuffer
		//RenderTargetHandle		Albedo;
		//RenderTargetHandle		Color;
		//RenderTargetHandle		Normal;
		//RenderTargetHandle		Depth;
	};


}	/************************************************************************************************/

#endif
