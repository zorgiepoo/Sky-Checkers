/*
* Copyright 2020 Mayur Pawashe
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern FILE *getUserDataFile(const char *mode);

Defaults userDefaultsForReading(void)
{
	FILE *file = getUserDataFile("rb");
	return (Defaults){.file = file};
}

Defaults userDefaultsForWriting(void)
{
	FILE *file = getUserDataFile("wb");
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
