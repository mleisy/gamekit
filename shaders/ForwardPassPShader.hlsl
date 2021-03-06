/**********************************************************************

Copyright (c) 2015-2016 Robert May

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

struct Plane
{
	float4 Normal;
	float4 Orgin;
};


/************************************************************************************************/


cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 Proj;
	float4x4 CameraWT;			// World Transform
	float4x4 PV;				// Projection x View
	float4x4 CameraInverse;
	float4   CameraPOS;
	float  	 MinZ;
	float  	 MaxZ;
	int 	 PointLightCount;
	int 	 SpotLightCount;
};


/************************************************************************************************/


cbuffer LocalConstants : register( b1 )
{
	float4	 Albedo; // + roughness
	float4	 Specular;
	float4x4 WT;
};

struct PointLight
{
	float4 P;// Float4 ( Position + Radius)
	float4 K;// Float4 ( Colour + Intensity)
};

StructuredBuffer<PointLight> PointLights : register(t0);


/************************************************************************************************/


struct VIN
{
	float4 POS    : SV_POSITION;
	float4 WPOS   : POSITION0;
	float3 NORMAL : NORMAL;
};

float4 PMain( VIN In ) : SV_TARGET
{
	float4 ColourOut = float4(0, 0, 0, 1);


	float3 WPOS = IN.WPOS;
	float3 Lp = WPOS + float3(0, 30, -100);
	float3 Lc = Albedo.xyz;
	float3 Lv			= normalize(Lp - WPOS);
	//float3 Lv	= float3(0, 1, 0);
	float  La	= 1.0f;//100.0f / pow(length(Lp - WPOS), 2);
	float  m	= Specular.w;
	float  r	= Albedo.w;
	float3 Kd	= Albedo;
	float3 n	= IN.Normal;
	float3 Ks	= float3(1, 0, 1);
	float3 vdir = normalize(CameraPOS - WPOS);

	float3 Color = float3(0, 1, 1)  *  Frd(Lv, Lc, vdir, WPOS, Kd, n, Ks, 1.0f, r) * La * PIInverse;


	return ColourOut;
}


/************************************************************************************************/