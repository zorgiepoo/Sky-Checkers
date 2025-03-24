/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "app.h"
#include "quit.h"
#include "zgtime.h"
#include "platforms.h"

#include <Windows.h>

#include <stdio.h>
#include <stdbool.h>

#define MAX_EVENTS 10

extern int main(int argc, char* argv[]);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR commandLine, int nCmdShow)
{
	return main(0, NULL);
}

int ZGAppInit(int argc, char* argv[], ZGAppHandlers* appHandlers, void* appContext)
{
	// Initialize COM for audio and font implementations
	HRESULT coInitializeResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult))
	{
		fprintf(stderr, "Error: Failed to CoInitializeEx() with error %d\n", coInitializeResult);
		return -1;
	}

	// Initialize time
	(void)ZGGetNanoTicks();

	appHandlers->launchedHandler(appContext);
	
	while (true)
	{
		int eventCount = 0;
		MSG message;
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);

			if (message.message == WM_QUIT)
			{
				goto APP_TERMINATION;
			}

			eventCount++;
			if (eventCount >= MAX_EVENTS)
			{
				break;
			}
		}

		appHandlers->pollEventHandler(appContext, NULL);
		appHandlers->runLoopHandler(appContext);
	}

APP_TERMINATION:
	appHandlers->terminatedHandler(appContext);

	return 0;
}

void ZGAppSetAllowsScreenIdling(bool allowsScreenIdling)
{
	if (allowsScreenIdling)
	{
		SetThreadExecutionState(ES_CONTINUOUS);
	}
	else
	{
		SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
	}
}
