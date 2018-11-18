#ifndef PIPELINESTATE_H
#define PIPELINESTATE_H


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

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\ThreadUtilities.h"

#include <atomic>
#include <condition_variable>

struct ID3D12PipelineStateObject;

using std::atomic_bool;
using std::condition_variable;
using std::mutex;

struct ID3D12Device;
struct ID3D12PipelineState;


namespace FlexKit
{
	class RenderSystem;
	class PipelineStateTable;


	typedef ID3D12PipelineState* LOADSTATE_FN(RenderSystem* RS);


	/************************************************************************************************/

	typedef Handle_t<32, GetTypeGUID(PSOHandle)> PSOHandle;

	class PipelineStateObject
	{
	public:
		enum class PSO_States
		{
			LoadInProgress,
			LoadQueued,
			Loaded,
			Failed,
			Stale,
			Unloaded,
			Undefined
		};

		bool changeState(const PipelineStateObject::PSO_States newState)
		{
			auto currentState = state.load(std::memory_order_acquire);

			if (currentState == PipelineStateObject::PSO_States::Unloaded)
			{
				if (state.compare_exchange_strong(currentState, newState, std::memory_order_release))
				{
					return true;
				}
			}

			return false;
		}


		ID3D12PipelineState*				PSO		= nullptr;
		PSOHandle							id		= InvalidHandle_t;
		std::atomic<PSO_States>				state	= PSO_States::Unloaded;
		std::atomic <PipelineStateObject*>	next	= nullptr;
		LOADSTATE_FN*						loader	= nullptr;
		bool								stale	= false;
		std::condition_variable				CV;
	};


	/************************************************************************************************/


	class LoadTask : public iWork
	{
	public:
		LoadTask(iAllocator* IN_allocator, PipelineStateTable* IN_PST, PipelineStateObject* IN_PSO);

		void Run()		override;
		void Release()	override;


		RenderSystem*			RS;
		PipelineStateObject*	PSO;
		iAllocator*				allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI PipelineStateTable
	{
	public:
		PipelineStateTable(iAllocator* allocator, RenderSystem* RS, ThreadManager* Threads);
		
		void ReleasePSOs();

		bool					QueuePSOLoad	( PSOHandle handle, iAllocator* Allocator );
		bool					ReloadLoadPSO	( PSOHandle handle );
		ID3D12PipelineState*	GetPSO			( PSOHandle handle );

		void					RegisterPSOLoader	( PSOHandle handle, LOADSTATE_FN Loader );

	private:
		PipelineStateObject*	_GetStateObject			(PSOHandle				handle);
		PipelineStateObject*	_GetNearestStateObject	(PSOHandle				handle);
		bool					_AddStateObject			(PipelineStateObject*	PSO);

		static_vector<PipelineStateObject,	1024>			States;

		ThreadManager*										WorkQueue;
		ID3D12Device*										Device;
		RenderSystem*										RS;
		iAllocator*											allocator;

		friend LoadTask;
	};


}	/************************************************************************************************/
#endif