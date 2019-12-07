/*
 * Copyright 2018 Mayur Pawashe
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

#include "renderer_projection.h"
#include "math_3d.h"
#include <string.h>

ZGFloat computeProjectionAspectRatio(Renderer *renderer)
{
	// Use a constant aspect ratio that stretches at any window size
	// We want to avoid a letterbox effect and allow the window to be resized in any way
	// Some history is this used to be width / height (where both width and height were integers), resulting in 1 most of the time.
	return (ZGFloat)1.0f;
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
