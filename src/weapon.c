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

static GLfloat gWeaponVertices[] =
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

static GLubyte gWeaponIndices[] =
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

void drawWeapon(Weapon *weap)
{
	if (!weap->drawingState)
		return;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glRotatef(-40, 1.0, 0.0, 0.0);
	glTranslatef(-7.0, 12.5, -23.0);
	
	glTranslatef(weap->x, weap->y, weap->z);
	
	glColor4f(weap->red, weap->green, weap->blue, 0.2f);
	
	int direction = weap->direction;
	if (direction == LEFT)
	{
		glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (direction == UP)
	{
		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (direction == DOWN)
	{
		glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, 0, gWeaponVertices);
	glDrawElements(GL_TRIANGLES, sizeof(gWeaponIndices) / sizeof(*gWeaponIndices), GL_UNSIGNED_BYTE, gWeaponIndices);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	
	glDisable(GL_BLEND);
	
	glLoadIdentity();
}
