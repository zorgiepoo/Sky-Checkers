/*
 * Copyright 2010 Mayur Pawashe
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

#include "weapon.h"
#include "characters.h"
#include "math_3d.h"

void initWeapon(Weapon *weap)
{	
	weap->x = 0.0f;
	weap->y = 0.0f;
	weap->z = 1.0f;
	
	weap->initialX = 0.0f;
	weap->initialX = 0.0f;
	
	weap->compensation = 0.0f;
	
	weap->drawingState = false;
	weap->animationState = false;
	weap->direction = NO_DIRECTION;
}

void drawWeapon(Renderer *renderer, Weapon *weap)
{
	if (!weap->drawingState || !weap->animationState)
		return;
	
	static BufferArrayObject vertexArrayObject;
	static BufferObject indicesBufferObject;
	static bool initializedBuffers;
	
	if (!initializedBuffers)
	{
		const ZGFloat vertices[] =
		{
			// top face
			1.0f, 0.0f, 0.5f, 1.0f, // right mid point
			0.0f, 1.0f, 0.5f, 1.0f, // left back
			0.0f, -1.0f, 0.5f, 1.0f, // left front
			
			// bottom face
			1.0f, 0.0f, 0.0f, 1.0f, // right mid point
			0.0f, 1.0f, 0.0f, 1.0f, // left back
			0.0f, -1.0f, 0.0f, 1.0f // left front
		};
		
		const uint16_t indices[] =
		{
			// top
			0, 1, 2,
			// bottom
			3, 4, 5,
			// front side
			2, 5, 3,
			2, 0, 3,
			// back side
			1, 4, 3,
			1, 0, 3
		};
		
		vertexArrayObject = createVertexArrayObject(renderer, vertices, sizeof(vertices));
		indicesBufferObject = createIndexBufferObject(renderer, indices, sizeof(indices));
		
		initializedBuffers = true;
	}
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * ((ZGFloat)M_PI / 180.0f));
	mat4_t worldTranslationMatrix = m4_translation((vec3_t){-7.0f, 12.5f, -23.0f});
	mat4_t modelTranslationMatrix = m4_translation((vec3_t){weap->x, weap->y, weap->z});
	
	mat4_t weaponMatrix = m4_mul(m4_mul(worldRotationMatrix, worldTranslationMatrix), modelTranslationMatrix);
	
	ZGFloat weaponRotationAngle = 0.0f;
	int direction = weap->direction;
	if (direction == LEFT)
	{
		weaponRotationAngle = (ZGFloat)M_PI;
	}
	else if (direction == UP)
	{
		weaponRotationAngle = (ZGFloat)M_PI / 2.0f;
	}
	else if (direction == DOWN)
	{
		weaponRotationAngle = 3.0f * (ZGFloat)M_PI / 2.0f;
	}
	
	mat4_t weaponRotationMatrix = m4_rotation_z(weaponRotationAngle);
	mat4_t modelViewMatrix = m4_mul(weaponMatrix, weaponRotationMatrix);
	
	ZGFloat colorFactor = 0.2f;
	drawVerticesFromIndices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, vertexArrayObject, indicesBufferObject, 18
							, (color4_t){weap->red * colorFactor, weap->green * colorFactor, weap->blue * colorFactor, 0.8f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}
