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

#ifdef __cplusplus
extern "C" {
#endif

#include "renderer_types.h"
#include "platforms.h"

#define PROJECTION_FOV_ANGLE 45.0f // in degrees
#define PROJECTION_NEAR_VIEW_DISTANCE 0.01f
#define PROJECTION_FAR_VIEW_DISTANCE 10000.0f

ZGFloat computeProjectionAspectRatio(Renderer *renderer);

void updateGLProjectionMatrix(Renderer *renderer);

#if PLATFORM_APPLE
void updateMetalProjectionMatrix(Renderer *renderer);
#endif

#ifdef __cplusplus
}
#endif
