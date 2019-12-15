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

#include "time.h"
#import <Foundation/Foundation.h>
#include <mach/mach_time.h>

uint32_t ZGGetTicks(void)
{
	static mach_timebase_info_data_t info;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		mach_timebase_info(&info);
	});
	
	return (uint32_t)((1.0 * mach_absolute_time() * info.numer / info.denom) / 1000000.0);
}