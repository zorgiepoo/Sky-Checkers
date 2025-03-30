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

#include "window.h"
#include "zgtime.h"
#include "quit.h"

#include <Windows.h>
#include <Dbt.h>
#include <ShellScalingApi.h>

#if __has_include("resource.h")
#define WIN_ICON_AVAILABLE 1
#include "resource.h"
#else
#define WIN_ICON_AVAILABLE 0
#endif

#define SC_WINDOW_CLASS_NAME "SC_WINDOW_CLASS"

static bool ZGGetWindowSizeForHandle(HWND handle, int32_t* width, int32_t* height);

typedef struct
{
	HDEVNOTIFY deviceNotificationHandle;

	void (*windowEventHandler)(ZGWindowEvent, void*);
	void* windowEventHandlerContext;

	void (*keyboardEventHandler)(ZGKeyboardEvent, void*);
	void* keyboardEventHandlerContext;

	int32_t minWidth;
	int32_t minHeight;

	bool minimized;
} WindowContext;

static float ZGScaleFactorForDpi(UINT dpi)
{
	return (float)dpi / USER_DEFAULT_SCREEN_DPI;
}

LRESULT CALLBACK windowCallback(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handledMessage = false;
	switch (message)
	{
	case WM_DESTROY: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
		if (windowContext != NULL && windowContext->deviceNotificationHandle != NULL)
		{
			UnregisterDeviceNotification(windowContext->deviceNotificationHandle);
		}
		ZGSendQuitEvent();
		handledMessage = true;
	} break;
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
				int32_t windowWidth;
				int32_t windowHeight;
				if (ZGGetWindowSizeForHandle(handle, &windowWidth, &windowHeight))
				{
					ZGWindowEvent windowEvent = { 0 };
					windowEvent.width = windowWidth;
					windowEvent.height = windowHeight;
					windowEvent.type = ZGWindowEventTypeResize;

					windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);

					handledMessage = true;
				}
			}
		}
	} break;
	case WM_DPICHANGED: {
		RECT* const newWindowRect = (RECT*)lParam;
		if (SetWindowPos(handle, NULL, newWindowRect->left, newWindowRect->top, newWindowRect->right - newWindowRect->left, newWindowRect->bottom - newWindowRect->top, SWP_NOZORDER | SWP_NOACTIVATE))
		{
			WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
			if (windowContext != NULL && windowContext->windowEventHandler != NULL)
			{
				UINT dpi = HIWORD(wParam);
				float scaleFactor = ZGScaleFactorForDpi(dpi);

				ZGWindowEvent windowEvent = { 0 };
				windowEvent.width = (int32_t)((newWindowRect->right - newWindowRect->left) / scaleFactor);
				windowEvent.height = (int32_t)((newWindowRect->bottom - newWindowRect->top) / scaleFactor);
				windowEvent.type = ZGWindowEventTypeResize;

				windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);

				handledMessage = true;
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
		if (ZGTestReturnKeyCode((uint16_t)wParam))
		{
			handledMessage = true;
		}
		break;
	case WM_KEYUP:
	case WM_KEYDOWN: {
		WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
		if (windowContext != NULL && windowContext->keyboardEventHandler != NULL)
		{
			bool downEvent = (message == WM_KEYDOWN);

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
	case WM_DEVICECHANGE: {
		// Don't care about removals (DBT_DEVICEREMOVECOMPLETE)
		if (wParam == DBT_DEVICEARRIVAL)
		{
			WindowContext* windowContext = (WindowContext*)GetWindowLongPtr(handle, GWLP_USERDATA);
			if (windowContext != NULL && windowContext->windowEventHandler != NULL)
			{
				ZGWindowEvent windowEvent = { 0 };
				windowEvent.type = ZGWindowEventDeviceConnected;

				windowContext->windowEventHandler(windowEvent, windowContext->windowEventHandlerContext);
			}
		}
	} break;
	}

	return handledMessage ? 0 : DefWindowProc(handle, message, wParam, lParam);
}

ZGWindow* ZGCreateWindow(const char* windowTitle, int32_t windowWidth, int32_t windowHeight, bool* fullscreenFlag)
{
	// This may not be necessary and fail if it's already set, but making the call in case
	// The DPI awareness should primarily be set in the project's manifest settings
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HINSTANCE appInstance = GetModuleHandle(NULL);

	static WNDCLASSEX windowClass;
	static bool createdWindowClass;
	if (!createdWindowClass)
	{
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpszClassName = SC_WINDOW_CLASS_NAME;
		windowClass.hInstance = appInstance;
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
#if WIN_ICON_AVAILABLE
		windowClass.hIcon = LoadIconA(appInstance, MAKEINTRESOURCE(IDI_ICON1));
#endif
		windowClass.lpfnWndProc = windowCallback;

		if (!RegisterClassEx(&windowClass))
		{
			fprintf(stderr, "Error: Failed to register window class %s\n", SC_WINDOW_CLASS_NAME);
			return NULL;
		}

		createdWindowClass = true;
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT screenRect = { 0 };
	screenRect.right = screenWidth;
	screenRect.bottom = screenHeight;
	HMONITOR monitor = MonitorFromRect(&screenRect, MONITOR_DEFAULTTONEAREST);

	UINT monitorDpi;
	UINT unusedDpi;
	HRESULT monitorDpiResult = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &monitorDpi, &unusedDpi);
	if (FAILED(monitorDpiResult))
	{
		monitorDpi = USER_DEFAULT_SCREEN_DPI;
	}

	DWORD windowStyle = WS_OVERLAPPEDWINDOW;

	float scaleFactor = ZGScaleFactorForDpi(monitorDpi);

	RECT windowRect = { 0 };
	windowRect.right = (LONG)(windowWidth * scaleFactor);
	windowRect.bottom = (LONG)(windowHeight * scaleFactor);
	// Not sure if it makes any difference to use AdjustWindowRect vs AdjustWindowRectExForDpi when using same dpi as monitor
	if (!AdjustWindowRectExForDpi(&windowRect, windowStyle, FALSE, 0, monitorDpi))
	{
		fprintf(stderr, "Error: failed to AdjustWindowRectExForDpi(): %d\n", GetLastError());
		return NULL;
	}

	int adjustedWindowWidth = windowRect.right - windowRect.left;
	int adjustedWindowHeight = windowRect.bottom - windowRect.top;

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

	// Register notification for gamepad devices being added
	DEV_BROADCAST_DEVICEINTERFACE deviceBroadcastInterface = { 0 };
	deviceBroadcastInterface.dbcc_size = sizeof(deviceBroadcastInterface);
	deviceBroadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	// See class HID guid at https://docs.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-hid
	deviceBroadcastInterface.dbcc_classguid = (GUID){ 0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 };

	context->deviceNotificationHandle = RegisterDeviceNotification(handle, &deviceBroadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (context->deviceNotificationHandle == NULL)
	{
		fprintf(stderr, "Error: failed to RegisterDeviceNotification(): %d\n", GetLastError());
	}

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

	UINT windowDpi = GetDpiForWindow(handle);
	float scalingFactor = ZGScaleFactorForDpi(windowDpi);

	windowContext->minWidth = (uint32_t)(minWidth * scalingFactor);
	windowContext->minHeight = (uint32_t)(minHeight * scalingFactor);
}

void ZGGetDrawableSize(ZGWindow* windowRef, int32_t* width, int32_t* height)
{
	HWND handle = windowRef;
	RECT clientRect;
	if (GetClientRect(handle, &clientRect))
	{
		int32_t drawableWidth = (int32_t)clientRect.right;
		int32_t drawableHeight = (int32_t)clientRect.bottom;

		*width = drawableWidth;
		*height = drawableHeight;
	}
	else
	{
		fprintf(stderr, "Error: failed to get window drawable size: %d\n", GetLastError());
	}
}

static bool ZGGetWindowSizeForHandle(HWND handle, int32_t* width, int32_t* height)
{
	RECT clientRect;
	if (GetClientRect(handle, &clientRect))
	{
		int32_t drawableWidth = (int32_t)clientRect.right;
		int32_t drawableHeight = (int32_t)clientRect.bottom;

		UINT windowDpi = GetDpiForWindow(handle);
		float scalingFactor = ZGScaleFactorForDpi(windowDpi);

		*width = (int32_t)(drawableWidth / scalingFactor);
		*height = (int32_t)(drawableHeight / scalingFactor);

		return true;
	}
	else
	{
		fprintf(stderr, "Error: failed to get window size: %d\n", GetLastError());

		return false;
	}
}

void ZGGetWindowSize(ZGWindow* windowRef, int32_t* width, int32_t* height)
{
	HWND handle = windowRef;
	ZGGetWindowSizeForHandle(handle, width, height);
}

void* ZGWindowHandle(ZGWindow* windowRef)
{
	HWND handle = windowRef;
	return handle;
}
