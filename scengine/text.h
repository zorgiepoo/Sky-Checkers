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

#include "math_3d.h"
#include "renderer.h"

#define SUPPORT_DEPRECATED_DRAW_STRING_APIS 1

// Requires Font subsystem to be initialized first
void initText(Renderer *renderer, int textRenderingCacheCount);

#if SUPPORT_DEPRECATED_DRAW_STRING_APIS
// Deprecated
void drawStringf(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat width, ZGFloat height, const char *format, ...);
// Deprecated
void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat width, ZGFloat height, const char *string);
#endif

void drawStringScaled(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat scale, const char *string);

void drawStringLeftAligned(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, ZGFloat scale, const char *string);
