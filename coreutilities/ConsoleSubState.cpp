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


namespace FlexKit
{	/************************************************************************************************/


	bool  ConsoleSubState::Update(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		return !pauseBackgroundLogic;
	}


	/************************************************************************************************/


	bool  ConsoleSubState::DebugDraw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT)
	{
		return false;
	}

	/************************************************************************************************/


	bool  ConsoleSubState::Draw(EngineCore* Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& graph)
	{
		console->Draw(graph, GetCurrentBackBuffer(&core->Window), Engine->GetTempMemory());

		return false;
	}


	/************************************************************************************************/


	bool  ConsoleSubState::EventHandler(Event evt)
	{
		if (evt.InputSource == Event::Keyboard)
		{
			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_TILDA: {
					PopSubState(framework);
					return false;
				}	break;
				case KC_BACKSPACE:
					framework->console.BackSpace();
					break;
				case KC_ENTER:
				{
					recallIndex = 0;
					framework->console.EnterLine(framework->core->GetTempMemory());
				}	break;
				case KC_ARROWUP:
				{
					if(console->commandHistory.size()){
						auto line	  = console->commandHistory[recallIndex].Str;
						auto LineSize = strlen(line);

						strcpy_s(console->inputBuffer, console->commandHistory[recallIndex]);

						console->inputBufferSize = LineSize;
						IncrementRecallIndex();
					}
				}	break;
				case KC_ARROWDOWN:
				{
					if (console->commandHistory.size()) {
						auto line		= console->commandHistory[recallIndex].Str;
						auto lineSize	= strlen(line);

						strcpy_s(console->inputBuffer, console->commandHistory[recallIndex]);

						console->inputBufferSize = lineSize;
						DecrementRecallIndex();
					}
				}	break;
				case KC_SPACE:
				{
					framework->console.Input(' ');
				}	break;
				default:
				{
					if ((evt.mData1.mKC[0] >= KC_A && evt.mData1.mKC[0] <= KC_Z ) || 
						(evt.mData1.mKC[0] >= KC_0 && evt.mData1.mKC[0] <= KC_9)  ||
						(evt.mData1.mKC[0] >= KC_SYMBOLS_BEGIN && evt.mData1.mKC[0] <= KC_SYMBOLS_END ) || 
						(evt.mData1.mKC[0] == KC_PLUS) || (evt.mData1.mKC[0] == KC_MINUS) ||
						(evt.mData1.mKC[0] == KC_UNDERSCORE) || (evt.mData1.mKC[0] == KC_EQUAL) ||
						(evt.mData1.mKC[0] == KC_SYMBOL ))
						framework->console.Input((char)evt.mData2.mINT[0]);

				}	break;
				}
			}	break;
			}
		}
		return false;
	}


	/************************************************************************************************/


	void ConsoleSubState::IncrementRecallIndex()
	{
		recallIndex = (recallIndex + 1) % console->commandHistory.size();
	}


	/************************************************************************************************/


	void ConsoleSubState::DecrementRecallIndex()
	{
		recallIndex = (console->commandHistory.size() + recallIndex - 1) % console->commandHistory.size();
	}


	/************************************************************************************************/

} // namespace FlexKit;