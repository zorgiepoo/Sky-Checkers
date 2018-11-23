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

static float gWeaponVertices[] =
{
	// Cube part
	
	// right face
	-1.0, -1.0, 1.0,
	-1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
	1.0, -1.0, 1.0,
	
	// left face
	-1.0, -1.0, -1.0,
	-1.0, 1.0, -1.0,
	1.0, 1.0, -1.0,
	1.0, -1.0, -1.0,
	
	// front face
	1.0, 1.0, 1.0,
	1.0, -1.0, 1.0,
	1.0, -1.0, -1.0,
	1.0, 1.0, -1.0,
	
	// back face
	-1.0, -1.0, 1.0,
	-1.0, 1.0, 1.0,
	-1.0, 1.0, -1.0,
	-1.0, -1.0, -1.0,
	
	// top face
	-1.0, 1.0, 1.0,
	-1.0, 1.0, -1.0,
	1.0, 1.0, -1.0,
	1.0, 1.0, 1.0,
	
	// bottom face
	-1.0, -1.0, 1.0,
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0, 1.0,
	
	// Prism part
	
	// right side
	1.0, 1.0, 1.0,
	1.0, -1.0, 1.0,
	3.0, 0.0, 0.0,
	
	// left side
	1.0, 1.0, -1.0,
	1.0, -1.0, -1.0,
	3.0, 0.0, 0.0,
	
	// top side
	1.0, 1.0, 1.0,
	1.0, 1.0, -1.0,
	3.0, 0.0, 0.0,
	
	// bottom side
	1.0, -1.0, 1.0,
	1.0, -1.0, -1.0,
	3.0, 0.0, 0.0
};

static uint8_t gWeaponIndices[] =
{
	// Cube part
	
	// right face
	0, 1, 2,
	2, 3, 0,
	
	// left face
	4, 5, 6,
	6, 7, 4,
	
	// front face
	8, 9, 10,
	10, 11, 8,
	
	// back face
	12, 13, 14,
	14, 15, 12,
	
	// top face
	16, 17, 18,
	18, 19, 16,
	
	// bottom face
	20, 21, 22,
	22, 23, 20,
	
	// Prism part
	
	// right side
	24, 25, 26,
	
	// left side
	27, 28, 29,
	
	// top side
	30, 31, 32,
	
	// bottom side
	33, 34, 35
};

void initWeapon(Weapon *weap)
{	
	weap->x = 0.0;
	weap->y = 0.0;
	weap->z = -1.0;
	
	weap->drawingState = SDL_FALSE;
	weap->animationState = SDL_FALSE;
	weap->direction = 0;
}

void drawWeapon(Renderer *renderer, Weapon *weap)
{
	if (!weap->drawingState)
		return;
	
	mat4_t worldRotationMatrix = m4_rotation_x(-40.0f * (M_PI / 180.0f));
	mat4_t worldTranslationMatrix = m4_translation((vec3_t){-7.0f, 12.5f, -23.0f});
	mat4_t modelTranslationMatrix = m4_translation((vec3_t){weap->x, weap->y, weap->z});
	
	mat4_t weaponMatrix = m4_mul(m4_mul(worldRotationMatrix, worldTranslationMatrix), modelTranslationMatrix);
	
	float weaponRotationAngle = 0.0f;
	int direction = weap->direction;
	if (direction == LEFT)
	{
		weaponRotationAngle = M_PI;
	}
	else if (direction == UP)
	{
		weaponRotationAngle = M_PI / 2.0f;
	}
	else if (direction == DOWN)
	{
		weaponRotationAngle = 3.0f * M_PI / 2.0;
	}
	
	mat4_t weaponRotationMatrix = m4_rotation_z(weaponRotationAngle);
	mat4_t modelViewMatrix = m4_mul(weaponMatrix, weaponRotationMatrix);
	
	drawVerticesFromIndices(renderer, modelViewMatrix, RENDERER_TRIANGLE_MODE, gWeaponVertices, 3, gWeaponIndices, RENDERER_INT8_TYPE, sizeof(gWeaponIndices) / sizeof(*gWeaponIndices), (color4_t){weap->red, weap->green, weap->blue, 0.2f}, RENDERER_OPTION_BLENDING_ONE_MINUS_ALPHA);
}
