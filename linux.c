/*
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
 
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */
 
 #include "linux.h"
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <ctype.h>
 
 FILE *getUserDataFile(const char *mode)
 {
	FILE *file = NULL;
	char dataDirectory[256];
	strcpy(dataDirectory, getenv("HOME"));
	strcat(dataDirectory, "/.skycheckers/");
	
	int success = mkdir(dataDirectory, 0777);

	if (success == 0 || errno == EEXIST)
	{
		strcat(dataDirectory, "user_data.txt");
		file = fopen(dataDirectory, mode);
	}

	return file;
 }

 void getDefaultUserName(char *defaultUserName, int maxLength)
 {
	strncpy(defaultUserName, getenv("USER"), maxLength);
	defaultUserName[0] = toupper(defaultUserName[0]);
 }
 
