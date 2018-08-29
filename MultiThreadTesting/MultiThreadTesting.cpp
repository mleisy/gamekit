// MultiThreadTesting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>


#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\ThreadUtilities.h"
#include "..\coreutilities\ThreadUtilities.cpp"


#include <Windows.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>


std::condition_variable CV;
std::condition_variable WorkStartBarrier;
std::atomic_int64_t		WorkersAwake;
std::atomic_int64_t		x			= 0;
const size_t			threadCount	= 1;


void Test()
{
	std::mutex						M;
	std::unique_lock<std::mutex>	Lock(M);

	CV.wait(Lock);

	std::cout << "working! \n";

	for (
			size_t i = 0; 
			i < 100000u / threadCount; 
			++i
		) 
		++x;
}


/************************************************************************************************/

template<class TY>
using Deque = FlexKit::Deque_MT<TY>;


bool DeQue_Test()
{
	class TestClass
	{
	public:
		TestClass(int X) :
			x{ X + 1 } {}

		~TestClass()
		{
			std::cout << "Test Deleting!\n";
		}

		void print()
		{
			std::cout << x << "\n";
		}

		int x;
	};

	{
		using ElementType = Deque<TestClass>::Element_TY;

		ElementType N1(1);
		ElementType N2(2);
		ElementType N3(3);
		ElementType N4(4);

		Deque<TestClass> deque;

		// Test One
		{
			ElementType* E;
			auto res = deque.try_pop_front(E);
			if (res != false)
				return false;

			deque.push_back(N1);
			res = deque.try_pop_front(E);

			if (res != true)
				return false;
		}

		deque.push_front(N1);
		deque.push_front(N2);
		deque.push_front(N3);
		deque.push_front(N4);

		for (auto& I : deque)
			I.print();

		auto& Temp = deque.pop_front();

		return true;
	}
}


/************************************************************************************************/


bool Thread_Test()
{
	FlexKit::ThreadManager Manager(FlexKit::SystemAllocator, threadCount);
	FlexKit::WorkBarrier   WorkBarrier;

	for (auto I = 0; I < threadCount; ++I) {
		auto Time = (rand() % 5) * 1000;
		auto& Work = FlexKit::CreateLambdaWork_New(
			[Time]()
		{
			++WorkersAwake;
			WorkStartBarrier.notify_all();
			Test();
		});

		WorkBarrier.AddDependentWork(Work);
		Manager.AddWork(Work);
	}


	auto& Work = FlexKit::CreateLambdaWork_New(
		[]()
		{
			std::cout << "Sleeping for " << 1000 << " MS\n";
			Sleep(1000);
		});


	WorkBarrier.AddOnCompletionEvent([] { std::cout << "Work Done1\n"; });
	WorkBarrier.AddOnCompletionEvent([] { std::cout << "Work Done2\n"; });
	WorkBarrier.AddOnCompletionEvent([&WorkBarrier, &Work]
		{
			WorkBarrier.AddDependentWork(Work);
		});

	std::mutex M;
	std::unique_lock<std::mutex> Lock(M);

	WorkStartBarrier.wait(Lock, [] { return WorkersAwake == threadCount; });
	CV.notify_all();
	WorkBarrier.Wait();
	WorkBarrier.Wait();

	Manager.Release();
	std::cout << x << '\n';

	return true;
}


/************************************************************************************************/


int main()
{//
	if (!DeQue_Test())	return -1;
	if (!Thread_Test())	return -1;

    return 0;
}


/************************************************************************************************/