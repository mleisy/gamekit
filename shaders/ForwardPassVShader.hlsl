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
}

cbuffer extraConstants : register(b10)
{
	float4x4 Stuff;
}

/************************************************************************************************/


struct VIN
{
	float3 POS : POSITION;
	float2 T : TEXCOORD0;
	float3 N : NORMAL;
};


/************************************************************************************************/


struct VOUT
{
	float4 POS 		: SV_POSITION;
	float4 WPOS		: POSITION0;
	float3 NORMAL	: NORMAL;
};


/************************************************************************************************/


VOUT VMain( VIN In )
{
	VOUT Out;
	Out.WPOS 	= mul(WT, float4(In.POS, 1));
	Out.POS 	= mul(PV, mul(WT, float4(In.POS, 1))); 
	Out.NORMAL  = mul(WT, float4(In.N, 0));

	return Out;
}


/************************************************************************************************/