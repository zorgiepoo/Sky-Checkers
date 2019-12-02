/*
 * Copyright 2010 Mayur Pawashe
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
#include "sdl_include.h"
#include <assert.h>

ZGThread ZGCreateThread(ZGThreadFunction function, const char *name, void *data)
{
	return SDL_CreateThread(function, name, data);
}

void ZGWaitThread(ZGThread thread)
{
	SDL_WaitThread(thread, NULL);
}

ZGMutex ZGCreateMutex(void)
{
	return SDL_CreateMutex();
}

void ZGLockMutex(ZGMutex mutex)
{
	int result = SDL_LockMutex(mutex);
	assert(result == 0);
}

void ZGUnlockMutex(ZGMutex mutex)
{
	int result = SDL_UnlockMutex(mutex);
	assert(result == 0);
}

void ZGDelay(uint32_t delayMilliseconds)
{
	SDL_Delay(delayMilliseconds);
}
