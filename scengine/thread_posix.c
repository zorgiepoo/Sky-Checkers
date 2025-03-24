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
	
#if PLATFORM_APPLE
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
#if PLATFORM_APPLE
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
