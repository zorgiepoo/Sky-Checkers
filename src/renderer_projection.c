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

void updateProjectionMatrix(Renderer *renderer)
{
	// The aspect ratio is not quite correct, which is a mistake I made a long time ago that is too troubling to fix properly
	mat4_t glProjectionMatrix = m4_perspective(45.0f, (float)(renderer->drawableWidth / renderer->drawableHeight), 10.0f, 300.0f);
	
	switch (renderer->ndcType)
	{
		case NDC_TYPE_GL:
			memcpy(renderer->projectionMatrix, glProjectionMatrix.m, sizeof(glProjectionMatrix.m));
			break;
#ifndef linux
		case NDC_TYPE_METAL:
		{
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
			
			break;
		}
#endif
	}
}
