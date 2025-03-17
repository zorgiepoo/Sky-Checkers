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

#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define SPIN_COUNT 2000

ZGThread ZGCreateThread(ZGThreadFunction function, const char *name, void *data)
{
	HANDLE threadHandle = CreateThread(NULL, 0, function, data, 0, NULL);
	if (threadHandle == NULL)
	{
		fprintf(stderr, "Error: Failed to create thread: %d\n", GetLastError());
	}
	return threadHandle;
}

void ZGWaitThread(ZGThread thread)
{
	if (WaitForSingleObjectEx(thread, INFINITE, FALSE) == WAIT_FAILED)
	{
		fprintf(stderr, "Error: Failed to WaitForSingleObjectEx()\n");
	}
	
	if (!CloseHandle(thread))
	{
		fprintf(stderr, "Error: Failed to CloseHandle()\n");
	}
}

ZGMutex ZGCreateMutex(void)
{
	CRITICAL_SECTION* mutex = calloc(1, sizeof(*mutex));
	InitializeCriticalSectionAndSpinCount(mutex, SPIN_COUNT);
	return mutex;
}

void ZGLockMutex(ZGMutex mutex)
{
	EnterCriticalSection(mutex);
}

void ZGUnlockMutex(ZGMutex mutex)
{
	LeaveCriticalSection(mutex);
}

void ZGDelay(uint32_t delayMilliseconds)
{
	Sleep(delayMilliseconds);
}
