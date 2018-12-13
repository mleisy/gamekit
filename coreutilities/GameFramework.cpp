/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "ConsoleSubState.h"
#include "GameFramework.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\TextureUtilities.h"
#include "..\coreutilities\Logging.h"


// Todo List
//	Gameplay:
//		Entity Model
//	Sound:
//	
//	Generic:
//		(DONE) Scene Loading
//		Config loading system?
//
//	Graphics:
//		(PARTLY) Frame Graph Rendering
//				(Done) Auto Barrier Handling
//				TODO: (In Progress) Replacement Draw Utility Functions that use the frame graph rather then direct submission
//
//		(DONE) Basic Gui rendering methods (Draw Rect, etc)
//		(PARTLY) Multi-threaded Texture Uploads
//		TODO: (Partly)	Terrain Rendering
//				(DONE) Geometry generation
//				(DONE) CULLING
//				Texture Splatting
//				Normal Generation
//		(DONE) Occlusion Culling
//		(TODO:)(Partly Done)Animation State Machine
//		(DONE/PARTLY) 3rd Person Camera Handler
//		(DONE) Object -> Bone Attachment
//		(DONE) Texture Loading
//		(DONE) Simple Window Utilities
//		(DONE) Simple Window Elements
//		(DONE) Deferred Rendering
//		(DONE) Forward Rendering
//		(DONE) Debug Drawing Functions for Drawing Circles, Capsules, Boxes
//
//		Particles
//		Static Mesh Batcher
//
//		Bugs:
//			TextRendering Bug, Certain Characters do not get Spaces Correctly
//
//	AI:
//		Path Finding
//		State Handling
//
//	Physics:
//		(DONE) Statics
//		(DONE) TriMesh Statics
//		
//	Network:
//		Client:
//		(DONE) Connect to Server
//		(DONE) Basic Game Room
//		(DONE) Client Side Prediction
//		Server:
//		(DONE) Correct errors on Clients deviating from Server
//
//	Tools:
//		(DONE) Meta-Data for Resource Compiling
//		(PARTLY) TTF Loading
//
// Fix Resource Table leak


namespace FlexKit
{	/************************************************************************************************/


	bool SetDebugRenderMode	(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void* USR);
	void EventsWrapper		(const Event& evt, void* _ptr);


	/************************************************************************************************/


	void HandleKeyEvents(const Event& in, GameFramework* framework)
	{
		switch (in.Action)
		{
		case Event::InputAction::Pressed:
		{
			switch (in.mData1.mKC[0])
			{
			case KC_ESC:
				framework->quit = true;
				break;
			case KC_R:
				framework->core->RenderSystem.QueuePSOLoad(DRAW_LINE_PSO);
				framework->core->RenderSystem.QueuePSOLoad(TERRAIN_CULL_PSO);
				framework->core->RenderSystem.QueuePSOLoad(TERRAIN_DRAW_PSO);
				framework->core->RenderSystem.QueuePSOLoad(TERRAIN_DRAW_PSO_DEBUG);
				framework->core->RenderSystem.QueuePSOLoad(TERRAIN_DRAW_WIRE_PSO);
				break;
			case KC_E:
			{
			}	break;
			case KC_T:
				framework->core->RenderSystem.QueuePSOLoad(TILEDSHADING_SHADE);
				break;
			case KC_M:
				framework->MouseState.Enabled = !framework->MouseState.Enabled;

				if (framework->MouseState.Enabled)
					FK_LOG_INFO("Mouse Enabled");
				else
					FK_LOG_INFO("Mouse Disabled");

				break;
			case KC_TILDA:
			{
				FK_VLOG(Verbosity_9, "Console Key Pressed!");

				if (!framework->consoleActive) {
					PushSubState(framework, &framework->core->GetBlockMemory().allocate<ConsoleSubState>(framework));
					framework->consoleActive = true;
				}
			}	break;
			case KC_F1:
			{
				auto temp1 = framework->drawDebug;
				auto temp2 = framework->drawDebugStats;
				framework->drawDebug		= !framework->drawDebug		 | (framework->drawDebugStats & !(temp1 & temp2));
				framework->drawDebugStats	= !framework->drawDebugStats | (framework->drawDebug		 & !(temp1 & temp2));
			}	break;
			case KC_F2:
			{
				framework->drawDebugStats = !framework->drawDebugStats;
			}	break;
			case KC_F3:
			{
			}	break;
			default:
				break;
			}
		}	break;
		default:
			break;
		}
	}


