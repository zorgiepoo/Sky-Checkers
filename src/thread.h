/*
* Copyright 2019 Mayur Pawashe
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

#include <stdint.h>

typedef void* ZGThread;
typedef int (*ZGThreadFunction)(void *data);

typedef void* ZGMutex;

ZGThread ZGCreateThread(ZGThreadFunction function, const char *name, void *data);
void ZGWaitThread(ZGThread thread);

// ZGDestroyMutex() is not provided because we don't need to destroy them in our game
ZGMutex ZGCreateMutex(void);
void ZGLockMutex(ZGMutex mutex);
void ZGUnlockMutex(ZGMutex mutex);

void ZGDelay(uint32_t delayMilliseconds);
