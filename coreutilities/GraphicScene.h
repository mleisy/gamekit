/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef GraphicScene_H
#define GraphicScene_H

#include "..\buildsettings.h"
#include "..\coreutilities\Resources.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\AnimationUtilities.h" 
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{
	typedef size_t SpotLightHandle;
	typedef Pair<bool, int64_t> GSPlayAnimation_RES;

	struct SceneNodeComponentSystem;

	template<typename TY_SCHEME>
	struct iScenePartitioner
	{
		typedef typename TY_SCHEME::TY_ENTRY TY_NODE;

		void Initiate(iAllocator* memory)
		{
			Scene.Initiate(memory);
		}

		void Release()
		{
			Scene.Release();
		}


		void TranslateEntity(float3 XYZ, float2 UV)
		{

		}

		Vector<EntityHandle>	GetViewables(Camera* C)
		{
			Vector<EntityHandle> Out;
			return Out;
		}

		void Update()
		{

		}

		TY_SCHEME Scene;
	};
	

	struct QuadTreeNode
	{
		float	r;
		static_vector<EntityHandle, 64>	Contents;
		static_vector<QuadTreeNode*,4>	ChildNodes;
	};

	typedef static_vector<QuadTreeNode*, 16> NodeBlock;

	void FreeNode(NodeBlock& FreeList, iAllocator* Memory, QuadTreeNode* Node);

	struct QuadTree
	{
		typedef QuadTreeNode TY_ENTRY;

		void Initiate(iAllocator* memory);
		void Release();

		TY_ENTRY*						Root;
		iAllocator*						Memory;
		static_vector<TY_ENTRY*, 16>	FreeList;
	};

	typedef iScenePartitioner<QuadTree>	ScenePartitionTable;

	void UpdateQuadTree		(QuadTreeNode* Node, GraphicScene* Scene);
	void InitiateSceneMgr	(ScenePartitionTable* SPT, iAllocator* Memory);

	struct FLEXKITAPI GraphicScene
	{
		EntityHandle CreateDrawable();
		void		 RemoveEntity(EntityHandle E);
		void		 ClearScene();

		Drawable&	GetDrawable			(EntityHandle EHandle) {return Drawables.at(EHandle);								}
		Skeleton*	GetEntitySkeleton	(EntityHandle EHandle) {return GetSkeleton(GT, Drawables.at(EHandle).MeshHandle);	}
		float4		GetMaterialColour	(EntityHandle EHandle) {return Drawables.at(EHandle).MatProperties.Albedo;			}
		float4		GetMaterialSpec		(EntityHandle EHandle) {return Drawables.at(EHandle).MatProperties.Spec;			}
		NodeHandle	GetNode				(EntityHandle EHandle) {return Drawables.at(EHandle).Node;							}
		float3		GetEntityPosition	(EntityHandle EHandle);

		bool isEntitySkeletonAvailable			(EntityHandle EHandle);
		bool EntityEnablePosing					(EntityHandle EHandle);

		inline DrawableAnimationState*	GetEntityAnimationState(EntityHandle EHandle) { return Drawables.at(EHandle).AnimationState; }

		GSPlayAnimation_RES EntityPlayAnimation	(EntityHandle EHandle, const char*	Animation,	float w = 1.0f, bool Loop = false);
		GSPlayAnimation_RES EntityPlayAnimation	(EntityHandle EHandle, GUID_t		Guid,		float w = 1.0f, bool Loop = false);
		size_t				GetEntityAnimationPlayCount(EntityHandle EHandle);

		inline void SetVisability		(EntityHandle EHandle, bool Visable = true)	{ DrawableVisibility.at(EHandle) = Visable; }
		inline bool GetVisability		(EntityHandle EHandle)						{ return DrawableVisibility.at(EHandle);	}

		inline void				SetMeshHandle(EntityHandle EHandle, TriMeshHandle M)	{ Drawables.at(EHandle).MeshHandle = M;		}
		inline TriMeshHandle	GetMeshHandle(EntityHandle EHandle)						{ return Drawables.at(EHandle).MeshHandle;	}

		inline void				SetTextureSet		(EntityHandle EHandle, TextureSet* Set)	{ Drawables.at(EHandle).Textures = Set; Drawables.at(EHandle).Textured = true; }
		inline TextureSet*		GetTextureSet		(EntityHandle EHandle)					{ return Drawables.at(EHandle).Textures; }

		inline void SetMaterialParams	(EntityHandle EHandle, float4 RGBA = float4(1.0f, 1.0f, 1.0f, 0.5f), float4 Spec = float4(1.0f, 1.0f, 1.0f, 0.5f))
		{
			Drawables.at(EHandle).MatProperties.Albedo		 = RGBA;
			Drawables.at(EHandle).MatProperties.Spec		 = Spec;
		}

		Drawable& SetNode(EntityHandle EHandle, NodeHandle Node);

		EntityHandle CreateDrawableAndSetMesh(GUID_t Mesh);
		EntityHandle CreateDrawableAndSetMesh(const char* Mesh);
		
		LightHandle	AddPointLight ( float3 Color, float3 POS, float I = 10, float R = 10);
		LightHandle	AddPointLight ( float3 Color, NodeHandle, float I = 10, float R = 10);

		void EnableShadowCasting(SpotLightHandle SpotLight);

		SpotLightHandle AddSpotLight ( NodeHandle Node, float3 Color, float3 Dir, float t = pi / 4, float I = 10.0f, float R = 10.0f);
		SpotLightHandle AddSpotLight ( float3 POS,		float3 Color, float3 Dir, float t = pi / 4, float I = 10, float R = 10);

		void _PushEntity(Drawable E);

		void Yaw					(EntityHandle Handle, float a);	
		void Pitch					(EntityHandle Handle, float a);			
		void Roll					(EntityHandle Handle, float a);
		void TranslateEntity_LT		(EntityHandle Handle, float3 V);			
		void TranslateEntity_WT		(EntityHandle Handle, float3 V);			
		void SetPositionEntity_WT	(EntityHandle Handle, float3 V);			
		void SetPositionEntity_LT	(EntityHandle Handle, float3 V);			
		void SetOrientation			(EntityHandle Handle, Quaternion Q);
		void SetOrientationL		(EntityHandle Handle, Quaternion Q);		
		void SetLightNodeHandle		(SpotLightHandle Handle, NodeHandle Node);

		inline Quaternion GetOrientation(EntityHandle Handle);


		Vector<SpotLightShadowCaster>		SpotLightCasters;
		Vector<Drawable>					Drawables;
		Vector<bool>						DrawableVisibility;
		Vector<EntityHandle>				FreeEntityList;

		struct TaggedJoint
		{
			EntityHandle	Source;
			JointHandle		Joint;
			NodeHandle		Target;
		};

		Vector<TaggedJoint>		TaggedJoints;

		iAllocator*					TempMem;
		iAllocator*					Memory;
		RenderSystem*				RS;
		Resources*					RM;
		SceneNodeComponentSystem*	SN;
		GeometryTable*				GT;

		PointLightBuffer	PLights;
		SpotLightBuffer		SPLights;
		PVS					_PVS;

		ScenePartitionTable	SceneManagement;


		operator GraphicScene* () { return this; }
	};


	FLEXKITAPI void InitiateGraphicScene			( GraphicScene* Out, RenderSystem* in_RS, Resources* in_RM, SceneNodeComponentSystem* in_SN, GeometryTable* GT, iAllocator* Memory, iAllocator* TempMemory );

	FLEXKITAPI void UpdateGraphicScene				( GraphicScene* SM );
	FLEXKITAPI void UpdateAnimationsGraphicScene	( GraphicScene* SM, double dt );
	FLEXKITAPI void UpdateGraphicScenePoseTransform	( GraphicScene* SM );
	FLEXKITAPI void GetGraphicScenePVS				( GraphicScene* SM, Camera* C, PVS* __restrict out, PVS* __restrict T_out );
	FLEXKITAPI void UploadGraphicScene				( GraphicScene* SM, PVS* , PVS* );
	FLEXKITAPI void UpdateShadowCasters				( GraphicScene* SM );

	FLEXKITAPI void ReleaseGraphicScene				( GraphicScene* SM );
	FLEXKITAPI void BindJoint						( GraphicScene* SM, JointHandle Joint, EntityHandle Entity, NodeHandle TargetNode );

	FLEXKITAPI bool LoadScene ( RenderSystem* RS, SceneNodes* SN, Resources* RM, GeometryTable*, GUID_t Guid, GraphicScene* GS_out, iAllocator* Temp );
	FLEXKITAPI bool LoadScene ( RenderSystem* RS, SceneNodes* SN, Resources* RM, GeometryTable*, const char* LevelName, GraphicScene* GS_out, iAllocator* Temp );

	template<typename ALLOC_T>
	void Release(DrawablePoseState* EPS, ALLOC_T* Mem)
	{
		if(EPS->Joints)			Mem->free(EPS->Joints);
		if(EPS->CurrentPose)	Mem->free(EPS->CurrentPose);

		EPS->Joints		 = nullptr;
		EPS->CurrentPose = nullptr;
	}

}

#endif