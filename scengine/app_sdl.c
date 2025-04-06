/*
 MIT License

 Copyright (c) 2019 Mayur Pawashe

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

#include <SDL2/SDL.h>

#include "app.h"
#include "quit.h"

#include <stdio.h>
#include <stdbool.h>

#if PLATFORM_LINUX
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#endif

int ZGAppInit(int argc, char *argv[], ZGAppHandlers *appHandlers, void *appContext)
{
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		ZGQuit();
	}

#if PLATFORM_LINUX && !_DEBUG
	char basePath[PATH_MAX + 1] = {0};
	if (argc > 2 && strcmp(argv[1], "--base") == 0)
	{
		strncpy(basePath, argv[2], sizeof(basePath) - 1);
	}
	else
	{
		// This approach would need to be adjusted if skycheckers is ported to other Unix systems
		char executablePath[PATH_MAX + 1] = {0};
		if (readlink("/proc/self/exe", executablePath, sizeof(executablePath) - 1) == -1)
		{
			fprintf(stderr, "Error: Failed to read /proc/self/exe: %d.. Assuming default path\n", errno);
			strncpy(basePath, "/usr/share/skycheckers", sizeof(basePath) - 1);
		}
		else
		{
			// Note this may modify executablePath
			char *parentPath = dirname(executablePath);
			
#if _FLATPAK_BUILD
			snprintf(basePath, sizeof(basePath) - 1, "%s/../extra/sc-snap-pak-data", parentPath);
#else
			snprintf(basePath, sizeof(basePath) - 1, "%s/../share/skycheckers", parentPath);
#endif
		}
	}

	if (chdir(basePath) != 0)
	{
		fprintf(stderr, "Failed to chdir() to %s: %d\nFor development builds use: make scdev\n", basePath, errno);
		ZGQuit();
	}
#endif
	
	if (appHandlers->launchedHandler != NULL)
	{
		appHandlers->launchedHandler(appContext);
	}
	
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					done = true;
					break;
				default:
					if (appHandlers->pollEventHandler != NULL)
					{
						appHandlers->pollEventHandler(appContext, &event);
					}
			}
		}
		
		if (appHandlers->runLoopHandler != NULL)
		{
			appHandlers->runLoopHandler(appContext);
		}
	}
	
	if (appHandlers->terminatedHandler != NULL)
	{
		appHandlers->terminatedHandler(appContext);
	}
	
	ZGQuit();
	return 0;
}

void ZGAppSetAllowsScreenIdling(bool allowsScreenIdling)
{
	if (!allowsScreenIdling)
	{
		if (SDL_IsScreenSaverEnabled())
		{
			SDL_DisableScreenSaver();
		}
	}
	else
	{
		if (!SDL_IsScreenSaverEnabled())
		{
			SDL_EnableScreenSaver();
		}
	}
}
