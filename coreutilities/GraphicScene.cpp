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

#include "GraphicScene.h"
#include "GraphicsComponents.h"
#include "intersection.h"

namespace FlexKit
{
	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawable()
	{
		EntityHandle Out = HandleTable.GetNewHandle();

		Drawable		D;
		NodeHandle N  = GetZeroedNode();

		D.Node = N;
		_PushEntity(D, Out);

		HandleTable[Out] = Drawables.size() - 1;

		SceneManagement.CreateNode(Out);
		return Out;
	}


	/************************************************************************************************/


	void GraphicScene::RemoveEntity(EntityHandle E)
	{
		ReleaseNode(GetDrawable(E).Node);

		auto& Drawable = GetDrawable(E);
		Drawable.Release();

		ReleaseMesh(RS, Drawable.MeshHandle);

		Drawable.MeshHandle		 = INVALIDMESHHANDLE;
		DrawableVisibility[E]	 = false;
		DrawableRayVisibility[E] = false;

		auto Index = HandleTable[E];

		auto TempDrawable			= Drawables.back();
		auto TempVisibility			= DrawableVisibility.back();
		auto TempRayVisilibility	= DrawableRayVisibility.back();
		auto TempDrawableHandle		= DrawableHandles.back();

		auto Index2 = HandleTable[TempDrawableHandle];


		Drawables.pop_back();
		DrawableVisibility.pop_back();
		DrawableRayVisibility.pop_back();
		DrawableHandles.pop_back();

		if (!Drawables.size() || (Index == Drawables.size()))
			return;

		Drawables[Index]				= TempDrawable;
		DrawableVisibility[Index]		= TempVisibility;
		DrawableRayVisibility[Index]	= TempRayVisilibility;
		DrawableHandles[Index]			= TempDrawableHandle;

		HandleTable[TempDrawableHandle] = HandleTable[E];
	}


	/************************************************************************************************/


	void GraphicScene::ClearScene()
	{
		PLights.Release();
		SPLights.Release();


		for (auto& D : this->Drawables)
		{
			ReleaseNode(D.Node);
			ReleaseMesh(RS, D.MeshHandle);
		}

		SceneManagement.Release();
		Drawables.Release();
		DrawableVisibility.Release();
		DrawableRayVisibility.Release();
		DrawableHandles.Release();
		TaggedJoints.Release();
	}


	/************************************************************************************************/


	void GraphicScene::Update()
	{
		SceneManagement.Update();
	}


	/************************************************************************************************/


