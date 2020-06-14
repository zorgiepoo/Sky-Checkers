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
