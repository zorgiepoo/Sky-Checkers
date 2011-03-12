/*
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
 