	bool GraphicScene::isEntitySkeletonAvailable(EntityHandle EHandle)
	{
		auto Index = HandleTable[EHandle];
		if (Drawables.at(Index).MeshHandle != INVALIDMESHHANDLE)
		{
			auto Mesh		= GetMesh(Drawables.at(Index).MeshHandle);
			auto ID			= Mesh->TriMeshID;
			bool Available	= isResourceAvailable(ID);
			return Available;
		}
		return false;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityEnablePosing(EntityHandle EHandle)
	{
		auto Index				= HandleTable[EHandle];
		bool Available			= isEntitySkeletonAvailable(EHandle);
		bool SkeletonAvailable  = false;
		auto MeshHandle			= GetDrawable(EHandle).MeshHandle;

		if (Available) {
			auto Mesh = GetMesh(MeshHandle);
			SkeletonAvailable = isResourceAvailable(Mesh->TriMeshID);
		}

		bool ret = false;
		if (Available && SkeletonAvailable)
		{
			if(!IsSkeletonLoaded(MeshHandle))
			{
				auto SkeletonGUID	= GetSkeletonGUID(MeshHandle);
				auto Handle			= LoadGameResource(SkeletonGUID);
				auto S				= Resource2Skeleton(Handle, Memory);
				SetSkeleton(MeshHandle, S);
			}

			auto& E = GetDrawable(EHandle);
			E.PoseState	= CreatePoseState(&E, Memory);
			E.Posed		= true;
			ret = true;
		}

		return ret;
	}


	/************************************************************************************************/

	bool LoadAnimation(GraphicScene* GS, EntityHandle EHandle, ResourceHandle RHndl, TriMeshHandle MeshHandle, float w = 1.0f)
	{
		auto Resource = GetResource(RHndl);
		if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
		{
			auto AC = Resource2AnimationClip(Resource, GS->Memory);
			FreeResource(RHndl);// No longer in memory once loaded

			auto mesh = GetMesh(MeshHandle);
			mesh->AnimationData |= FlexKit::TriMesh::AnimationData::EAD_Skin;
			AC.Skeleton = mesh->Skeleton;

			if (AC.Skeleton->Animations)
			{
				auto I = AC.Skeleton->Animations;
				while (I->Next)
					I = I->Next;

				I->Next				= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				I->Next->Clip		= AC;
				I->Next->Memory		= GS->Memory;
				I->Next->Next		= nullptr;
			}
			else
			{
				AC.Skeleton->Animations			= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				AC.Skeleton->Animations->Clip	= AC;
				AC.Skeleton->Animations->Next	= nullptr;
				AC.Skeleton->Animations->Memory = GS->Memory;
			}

			return true;
		}
		return false;
	}


	/************************************************************************************************/


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(EntityHandle EHandle, GUID_t Guid, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded)
		{
			auto S = FlexKit::GetSkeleton(MeshHandle);
			Skeleton::AnimationList* I = S->Animations;
			bool AnimationLoaded = false;

			// Find if Animation is Already Loaded
			{
				
				while (I)
				{
					if (I->Clip.guid == Guid) {
						AnimationLoaded = true;
						break;
					}
					
					I = I->Next;
				}
			}
			if (!AnimationLoaded)
			{
				// Search Resources for Animation
				if (isResourceAvailable(Guid))
				{
					auto RHndl = LoadGameResource(Guid);
					auto Res = LoadAnimation(this, EHandle, RHndl, MeshHandle, W);
					if(!Res)
						return{ false, -1 };
				}
				else
					return{ false, -1 };
			}

			int64_t ID = INVALIDHANDLE;
			auto Res = PlayAnimation(&GetDrawable(EHandle), Guid, Memory, Loop, W, ID);
			if(Res == EPLAY_ANIMATION_RES::EPLAY_SUCCESS)
				return { true, ID };

			return{ false, -1};
		}
		return{ false, -1 };
	}