	/************************************************************************************************/


	void PushMessageToConsole(void* User, const char* Str, size_t StrLen)
	{
		GameFramework* framework = reinterpret_cast<GameFramework*>(User);

		char* NewStr = (char*)framework->core->GetBlockMemory().malloc(StrLen + 1);
		memset((void*)NewStr, '\0', StrLen + 1);
		strncpy_s(NewStr, StrLen + 1, Str, StrLen);

		framework->console.PrintLine(NewStr, framework->core->GetBlockMemory());
	}


	/************************************************************************************************/


	GameFramework::GameFramework(EngineCore* IN_core) :
		console	{DefaultAssets.Font, IN_core->GetBlockMemory()},
		core	{IN_core}
	{
		Initiate();
	}


	/************************************************************************************************/


	void GameFramework::Initiate()
	{
		SetDebugMemory			(core->GetDebugMemory());
		InitiateResourceTable	(core->GetBlockMemory());
		InitiateGeometryTable	(core->GetBlockMemory());
		InitiateCameraTable		(core->GetBlockMemory());

		clearColor					= { 0.0f, 0.2f, 0.4f, 1.0f };
		quit						= false;
		physicsUpdateTimer			= 0.0f;

		drawDebug					= false;

#ifdef _DEBUG
		drawDebugStats			= true;
#else
		drawDebugStats			= false;
#endif

		//framework.ActivePhysicsScene		= nullptr;
		ActiveScene					= nullptr;
		ActiveWindow				= &core->Window;

		drawPhysicsDebug			= false;

		stats.fps						= 0;
		stats.fpsCounter				= 0;
		stats.fpsT						= 0.0;
		stats.objectsDrawnLastFrame		= 0;
		rootNode						= GetZeroedNode();

		uint2	WindowRect	   = core->Window.WH;
		float	Aspect		   = (float)WindowRect[0] / (float)WindowRect[1];

		MouseState.NormalizedPos	= { 0.5f, 0.5f };
		MouseState.Position			= { float(WindowRect[0]/2), float(WindowRect[1] / 2) };


		EventNotifier<>::Subscriber sub;
		sub.Notify = &EventsWrapper;
		sub._ptr   = this;
		core->Window.Handler.Subscribe(sub);

		console.BindUIntVar("FPS",			&stats.fps);
		console.BindBoolVar("HUD",			&drawDebugStats);
		console.BindBoolVar("DrawDebug",	&drawDebug);
		//BindBoolVar(&framework.Console, "DrawPhysicsDebug",	&framework.DrawPhysicsDebug);
		console.BindBoolVar("FrameLock",		&core->FrameLock);

		console.AddFunction({ "SetRenderMode", &SetDebugRenderMode, this, 1, { ConsoleVariableType::CONSOLE_UINT }});
		AddLogCallback(&logMessagePipe, Verbosity_INFO);

		core->RenderSystem.UploadResources();// Uploads fresh Resources to GPU
	}


	/************************************************************************************************/


	void GameFramework::Update(double dT)
	{
		timeRunning += dT;


		UpdateMouseInput(&MouseState, &core->Window);

		if (!subStates.size()) {
			quit = true;
			return;
		}

		UpdateDispatcher Dispatcher{ core->GetTempMemory() };

		for(size_t I = 1; I <= subStates.size(); ++I)
		{

			auto& State = subStates[subStates.size() - I];

			if (!State->Update(core, Dispatcher, dT))
				break;
		}

		Dispatcher.Execute();
		core->End = quit;
	}


	/************************************************************************************************/


	void GameFramework::UpdateFixed(double dt)
	{
		UpdateMouseInput(&MouseState, &core->Window);
	}


	/************************************************************************************************/


	void GameFramework::UpdatePreDraw(iAllocator* TempMemory, double dT)
	{
		if (!subStates.size()) {
			quit = true;
			return;
		}

		{
			UpdateDispatcher dispatcher{ core->GetTempMemory() };

			if (drawDebug) 
			{
				for(size_t I = 1; I <= subStates.size(); ++I)
				{
					auto& State = subStates[subStates.size() - I];

					if (!State->DebugDraw(core, dispatcher, dT))
						break;
				}
			}

			dispatcher.Execute();
		}

		{
			UpdateDispatcher dispatcher{ core->GetTempMemory() };

			for (size_t I = 1; I <= subStates.size(); ++I)
			{
				auto& State = subStates[subStates.size() - I];
				if (!State->PreDrawUpdate(core, dispatcher, dT))
					break;
			}

			dispatcher.Execute();
		}

		if (stats.fpsT > 1.0)
		{
			stats.fps         = stats.fpsCounter;
			stats.fpsCounter = 0;
			stats.fpsT       = 0.0;
		}

		stats.fpsCounter++;
		stats.fpsT += dT;
	}


