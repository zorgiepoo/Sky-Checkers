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

#pragma once

#define USE_HALF_WHEN_POSSIBLE 0

#if defined(__METAL_VERSION__)

#if USE_HALF_WHEN_POSSIBLE
typedef half4 ZGFloat4;
typedef half2 ZGFloat2;
typedef half ZGFloat;
typedef matrix_half4x4 ZGFloat_matrix4x4;
#else
typedef float4 ZGFloat4;
typedef float2 ZGFloat2;
typedef float ZGFloat;
typedef matrix_float4x4 ZGFloat_matrix4x4;
#endif

#else

#include "platforms.h"

#if PLATFORM_APPLE

#if USE_HALF_WHEN_POSSIBLE
typedef _Float16 ZGFloat;
#else
typedef float ZGFloat;
#endif

#else
typedef float ZGFloat;
#endif


#endif
