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
	static bool initializedTime;
	if (!initializedTime)
	{
		mach_timebase_info(&info);
		initialTime = (1.0 * mach_absolute_time() * info.numer / info.denom);
		
		initializedTime = true;
	}
	
	return (uint64_t)((1.0 * mach_absolute_time() * info.numer / info.denom) - initialTime);
}
