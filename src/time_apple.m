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

#include "zgtime.h"
#import <Foundation/Foundation.h>
#include <mach/mach_time.h>

uint32_t ZGGetTicks(void)
{
	return (uint32_t)(ZGGetNanoTicks() / 1000000.0);
}

uint64_t ZGGetNanoTicks(void)
{
	static mach_timebase_info_data_t info;
	static double initialTime = 0.0;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		mach_timebase_info(&info);
		initialTime = (1.0 * mach_absolute_time() * info.numer / info.denom);
	});
	
	return (uint64_t)((1.0 * mach_absolute_time() * info.numer / info.denom) - initialTime);
}
