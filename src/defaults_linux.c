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
 
 #include "defaults.h"
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <ctype.h>
 #include <limits.h>
 
 FILE *getUserDataFile(const char *mode)
 {
	char dataDirectory[PATH_MAX + 1] = {0};

	char *xdgConfigHome = getenv("XDG_CONFIG_HOME");
	if (xdgConfigHome != NULL)
	{
		int configSuccess = mkdir(xdgConfigHome, 0777);
		if (configSuccess != 0 && errno != EEXIST)
		{
			return NULL;
		}
		snprintf(dataDirectory, sizeof(dataDirectory) - 1, "%s/skycheckers", xdgConfigHome);
	}
	else
	{
		char *homeEnv = getenv("HOME");
		if (homeEnv == NULL)
		{
			return NULL;
		}
		
		char configDirectory[PATH_MAX + 1] = {0};
		snprintf(configDirectory, sizeof(configDirectory) - 1, "%s/.config", homeEnv);

		int configSuccess = mkdir(configDirectory, 0777);
		if (configSuccess != 0 && errno != EEXIST)
		{
			return NULL;
		}

		snprintf(dataDirectory, sizeof(dataDirectory) - 1, "%s/skycheckers", configDirectory);
	}
	
	fprintf(stderr, "user data dir: %s\n", dataDirectory);
	int success = mkdir(dataDirectory, 0777);
	if (success == 0 || errno == EEXIST)
	{
		strncat(dataDirectory, "/user_data.txt", sizeof(dataDirectory) - 1 - strlen(dataDirectory));
		return fopen(dataDirectory, mode);
	}
	return NULL;
 }