	/************************************************************************************************/


	void GameFramework::Draw(iAllocator* TempMemory)
	{
		FrameGraph		FrameGraph(core->RenderSystem, TempMemory);

		// Add in Base Resources
		FrameGraph.Resources.AddRenderTarget(core->Window.GetBackBuffer());
		FrameGraph.UpdateFrameGraph(core->RenderSystem, ActiveWindow, core->GetTempMemory());

		UpdateDispatcher Dispatcher{ core->GetTempMemory() };

		for (size_t I = 0; I < subStates.size(); ++I)
		{
			auto& SubState = subStates[I];
			if (!SubState->Draw(core, Dispatcher, 0, FrameGraph))
				break;
		}

		for (size_t I = 1; I <= subStates.size(); ++I)
		{
			auto& State = subStates[subStates.size() - I]; 
			if (!State->PostDrawUpdate(core, Dispatcher, 0, FrameGraph))
				break;
		}

		Dispatcher.Execute();

		ProfileBegin(PROFILE_SUBMISSION);

		if(	ActiveWindow )
		{
			FrameGraph.SubmitFrameGraph(core->RenderSystem, ActiveWindow);

			Free_DelayedReleaseResources(core->RenderSystem);
		}

		ProfileEnd(PROFILE_SUBMISSION);
	}


	/************************************************************************************************/


	void GameFramework::PostDraw(iAllocator* TempMemory, double dt)
	{
		core->RenderSystem.PresentWindow(&core->Window);
	}


	/************************************************************************************************/


	void GameFramework::Cleanup()
	{
		auto end = subStates.rend();
		auto itr = subStates.rbegin();


		console.Release();
		Release(DefaultAssets.Font, core->RenderSystem);

		// wait for last Frame to finish Rendering
		auto CL = core->RenderSystem._GetCurrentCommandList();

		for (size_t I = 0; I < 4; ++I) 
		{
			core->RenderSystem.WaitforGPU();
			core->RenderSystem._IncrementRSIndex();
		}


		// Counters are at Max 3
		Free_DelayedReleaseResources(core->RenderSystem);
		Free_DelayedReleaseResources(core->RenderSystem);
		Free_DelayedReleaseResources(core->RenderSystem);

		FreeAllResourceFiles	();
		FreeAllResources		();
	
		ReleaseGameFramework(core, this);
	}


	/************************************************************************************************/


	bool GameFramework::DispatchEvent(const Event& evt)
	{
		auto itr = subStates.rbegin();
		while (itr != subStates.rend())
		{
			if (!(*itr)->EventHandler(evt))
				return false;
			itr++;
		}
		return true;
	}


	/************************************************************************************************/


	void GameFramework::DrawDebugHUD(double dT, VertexBufferHandle textBuffer, FrameGraph& graph)
	{
		uint32_t VRamUsage	= core->RenderSystem._GetVidMemUsage() / MEGABYTE;
		char* TempBuffer	= (char*)core->GetTempMemory().malloc(512);
		auto DrawTiming		= float(GetDuration(PROFILE_SUBMISSION)) / 1000.0f;

		sprintf_s(TempBuffer, 512, 
			"Current VRam Usage: %u MB\n"
			"FPS: %u\n"
			"Draw Time: %fms\n"
			"Objects Drawn: %u\n"
			"Build Date: " __DATE__ "\n",
			VRamUsage, 
			(uint32_t)stats.fps, DrawTiming, 
			(uint32_t)stats.objectsDrawnLastFrame);


		PrintTextFormatting Format = PrintTextFormatting::DefaultParams();
		Format.Scale = { 0.5f, 0.5f};
		DrawSprite_Text(
				TempBuffer, 
				graph, 
				*DefaultAssets.Font, 
				textBuffer, 
				GetCurrentBackBuffer(&core->Window), 
				core->GetTempMemory(), 
				Format);
	}


	/************************************************************************************************/


	void GameFramework::PostPhysicsUpdate()
	{

	}


	/************************************************************************************************/


	void GameFramework::PrePhysicsUpdate()
	{

	}


