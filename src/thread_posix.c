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

#include "thread.h"
#include "quit.h"
#include "platforms.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

typedef struct
{
	ZGThreadFunction function;
	void *data;
	char *name;
} ThreadDataWrapper;

static void *threadFunctionWrapper(void *context)
{
	ThreadDataWrapper *dataWrapper = context;
	
#ifdef MAC_OS_X
	if (dataWrapper->name != NULL)
	{
		pthread_setname_np(dataWrapper->name);
		free(dataWrapper->name);
		dataWrapper->name = NULL;
	}
#endif
	
	// ignore return value
	dataWrapper->function(dataWrapper->data);
	
	free(dataWrapper);
	
	return NULL;
}

ZGThread ZGCreateThread(ZGThreadFunction function, const char *name, void *data)
{
	pthread_t *thread = calloc(1, sizeof(*thread));
	assert(thread != NULL);
	
	ThreadDataWrapper *dataWrapper = calloc(1, sizeof(*dataWrapper));
	assert(dataWrapper != NULL);
	
	dataWrapper->function = function;
	dataWrapper->data = data;
#ifdef PTHREAD_SETNAME_AVAILABLE
	if (name != NULL)
	{
		dataWrapper->name = strdup(name);
	}
#endif
	
	int result = pthread_create(thread, NULL, threadFunctionWrapper, dataWrapper);
	if (result != 0)
	{
		fprintf(stderr, "Failed to create thread: %d - %s\n", result, strerror(result));
		free(thread);
		thread = NULL;
	}
	
	return thread;
}

void ZGWaitThread(ZGThread thread)
{
	int result = pthread_join(*(pthread_t *)thread, NULL);
	if (result != 0)
	{
		fprintf(stderr, "Failed to wait for thread %p: %d - %s\n", thread, result, strerror(result));
	}
	free(thread);
}

ZGMutex ZGCreateMutex(void)
{
	pthread_mutex_t *mutex = calloc(1, sizeof(*mutex));
	assert(mutex != NULL);
	
	int result = pthread_mutex_init(mutex, NULL);
	if (result != 0)
	{
		fprintf(stderr, "Failed to create mutex: %d - %s\n", result, strerror(result));
		ZGQuit();
	}
	
	return mutex;
}

void ZGLockMutex(ZGMutex mutex)
{
	int result = pthread_mutex_lock(mutex);
	assert(result == 0);
}

void ZGUnlockMutex(ZGMutex mutex)
{
	int result = pthread_mutex_unlock(mutex);
	assert(result == 0);
}

void ZGDelay(uint32_t delayMilliseconds)
{
	struct timespec delaySpec;
	delaySpec.tv_sec = delayMilliseconds / 1000;
	delaySpec.tv_nsec = (delayMilliseconds % 1000) * 1000000;
	int result;
	do
	{
		struct timespec elapsedSpec;
		result = nanosleep(&delaySpec, &elapsedSpec);
		delaySpec = elapsedSpec;
	}
	while (result != 0 && errno == EINTR);
}
