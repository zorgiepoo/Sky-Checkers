/*
 MIT License

 Copyright (c) 2010 Mayur Pawashe

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
 
 #include "defaults.h"
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <ctype.h>
 #include <limits.h>
 
 FILE *getUserDataFile(const char *defaultsName, const char *mode)
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
		snprintf(dataDirectory, sizeof(dataDirectory) - 1, "%s/%s", xdgConfigHome, defaultsName);
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

		snprintf(dataDirectory, sizeof(dataDirectory) - 1, "%s/%s", configDirectory, defaultsName);
	}
	
	int success = mkdir(dataDirectory, 0777);
	if (success == 0 || errno == EEXIST)
	{
		strncat(dataDirectory, "/user_data.txt", sizeof(dataDirectory) - 1 - strlen(dataDirectory));
		return fopen(dataDirectory, mode);
	}
	return NULL;
 }
