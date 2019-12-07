/*
* Copyright 2019 Mayur Pawashe
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

#if defined(__APPLE__) && defined(__MACH__)
#import <TargetConditionals.h>
#define PLATFORM_APPLE 1
#define PLATFORM_OSX TARGET_OS_OSX
#define PLATFORM_IOS TARGET_OS_IPHONE
#define PLATFORM_IOS_SIMULATOR TARGET_OS_SIMULATOR
#else
#define PLATFORM_APPLE 0
#define PLATFORM_OSX 0
#define PLATFORM_IOS 0
#define PLATFORM_IOS_SIMULATOR 0
#endif

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#else
#define PLATFORM_WINDOWS 0
#endif

#if defined(linux)
#define PLATFORM_LINUX 1
#else
#define PLATFORM_LINUX 0
#endif
