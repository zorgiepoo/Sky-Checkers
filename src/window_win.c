/*
* Copyright 2020 Mayur Pawashe
* https://zgcoder.net

* This file is part of skycheckers.
* skycheckers is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* skycheckers is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "window.h"
#include "zgtime.h"
#include "quit.h"

#include <Windows.h>
#include "resource.h"

#define SC_WINDOW_CLASS_NAME "SKYC_WINDOW_CLASS"

typedef struct
{
	void (*windowEventHandler)(ZGWindowEvent, void*);
	void* windowEventHandlerContext;

	void (*keyboardEventHandler)(ZGKeyboardEvent, void*);
	void* keyboardEventHandlerContext;

	int32_t minWidth;
	int32_t minHeight;

	bool minimized;
} WindowContext;

LRESULT CALLBACK windowCallback(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handledMessage = false;
	switch (message)
	{
	case WM_DESTROY:
		ZGSendQuitEvent();
		handledMessage = true;
		break;
	case WM_SIZE: {
		WindowContext* windowContext = (WindowContext *)GetWindowLongPtr(handle, GWLP_USERDATA);

		//if (windowContext == NULL || windowContext->windowEventHandler == NULL)
		//{
		//	fprintf(stderr, "Error: WM_SIZE: %d, %d\n", LOWORD(lParam), HIWORD(lParam));
		//}

		if (windowContext != NULL && windowContext->windowEventHandler != NULL)
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);

			// When the window is minimized, width and height may be zero
			// Ignore this event
			if (width > 0 && height > 0)
			{
				RECT clientRect;
				if (GetClientRect(handle, &clientRect))
				{
					ZGWindowEvent windowEvent = { 0 };
					windowEvent.width = (int32_t)clientRect.right;
					windowEvent.height = (int32_t)clientRect.bottom;
					windowEvent.type = ZGWindowEventTypeResize;

					windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);

					handledMessage = true;
				}
			}
		}
	} break;
	case WM_ACTIVATEAPP: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);

		if (windowContext != NULL && windowContext->windowEventHandler != NULL)
		{
			bool activated = !!wParam;

			ZGWindowEvent windowEvent = { 0 };
			windowEvent.type = activated ? ZGWindowEventTypeFocusGained : ZGWindowEventTypeFocusLost;

			windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);

			handledMessage = true;
		}
	} break;
	case WM_GETMINMAXINFO: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);

		if (windowContext != NULL && (windowContext->minWidth > 0 || windowContext->minHeight > 0))
		{
			MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;

			int systemMinWidth = GetSystemMetrics(SM_CXMINTRACK);
			int systemMinHeight = GetSystemMetrics(SM_CYMINTRACK);

			minMaxInfo->ptMinTrackSize.x = systemMinWidth + windowContext->minWidth;
			minMaxInfo->ptMinTrackSize.y = systemMinHeight + windowContext->minHeight;

			handledMessage = true;
		}
	} break;
	case WM_SYSCOMMAND: {
		if (wParam == SC_MINIMIZE)
		{
			WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
			if (windowContext != NULL)
			{
				windowContext->minimized = true;

				if (windowContext->windowEventHandler != NULL)
				{
					ZGWindowEvent windowEvent = { 0 };
					windowEvent.type = ZGWindowEventTypeHidden;

					windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);
				}
			}
		}
		else if (wParam == SC_RESTORE)
		{
			WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
			if (windowContext->minimized)
			{
				if (windowContext->windowEventHandler != NULL)
				{
					ZGWindowEvent windowEvent = { 0 };
					windowEvent.type = ZGWindowEventTypeShown;

					windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);
				}

				windowContext->minimized = false;
			}
		}
	} break;
	case WM_SYSCHAR:
		if (wParam == VK_RETURN)
		{
			handledMessage = true;
		}
		break;
	case WM_KEYUP:
	case WM_KEYDOWN: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
		if (windowContext != NULL && windowContext->keyboardEventHandler != NULL)
		{
			bool downEvent = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

			// Control is the only modifier we care about
			SHORT controlModifierStatus = GetKeyState(VK_CONTROL);
			bool controlHeldDown = (controlModifierStatus & 0x8000) != 0;

			ZGKeyboardEvent keyboardEvent = { 0 };
			keyboardEvent.keyCode = (uint16_t)wParam;
			keyboardEvent.keyModifier = controlHeldDown ? VK_CONTROL : 0;
			keyboardEvent.timestamp = ZGGetNanoTicks();
			keyboardEvent.type = downEvent ? ZGKeyboardEventTypeKeyDown : ZGKeyboardEventTypeKeyUp;

			windowContext->keyboardEventHandler(keyboardEvent, windowContext->keyboardEventHandlerContext);

			handledMessage = true;
		}
	} break;
	case WM_CHAR: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
		if (windowContext != NULL && windowContext->keyboardEventHandler != NULL)
		{
			ZGKeyboardEvent keyboardEvent = { 0 };
			keyboardEvent.text[0] = (char)wParam;
			keyboardEvent.timestamp = ZGGetNanoTicks();
			keyboardEvent.type = ZGKeyboardEventTypeTextInput;

			windowContext->keyboardEventHandler(keyboardEvent, windowContext->keyboardEventHandlerContext);
		}
	} break;
	case WM_SETCURSOR: {
		if (LOWORD(lParam) == HTCLIENT)
		{
			// Hide the cursor for our window
			SetCursor(NULL);
			return TRUE;
		}
	} break;
	}

	return handledMessage ? 0 : DefWindowProc(handle, message, wParam, lParam);
}

ZGWindow* ZGCreateWindow(const char* windowTitle, int32_t windowWidth, int32_t windowHeight, bool* fullscreenFlag)
{
	HINSTANCE appInstance = GetModuleHandle(NULL);

	static WNDCLASSEX windowClass;
	static bool createdWindowClass;
	if (!createdWindowClass)
	{
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpszClassName = SC_WINDOW_CLASS_NAME;
		windowClass.hInstance = appInstance;
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.hIcon = LoadIconA(appInstance, MAKEINTRESOURCE(IDI_ICON1));
		windowClass.lpfnWndProc = windowCallback;

		if (!RegisterClassEx(&windowClass))
		{
			fprintf(stderr, "Error: Failed to register window class %s\n", SC_WINDOW_CLASS_NAME);
			return NULL;
		}

		createdWindowClass = true;
	}

	DWORD windowStyle = WS_OVERLAPPEDWINDOW;

	RECT windowRect = { 0 };
	windowRect.right = windowWidth;
	windowRect.bottom = windowHeight;
	if (!AdjustWindowRectEx(&windowRect, windowStyle, FALSE, 0))
	{
		fprintf(stderr, "Error: failed toAdjustWindowRect(): %d\n", GetLastError());
		return NULL;
	}

	int adjustedWindowWidth = windowRect.right - windowRect.left;
	int adjustedWindowHeight = windowRect.bottom - windowRect.top;

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	int centerX = (int)((float)screenWidth / 2.0f - (float)adjustedWindowWidth / 2.0f);
	int centerY = (int)((float)screenHeight / 2.0f - (float)adjustedWindowHeight / 2.0f);
	
	HWND handle = CreateWindowA(SC_WINDOW_CLASS_NAME, windowTitle, windowStyle, centerX, centerY, adjustedWindowWidth, adjustedWindowHeight, NULL, NULL, appInstance, NULL);
	if (handle == NULL)
	{
		fprintf(stderr, "Error: Failed to CreateWindow()\n");
		return NULL;
	}

	WindowContext* context = calloc(1, sizeof(*context));
	SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)context);

	return handle;
}

void ZGDestroyWindow(ZGWindow* windowRef)
{
	HWND handle = windowRef;

	WindowContext*windowContext = (WindowContext *)GetWindowLongPtr(handle, GWLP_USERDATA);
	free(windowContext);

	DestroyWindow(handle);
}

void ZGShowWindow(ZGWindow* windowRef)
{
	HWND handle = windowRef;
	ShowWindow(handle, SW_SHOW);
}

bool ZGWindowHasFocus(ZGWindow* windowRef)
{
	HWND handle = windowRef;

	return (handle == GetFocus());
}

void ZGSetWindowEventHandler(ZGWindow* windowRef, void* context, void (*windowEventHandler)(ZGWindowEvent, void*))
{
	HWND handle = windowRef;
	WindowContext* windowContext = (WindowContext *)GetWindowLongPtr(handle, GWLP_USERDATA);

	windowContext->windowEventHandler = windowEventHandler;
	windowContext->windowEventHandlerContext = context;
}

void ZGSetKeyboardEventHandler(ZGWindow* windowRef, void* context, void (*keyboardEventHandler)(ZGKeyboardEvent, void*))
{
	HWND handle = windowRef;
	WindowContext* windowContext = (WindowContext *)GetWindowLongPtr(handle, GWLP_USERDATA);

	windowContext->keyboardEventHandler = keyboardEventHandler;
	windowContext->keyboardEventHandlerContext = context;
}

void ZGSetWindowMinimumSize(ZGWindow* windowRef, int32_t minWidth, int32_t minHeight)
{
	HWND handle = windowRef;
	WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);

	windowContext->minWidth = minWidth;
	windowContext->minHeight = minHeight;
}

void ZGGetWindowSize(ZGWindow* windowRef, int32_t* width, int32_t* height)
{
	HWND handle = windowRef;
	RECT clientRect;
	if (GetClientRect(handle, &clientRect))
	{
		*width = (int32_t)clientRect.right;
		*height = (int32_t)clientRect.bottom;
	}
	else
	{
		fprintf(stderr, "Error: failed to get window size: %d\n", GetLastError());
	}
}

void* ZGWindowHandle(ZGWindow* windowRef)
{
	HWND handle = windowRef;
	return handle;
}
