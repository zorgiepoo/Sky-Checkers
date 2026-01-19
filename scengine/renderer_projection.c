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

#include "renderer_projection.h"
#include "math_3d.h"
#include <string.h>

ZGFloat computeProjectionAspectRatio(Renderer *renderer)
{
	// Some history is this used to be width / height (where both width and height were integers), resulting in 1 most of the time.
	ZGFloat drawableWidth = (ZGFloat)renderer->drawableWidth;
	ZGFloat drawableHeight = (ZGFloat)renderer->drawableHeight;
	
	ZGFloat sizeRatio = fmaxf(drawableWidth, drawableHeight) / fminf(drawableWidth, drawableHeight);
	
	// If size is 4:3 or 4.3:1 just use 1.0
	if (fabsf(sizeRatio - (4.0f / 3.0f)) <= 0.01f || fabsf(sizeRatio - (4.3f / 3.0f)) <= 0.01f)
	{
		return (ZGFloat)1.0f;
	}
	else
	{
		return sizeRatio / (ZGFloat)(16.0f / 9.0f);
	}
}

static mat4_t computeGLProjectionMatrix(Renderer *renderer)
{
	return m4_perspective(PROJECTION_FOV_ANGLE, computeProjectionAspectRatio(renderer), PROJECTION_NEAR_VIEW_DISTANCE, PROJECTION_FAR_VIEW_DISTANCE);
}

void updateGLProjectionMatrix(Renderer *renderer)
{
	mat4_t glProjectionMatrix = computeGLProjectionMatrix(renderer);
	
	memcpy(renderer->projectionMatrix, glProjectionMatrix.m, sizeof(glProjectionMatrix.m));
}

#if PLATFORM_APPLE
void updateMetalProjectionMatrix(Renderer *renderer)
{
	mat4_t glProjectionMatrix = computeGLProjectionMatrix(renderer);
	
	// https://metashapes.com/blog/opengl-metal-projection-matrix-problem/
	mat4_t metalProjectionAdjustMatrix =
	mat4(
		 1.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 1.0f, 0.0f, 0.0f,
		 0.0f, 0.0f, 0.5f, 0.5f,
		 0.0f, 0.0f, 0.0f, 1.0f
		 );
	mat4_t finalProjectionMatrix = m4_mul(metalProjectionAdjustMatrix, glProjectionMatrix);
	
	memcpy(renderer->projectionMatrix, finalProjectionMatrix.m, sizeof(finalProjectionMatrix.m));
}
#endif
