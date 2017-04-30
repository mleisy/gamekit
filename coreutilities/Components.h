/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "..\buildsettings.h"
#include "..\graphicsutilities\graphics.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Handle.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\type.h"


#ifndef COMPONENT_H
#define COMPONENT_H

namespace FlexKit
{
	// GOAL: Help reduce code involved to do shit which is currently excessive 
	/************************************************************************************************/

	// Components store all data needed for a component
	typedef FlexKit::Handle_t<16> ComponentHandle;
	typedef uint32_t EventTypeID;

	typedef uint32_t ComponentType;


	struct Component;
	struct GameObjectInterface;

	class ComponentSystemInterface
	{
	public:
		virtual void ReleaseHandle	(ComponentHandle Handle) = 0;
		virtual void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID, Component* Components, size_t ComponentCount) {}
	};

	const uint32_t UnknownComponentID = GetTypeGUID(Component);

	struct Component
	{
		Component() :
			ComponentSystem	(nullptr),
			Type			(GetTypeGUID(Component)){}


		Component(
			ComponentSystemInterface*	CS,
			FlexKit::Handle_t<16>		CH,
			ComponentType				T 
		) :
			ComponentSystem		(CS),
			ComponentHandle		(CH),
			Type				(T)	{}


		Component& operator = (Component&& RHS)
		{
			ComponentSystem			= RHS.ComponentSystem;
			ComponentHandle			= RHS.ComponentHandle;
			Type					= RHS.Type;

			RHS.ComponentSystem		= nullptr;

			return *this;
		}


		Component(const Component& RValue)
		{
			ComponentHandle		= RValue.ComponentHandle;
			ComponentHandle		= RValue.ComponentHandle;
			Type				= RValue.Type;
		}


		~Component()
		{
			Release();
		}


		void Release()
		{
			if (ComponentSystem)
				ComponentSystem->ReleaseHandle(ComponentHandle);

			ComponentSystem = nullptr;
		}

		ComponentSystemInterface*	ComponentSystem;
		ComponentType				Type;
		Handle_t<16>				ComponentHandle;
	};


	/************************************************************************************************/


	template<size_t COMPONENTCOUNT = 6>
	struct FLEXKITAPI GameObject
	{
		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		Component	Components[COMPONENTCOUNT];
		
		static const size_t MaxComponentCount = COMPONENTCOUNT;
		
		GameObject()
		{
			LastComponent	= 0;
			ComponentCount	= 0;
		}

		~GameObject()
		{
			for (size_t I = 0; I < ComponentCount; ++I)
				Components[I].Release();
		}


		bool Full()
		{
			return (MaxComponentCount <= ComponentCount);
		}

		void NotifyAll(ComponentType Source, EventTypeID EventID)
		{
			for (size_t I = 0; I < ComponentCount; ++I) 
			{
				if(Components[I].ComponentSystem)
					Components[I].ComponentSystem->HandleEvent(
						Components[I].ComponentHandle, 
						Source, EventID, 
						Components, 
						ComponentCount);
			}
		}s

		bool AddComponent(Component&& NewC)
		{
			if (!Full())
			{
				for (auto& C : Components) {
					if (C.Type == UnknownComponentID) {
						C = std::move(NewC);
						break;
					}
				}

				++ComponentCount;
				return true;
			}
			return false;
		}

		void Release()
		{
			for (size_t I = 0; I < ComponentCount; ++I) {
				if (Components[I].ComponentSystem)
					Components[I].Release();
			}
		}


		operator GameObjectInterface* ()
		{
			return (GameObjectInterface*)this;
		}
	};


	struct FLEXKITAPI GameObjectInterface
	{
		uint16_t	LastComponent;
		uint16_t	ComponentCount;
		Component	Components[];
	};


	Component*	FindComponent(GameObjectInterface* GO, ComponentType T)
	{
		if (GO->LastComponent != -1) {
			if (GO->Components[GO->LastComponent].Type == T)
			{
				return &GO->Components[GO->LastComponent];
			}
		}

		for (size_t I = 0; I < GO->ComponentCount; ++I)
			if (GO->Components[I].Type == T) {
				GO->LastComponent = I;
				return &GO->Components[I];
			}

		return nullptr;
	}	

	/************************************************************************************************/


	template<typename TY_GO>
	void CreateComponent(TY_GO& GO)
	{
	}


	template<typename TY_GO>
	void InitiateGameObject(TY_GO& GO) {}


	template<size_t COUNT, typename TY, typename ... TY_ARGS>
	void InitiateGameObject(GameObject<COUNT>& GO, TY Component, TY_ARGS ... Args)
	{
		CreateComponent(GO, Component);
		InitiateGameObject(GO, Args...);
	}


	/************************************************************************************************/
}

#endif