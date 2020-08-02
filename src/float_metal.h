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

#include "platforms.h"

#if APPLE_ARM64

#define ZGMatrixFloat4x4 matrix_half4x4
#define ZGFloat4 half4
#define ZGFloat2 half2

#else

#define ZGMatrixFloat4x4 matrix_float4x4
#define ZGFloat4 float4
#define ZGFloat2 float2

#endif
