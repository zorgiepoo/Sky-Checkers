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

#include "app.h"
#include "sdl_include.h"
#include "quit.h"
#include "platforms.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

int ZGAppInit(int argc, char *argv[], ZGAppHandlers *appHandlers, void *appContext)
{
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		ZGQuit();
	}

#if PLATFORM_LINUX && !_DEBUG
	char *basePath;
	if (argc > 2 && strcmp(argv[1], "--base") == 0)
	{
		basePath = argv[2];
	}
	else
	{
		basePath = "/usr/share/skycheckers/";
	}

	if (chdir(basePath) != 0)
	{
		fprintf(stderr, "Failed to chdir() to %s: %d\n", basePath, errno);
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
