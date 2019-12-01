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
#include <Shlwapi.h>
#include <ShlObj.h>
#include <windows.h>

FILE *getUserDataFile(const char *mode)
{
	FILE *file = NULL;
	
	TCHAR appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath)))
	{
		PathAppend(appDataPath, "\\SkyCheckers\\");
		
		int success = CreateDirectory(appDataPath, NULL);
		if (success == 0 || success == ERROR_ALREADY_EXISTS)
		{
			PathAppend(appDataPath, "user_data.txt");
			file = fopen(appDataPath, mode);
		}
	}
	
	return file;
}

void getDefaultUserName(char *defaultUserName, int maxLength)
{
	if (maxLength > 0)
	{
		defaultUserName[0] = '\0';
	}
}
