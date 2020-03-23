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

#import "defaults.h"
#import <Foundation/Foundation.h>

Defaults userDefaultsForReading(void)
{
	Defaults defaults = {};
	return defaults;
}

Defaults userDefaultsForWriting(void)
{
	Defaults defaults = {};
	return defaults;
}

void closeDefaults(Defaults defaults)
{
}

bool readDefaultKey(Defaults defaults, const char *key, char *valueBuffer, size_t maxValueSize)
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	NSString *valueString = [userDefaults stringForKey:[NSString stringWithUTF8String:key]];
	if (valueString == nil)
	{
		return false;
	}
	
	return (bool)[valueString getCString:valueBuffer maxLength:maxValueSize encoding:NSUTF8StringEncoding];
}

int readDefaultIntKey(Defaults defaults, const char *key, int defaultValue)
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	id object = [userDefaults objectForKey:[NSString stringWithUTF8String:key]];
	if (object != nil && [object isKindOfClass:[NSNumber class]])
	{
		return [object intValue];
	}
	
	return defaultValue;
}

bool readDefaultBoolKey(Defaults defaults, const char *key, bool defaultValue)
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	id object = [userDefaults objectForKey:[NSString stringWithUTF8String:key]];
	if (object != nil && [object isKindOfClass:[NSNumber class]])
	{
		return [object boolValue];
	}
	
	return defaultValue;
}

void writeDefaultIntKey(Defaults defaults, const char *key, int value)
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	[userDefaults setInteger:value forKey:[NSString stringWithUTF8String:key]];
}

void writeDefaultStringKey(Defaults defaults, const char *key, const char *value)
{
	NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
	[userDefaults setObject:[NSString stringWithUTF8String:value] forKey:[NSString stringWithUTF8String:key]];
}

#if PLATFORM_OSX
void getDefaultUserName(char *defaultUserName, int maxLength)
{
	NSString *fullUsername = NSFullUserName();
	if (fullUsername)
	{
		NSUInteger spaceIndex = [fullUsername rangeOfString:@" "].location;
		if (spaceIndex != NSNotFound)
		{
			strncpy(defaultUserName, [[fullUsername substringToIndex:spaceIndex] UTF8String], maxLength);
		}
		else
		{
			strncpy(defaultUserName, [fullUsername UTF8String], maxLength);
		}
	}
}
#endif
