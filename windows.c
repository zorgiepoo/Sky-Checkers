/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */
 
 #include "windows.h"
 #include <shlwapi.h>
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
 
