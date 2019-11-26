//
//  thread_sdl.c
//  SkyCheckers
//
//  Created by Mayur Pawashe on 10/20/19.
//

#include "thread.h"
#include "maincore.h"
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
