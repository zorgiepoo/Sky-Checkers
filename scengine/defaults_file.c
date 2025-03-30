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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern FILE *getUserDataFile(const char *defaultsName, const char *mode);

Defaults userDefaultsForReading(const char* defaultsName)
{
	FILE *file = getUserDataFile(defaultsName, "rb");
	return (Defaults){.file = file};
}

Defaults userDefaultsForWriting(const char* defaultsName)
{
	FILE *file = getUserDataFile(defaultsName, "wb");
	return (Defaults){.file = file};
}

void closeDefaults(Defaults defaults)
{
	FILE *file = defaults.file;
	if (file != NULL)
	{
		fclose(file);
	}
}

static bool readDefaultLine(FILE *file, char *lineBuffer, size_t maxLineSize)
{
	memset(lineBuffer, 0, maxLineSize);
	
	size_t bufferIndex = 0;
	while (true)
	{
		int character = fgetc(file);
		if (ferror(file) != 0)
		{
			return false;
		}
		
		if (character == '\n' || character == '\r' || character == EOF)
		{
			break;
		}
		else if (bufferIndex < maxLineSize - 1)
		{
			lineBuffer[bufferIndex] = character;
			bufferIndex++;
		}
		else
		{
			fprintf(stderr, "Line in defaults is too big...: %s\n", lineBuffer);
			return false;
		}
	}
	
	return true;
}

bool readDefaultKey(Defaults defaults, const char *key, char *valueBuffer, size_t maxValueSize)
{
	FILE *file = defaults.file;
	memset(valueBuffer, 0, maxValueSize);
	
	if (file == NULL)
	{
		return false;
	}
	
	long initialOffset = ftell(file);
	if (initialOffset == -1)
	{
		return false;
	}
	
	char lineBuffer[512];
	bool hitEOFOnce = false;
	while (true)
	{
		if (!readDefaultLine(file, lineBuffer, sizeof(lineBuffer)))
		{
			return false;
		}
		
		char *endPointer = strstr(lineBuffer, ": ");
		if (endPointer != NULL && strncmp(lineBuffer, key, strlen(key)) == 0)
		{
			strncpy(valueBuffer, endPointer + 2, maxValueSize - 1);
			return true;
		}
		
		if (feof(file) != 0)
		{
			if (hitEOFOnce)
			{
				return false;
			}
			else
			{
				hitEOFOnce = true;
				if (fseek(file, 0, SEEK_SET) == -1)
				{
					return false;
				}
			}
		}
		
		if (hitEOFOnce)
		{
			long currentOffset = ftell(file);
			if (currentOffset == -1)
			{
				return false;
			}
			
			if (currentOffset >= initialOffset)
			{
				return false;
			}
		}
	}
	
	return false;
}

int readDefaultIntKey(Defaults defaults, const char *key, int defaultValue)
{
	char valueBuffer[512];
	if (readDefaultKey(defaults, key, valueBuffer, sizeof(valueBuffer)))
	{
		return atoi(valueBuffer);
	}
	return defaultValue;
}

bool readDefaultBoolKey(Defaults defaults, const char *key, bool defaultValue)
{
	int value = readDefaultIntKey(defaults, key, defaultValue ? 1 : 0);
	return (value != 0);
}

void writeDefaultIntKey(Defaults defaults, const char *key, int value)
{
	FILE *file = defaults.file;
	fprintf(file, "%s: %d\n", key, value);
}

void writeDefaultStringKey(Defaults defaults, const char *key, const char *value)
{
	FILE *file = defaults.file;
	fprintf(file, "%s: %s\n", key, value);
}
