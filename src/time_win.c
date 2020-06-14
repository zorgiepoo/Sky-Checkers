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

#include "zgtime.h"

#include <windows.h>
#include <stdbool.h>

static uint64_t getMicroTicks(void)
{
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER initialTime;
	if (frequency.QuadPart == 0)
	{
		// These calls can't fail on XP or later
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&initialTime);
	}
	
	LARGE_INTEGER performanceCounter;
	QueryPerformanceCounter(&performanceCounter);

	LARGE_INTEGER elapsedTime;
	elapsedTime.QuadPart = ((performanceCounter.QuadPart - initialTime.QuadPart) * 1000000) / frequency.QuadPart;

	return (uint64_t)elapsedTime.QuadPart;
}

uint32_t ZGGetTicks(void)
{
	return (uint32_t)(getMicroTicks() / 1000.0);
}

uint64_t ZGGetNanoTicks(void)
{
	return getMicroTicks() * 1000;
}