	/************************************************************************************************/


	void GameFramework::PopState()
	{
		subStates.back()->~FrameworkState();
		core->GetBlockMemory().free(subStates.back());
		subStates.pop_back();
	}


	/************************************************************************************************/


	void HandleMouseEvents(const Event& in, GameFramework* framework) {
	switch (in.Action)
	{
	case Event::InputAction::Pressed:
	{
		if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			framework->MouseState.LMB_Pressed = true;
		}
	}	break;
	case Event::InputAction::Release:
	{
		if (in.mData1.mKC[0] == KC_MOUSELEFT) {
			framework->MouseState.LMB_Pressed = false;
		}
	}	break;
	default:
		break;
	}
}


	/************************************************************************************************/


	void EventsWrapper(const Event& evt, void* _ptr)
	{
		auto* framework = reinterpret_cast<GameFramework*>(_ptr);

		auto itr = framework->subStates.rbegin();

		if (framework->DispatchEvent(evt))
		{
			switch (evt.InputSource)
			{
			case Event::Keyboard:
				HandleKeyEvents(evt, framework);
			case Event::Mouse:
				HandleMouseEvents(evt, framework);
				break;
			}
		}

	}


	/************************************************************************************************/


	bool LoadScene(EngineCore* core, GraphicScene* scene, const char* sceneName)
	{
		return LoadScene(core->RenderSystem, sceneName, scene, core->GetTempMemory());
	}


	/************************************************************************************************/


	bool LoadScene(EngineCore* core, GraphicScene* scene, GUID_t sceneID)
	{
		return LoadScene(core->RenderSystem, sceneID, scene, core->GetTempMemory());
	}


	/************************************************************************************************/


	void DrawMouseCursor(
		float2					CursorPos, 
		float2					CursorSize, 
		VertexBufferHandle		vertexBuffer, 
		ConstantBufferHandle	constantBuffer,
		TextureHandle			renderTarget,
		iAllocator*				tempMemory,
		FrameGraph*				frameGraph)
	{
		DrawShapes(
			DRAW_PSO,
			*frameGraph,
			vertexBuffer,
			constantBuffer,
			renderTarget,
			tempMemory,
			RectangleShape(
				CursorPos,
				CursorSize,
				{Grey(1.0f), 1.0f}));
	}


	/************************************************************************************************/


	void ReleaseGameFramework(EngineCore* Core, GameFramework* State)
	{
		ClearLogCallbacks();

		auto RItr = State->subStates.rbegin();
		auto REnd = State->subStates.rend();
		while (RItr != REnd)
		{
			(*RItr)->~FrameworkState();
			Core->GetBlockMemory().free(*RItr);

			RItr++;
		}


		ReleaseGeometryTable();
		ReleaseResourceTable();

		//TODO
		//Release(State->DefaultAssets.Font);
		Release(State->DefaultAssets.Terrain);

		Core->Threads.SendShutdown();
		Core->Threads.WaitForWorkersToComplete();
	}


	/************************************************************************************************/


	inline void PushSubState(GameFramework* _ptr, FrameworkState* SS)
	{
		_ptr->subStates.push_back(SS);
	}


	/************************************************************************************************/


	void PopSubState(GameFramework* framework)
	{
		if (!framework->subStates.size())
			return;

		FrameworkState* State = framework->subStates.back();
		State->~FrameworkState();

		framework->core->GetBlockMemory().free(State);
		framework->subStates.pop_back();
	}


	/************************************************************************************************/


	bool SetDebugRenderMode(Console* C, ConsoleVariable* Arguments, size_t ArguementCount, void* USR)
	{
		GameFramework* framework = (GameFramework*)USR;
		if (ArguementCount == 1 && Arguments->Type == ConsoleVariableType::STACK_STRING) {
			size_t Mode = 0;

			const char* VariableIdentifier = (const char*)Arguments->Data_ptr;
			for (auto Var : C->variables)
			{
				if (!strncmp(Var.VariableIdentifier.str, VariableIdentifier, min(strlen(Var.VariableIdentifier.str), Arguments->Data_size)))
				{
					if(Var.Type == ConsoleVariableType::CONSOLE_UINT)
					Mode = *(size_t*)Var.Data_ptr;
				}
			}
		}
		return false;
	}


	/************************************************************************************************/


	void InitiateFramework(EngineCore* Core, GameFramework& framework)
	{

	}


}	/************************************************************************************************/