	/************************************************************************************************/


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(EntityHandle EHandle, const char* Animation, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded && HasAnimationData(MeshHandle))
		{
			// TODO: Needs to Iterate Over Clips
			auto S = FlexKit::GetSkeleton(MeshHandle);
			if (!strcmp(S->Animations->Clip.mID, Animation))
			{
				int64_t AnimationID = -1;
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID))
					return { true, AnimationID };
				return{ false, -1 };
			}
		}

		// Search Resources for Animation
		if(isResourceAvailable(Animation))
		{
			auto RHndl = LoadGameResource(Animation);
			int64_t AnimationID = -1;
			if (LoadAnimation(this, EHandle, RHndl, MeshHandle, W)) {
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID) == EPLAY_SUCCESS)
					return { true, AnimationID };
				return{ false, -1};
			}
			else
				return { false, -1 };
		}
		return { false, -1 };
	}


	/************************************************************************************************/


	size_t	GraphicScene::GetEntityAnimationPlayCount(EntityHandle EHandle)
	{
		size_t Out = 0;
		Out = GetAnimationCount(&Drawables.at(HandleTable[EHandle]));
		return Out;
	}


	/************************************************************************************************/


	Drawable& GraphicScene::SetNode(EntityHandle EHandle, NodeHandle Node) 
	{
		FlexKit::ReleaseNode(GetNode(EHandle));
		auto& Drawable = Drawables.at(HandleTable[EHandle]);
		Drawable.Node = Node;
		return Drawable;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(GUID_t Mesh)
	{
		auto EHandle = CreateDrawable();

		auto		Geo = FindMesh(Mesh);
		if (!Geo)	Geo = LoadTriMeshIntoTable(RS, Mesh);

		auto& Drawble       = GetDrawable(EHandle);
		SetVisability(EHandle, true);

		Drawble.MeshHandle	= Geo;
		Drawble.Dirty		= true;
		Drawble.Textured	= false;
		Drawble.Textures	= nullptr;

		SetRayVisability(EHandle, true);

		return EHandle;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(const char* Mesh)
	{
		auto EHandle = CreateDrawable();

		TriMeshHandle MeshHandle = FindMesh(Mesh);
		if (MeshHandle == INVALIDMESHHANDLE)	
			MeshHandle = LoadTriMeshIntoTable(RS, Mesh);

#ifdef _DEBUG
		FK_ASSERT(MeshHandle != INVALIDMESHHANDLE, "FAILED TO FIND MESH IN RESOURCES!");
#endif

		auto& Drawble       = GetDrawable(EHandle);
		SetVisability(EHandle, true);

		Drawble.Textures    = nullptr;
		Drawble.MeshHandle  = MeshHandle;
		Drawble.Dirty		= true;
		Drawble.Textured    = false;
		Drawble.Posed		= false;
		Drawble.PoseState   = nullptr;

		return EHandle;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, NodeHandle Node, float I, float R)
	{
		PLights.push_back({Color, I, R, Node});
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(NodeHandle Node, float3 Color, float3 Dir, float t, float I, float R )
	{
		SPLights.push_back({ Color, Dir, I, R, t, Node });
		return PLights.size() - 1;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, float3 POS, float I, float R)
	{
		auto Node = GetNewNode();
		SetPositionW(Node, POS);
		PLights.push_back({ Color, I, R, Node });
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(float3 POS, float3 Color, float3 Dir, float t, float I, float R)
	{
		auto Node = GetNewNode();
		SetPositionW(Node, POS);
		SPLights.push_back({Color, Dir, I, R, t, Node});
		return PLights.size() - 1;
	}

	
	/************************************************************************************************/


	void GraphicScene::EnableShadowCasting(SpotLightHandle SpotLight)
	{
		FK_ASSERT(0, "UNIMPLENTED");
		auto Light = &SPLights[0];
		//SpotLightCasters.push_back({ Camera(), SpotLight});
		//SpotLightCasters.back().C.FOV = RadToDegree(Light->t);
		//InitiateCamera(*SN, &SpotLightCasters.back().C, 1.0f, 15.0f, 100, false);
	}


	/************************************************************************************************/


	void GraphicScene::_PushEntity(Drawable E, EntityHandle Handle)
	{
		Drawables.push_back(E);
		DrawableVisibility.push_back(false);
		DrawableRayVisibility.push_back(false);
		DrawableHandles.push_back(Handle);
	}


	/************************************************************************************************/


	void UpdateQuadTree(QuadTreeNode* Node, GraphicScene* Scene)
	{
		for (const auto D : Scene->Drawables) {
			if (GetFlag(D.Node, SceneNodes::StateFlags::UPDATED))
			{
				/*
				if()
				{	// Moved Branch
					if()
					{	// Space in Node Available
					}
					else
					{	// Node Split Needed
					}
				}
				*/
			}
		}
	}


	/************************************************************************************************/


	void QuadTree::Release()
	{
	}


	/************************************************************************************************/


	void QuadTree::CreateNode(EntityHandle Handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::ReleaseNode(EntityHandle Handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::Update()
	{
	}


	/************************************************************************************************/


	void QuadTree::Initiate(iAllocator* memory, float2 AreaDimensions)
	{
		Memory = memory;
		Area = AreaDimensions;
	}


	/************************************************************************************************/


	void InitiateGraphicScene()
	{
		
	}


	/************************************************************************************************/


	void UpdateAnimationsGraphicScene(GraphicScene* SM, double dt)
	{
		for (auto E : SM->Drawables)
		{
			if (E.Posed) {
				if (E.AnimationState && GetAnimationCount(&E))
					UpdateAnimation(SM->RS, &E, dt, SM->TempMemory);
				else
					ClearAnimationPose(E.PoseState, SM->TempMemory);
			}
		}
	}


	/************************************************************************************************/


	void UpdateGraphicScenePoseTransform(GraphicScene* SM)
	{
		for(auto Tag : SM->TaggedJoints)
		{
			auto Entity = SM->GetDrawable(Tag.Source);

			auto WT		= GetJointPosed_WT(Tag.Joint, Entity.Node, Entity.PoseState);
			auto WT_t	= Float4x4ToXMMATIRX(&WT.Transpose());

			FlexKit::SetWT		(Tag.Target, &WT_t);
			FlexKit::SetFlag	(Tag.Target, SceneNodes::StateFlags::UPDATED);
		}
	}


	/************************************************************************************************/


	void UpdateShadowCasters(GraphicScene* GS)
	{
		//for (auto& Caster : GS->SpotLightCasters)
		//{
		//	UpdateCamera(GS->RS, *GS->SN, &Caster.C, 0.00f);
		//}
	}


	/************************************************************************************************/


	void UpdateGraphicScene(GraphicScene* SM)
	{
		SM->SceneManagement.Update();
		UpdateShadowCasters(SM);
	}


	/************************************************************************************************/


	void GetGraphicScenePVS(GraphicScene* SM, CameraHandle Camera, PVS* __restrict out, PVS* __restrict T_out)
	{
		FK_ASSERT(out		!= T_out);
		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);
		FK_ASSERT(out		!= nullptr);
		FK_ASSERT(T_out		!= nullptr);


		auto CameraNode = GetCameraNode(Camera);
		float3	POS		= GetPositionW(CameraNode);
		Quaternion Q	= GetOrientation(CameraNode);

		auto F			= GetFrustum(Camera);

		auto End = SM->Drawables.size();
		for (size_t I = 0; I < End; ++I)
		{
			auto &E = SM->Drawables[I];
			auto Mesh	= GetMesh			(E.MeshHandle);
			auto Ls		= GetLocalScale		(E.Node).x;
			auto Pw		= GetPositionW		(E.Node);
			auto Lq		= GetOrientation	(E.Node);
			auto BS		= Mesh->BS;

			BoundingSphere BoundingVolume = float4((Lq * BS.xyz()) + Pw, BS.w * Ls);

			auto DrawableVisibility = SM->DrawableVisibility[I];

			if (DrawableVisibility && Mesh && CompareBSAgainstFrustum(&F, BoundingVolume))
			{
				if (!E.Transparent)
					PushPV(&E, out);
				else
					PushPV(&E, T_out);
			}
		}	
	}


	/************************************************************************************************/


	void UploadGraphicScene(GraphicScene* SM, PVS* Dawables, PVS* Transparent_PVS)
	{
	}


	/************************************************************************************************/


	void ReleaseSceneAnimation(AnimationClip* AC, iAllocator* Memory)
	{
		for (size_t II = 0; II < AC->FrameCount; ++II) 
		{
			Memory->free(AC->Frames[II].Joints);
			Memory->free(AC->Frames[II].Poses);
		}

		Memory->free(AC->Frames);
		Memory->free(AC->mID);
	}


	/************************************************************************************************/


	void ReleaseGraphicScene(GraphicScene* SM)
	{
		for (auto E : SM->Drawables)
		{
			ReleaseMesh(SM->RS, E.MeshHandle);

			if (E.PoseState) 
			{
				FK_ASSERT(0);
				//Release(E.PoseState);
				//Release(E.PoseState, SM->Memory);
				SM->Memory->_aligned_free(E.PoseState);
				SM->Memory->_aligned_free(E.AnimationState);
			}
		}

		SM->Drawables.Release();
		SM->TaggedJoints.Release();
		SM->PLights.Lights	= nullptr;
		SM->Drawables		= nullptr;
	}


	/************************************************************************************************/


	void BindJoint(GraphicScene* SM, JointHandle Joint, EntityHandle Entity, NodeHandle TargetNode)
	{
		SM->TaggedJoints.push_back({ Entity, Joint, TargetNode });
	}


	/************************************************************************************************/


	//bool LoadScene(RenderSystem* RS, Resources* RM, GeometryTable*, const char* ID, GraphicScene* GS_out)
	//{
	//	return false;
	//}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, GUID_t Guid, GraphicScene* GS_out, iAllocator* Temp)
	{
		bool Available = isResourceAvailable(Guid);
		if (Available)
		{
			auto RHandle = LoadGameResource(Guid);
			auto R		 = GetResource(RHandle);

			FINALLY
			{
				FreeResource(RHandle);
			}
			FINALLYOVER

			if (R != nullptr) {
				SceneResourceBlob* SceneBlob = (SceneResourceBlob*)R;

				auto& CreatedNodes = 
						Vector<NodeHandle>(
							Temp, 
							SceneBlob->SceneTable.NodeCount);

				{
					CompiledScene::SceneNode* Nodes = (CompiledScene::SceneNode*)(SceneBlob->Buffer + SceneBlob->SceneTable.NodeOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.NodeCount; ++I)
					{
						auto Node		= Nodes[I];
						auto NewNode	= GetNewNode();
						
						SetOrientationL	(NewNode, Node.Q );
						SetPositionL	(NewNode, Node.TS.xyz());
						SetScale		(NewNode, { Node.TS.w, Node.TS.w, Node.TS.w });

						if (Node.Parent != -1)
							SetParentNode(CreatedNodes[Node.Parent], NewNode);

						CreatedNodes.push_back(NewNode);
					}
				}

				{
					auto Entities = (CompiledScene::Entity*)(SceneBlob->Buffer + SceneBlob->SceneTable.EntityOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.EntityCount; ++I)
					{
						if (Entities[I].MeshGuid != INVALIDHANDLE) {
							auto NewEntity = GS_out->CreateDrawableAndSetMesh(Entities[I].MeshGuid);
							GS_out->SetNode(NewEntity, CreatedNodes[Entities[I].Node]);
							auto Position_DEBUG = GetPositionW(CreatedNodes[Entities[I].Node]);
							SetFlag(CreatedNodes[Entities[I].Node], SceneNodes::StateFlags::SCALE);
							int x = 0;
						}
					}
				}

				{
					auto Lights = (CompiledScene::PointLight*)(SceneBlob->Buffer + SceneBlob->SceneTable.LightOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.LightCount; ++I)
					{
						auto Light		= Lights[I];
						auto NewEntity	= GS_out->AddPointLight(Light.K, CreatedNodes[Light.Node], Light.I, Light.R * 10);
					}
				}
				return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, const char* LevelName, GraphicScene* GS_out, iAllocator* Temp)
	{
		if (isResourceAvailable(LevelName))
		{
			auto RHandle = LoadGameResource(LevelName);
			auto R = GetResource(RHandle);

			FINALLY
			{
				FreeResource(RHandle);
			}
			FINALLYOVER

			return LoadScene(RS, R->GUID, GS_out, Temp);
		}
		return false;
	}


	/************************************************************************************************/


	float3 GraphicScene::GetEntityPosition(EntityHandle EHandle) 
	{ 
		return GetPositionW(Drawables.at(EHandle).Node); 
	}


	/************************************************************************************************/


	Quaternion GraphicScene::GetOrientation(EntityHandle Handle)
	{
		return FlexKit::GetOrientation(GetNode(Handle));
	}


	/************************************************************************************************/

	/*
	void GraphicScene::Yaw					(EntityHandle Handle, float a)				{ FlexKit::Yaw	(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::Pitch				(EntityHandle Handle, float a)				{ FlexKit::Pitch(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::Roll					(EntityHandle Handle, float a)				{ FlexKit::Roll	(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::TranslateEntity_LT	(EntityHandle Handle, float3 V)				{ FlexKit::TranslateLocal	(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::TranslateEntity_WT	(EntityHandle Handle, float3 V)				{ FlexKit::TranslateWorld	(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetPositionEntity_WT	(EntityHandle Handle, float3 V)				{ FlexKit::SetPositionW		(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetPositionEntity_LT	(EntityHandle Handle, float3 V)				{ FlexKit::SetPositionL		(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetOrientation		(EntityHandle Handle, Quaternion Q)			{ FlexKit::SetOrientation	(*SN, GetDrawable(Handle).Node, Q);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetOrientationL		(EntityHandle Handle, Quaternion Q)			{ FlexKit::SetOrientationL	(*SN, GetDrawable(Handle).Node, Q);  GetDrawable(Handle).Dirty = true; }
	*/

	void GraphicScene::SetLightNodeHandle	(SpotLightHandle Handle, NodeHandle Node)	{ FlexKit::ReleaseNode		(PLights[Handle].Position); PLights[Handle].Position = Node;	   }



	



	Drawable::VConsantsLayout Drawable::GetConstants()
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetTransform(Node, &WT);

		Drawable::VConsantsLayout	Constants;

		Constants.MP.Albedo = float4(0, 1, 1, 0.5f);// E->MatProperties.Albedo;
		Constants.MP.Spec	= MatProperties.Spec;
		Constants.Transform = DirectX::XMMatrixTranspose(WT);

		return Constants;
	}

}	/************************************************************************************************/