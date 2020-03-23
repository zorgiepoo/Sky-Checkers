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

#pragma once

#include <stdio.h>
#include <stdbool.h>

#include "platforms.h"

typedef struct
{
#if !PLATFORM_APPLE
	FILE *file;
#endif
} Defaults;

Defaults userDefaultsForReading(void);
Defaults userDefaultsForWriting(void);

void closeDefaults(Defaults defaults);

bool readDefaultKey(Defaults defaults, const char *key, char *valueBuffer, size_t maxValueSize);
int readDefaultIntKey(Defaults defaults, const char *key, int defaultValue);
bool readDefaultBoolKey(Defaults defaults, const char *key, bool defaultValue);

void writeDefaultIntKey(Defaults defaults, const char *key, int value);
void writeDefaultStringKey(Defaults defaults, const char *key, const char *value);

#if PLATFORM_OSX
void getDefaultUserName(char *defaultUserName, int maxLength);
#endif
