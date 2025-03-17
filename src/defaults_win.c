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

#include "defaults.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include <windows.h>

FILE *getUserDataFile(const char* defaultsName, const char *mode)
{
	FILE *file = NULL;
	
	TCHAR appDataPath[MAX_PATH] = { 0 };
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath)))
	{
		PathAppend(appDataPath, defaultsName);
		
		int success = CreateDirectory(appDataPath, NULL);
		if (success == 0 || success == ERROR_ALREADY_EXISTS)
		{
			const char userDataFilename[] = "user_data.txt";

			PathAppend(appDataPath, userDataFilename);
			file = fopen(appDataPath, mode);
		}
	}
	
	return file;
}
