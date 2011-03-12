/*
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
 
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#include "weapon.h"
#include "characters.h"

void initWeapon(Weapon *weap)
{	
	weap->x = 0.0;
	weap->y = 0.0;
	weap->z = -1.0;
	
	weap->drawingState = SDL_FALSE;
	weap->animationState = SDL_FALSE;
	weap->direction = 0;
}

/*
 * Some considerations in this drawing code are:
 * These objects will be roated to fit correctly on the checkerboard, meaning that the y axis will
 * look like the z axis, and that the z axis will look like the y axis when drawing
 */
void buildWeaponList(Weapon *weap)
{
	weap->d_list_right = glGenLists(1);
	glNewList(weap->d_list_right, GL_COMPILE);
	
	glBegin(GL_QUADS);
	
	// right face
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	// left face.
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	// front face.
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	// back face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	// top face.
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	// bottom face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glEnd();
	
	glBegin(GL_TRIANGLES);
	
	// right side.
	glVertex3f(1.0, 1.0, 1.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(3.0, 0.0, 0.0);
	
	// left side.
	glVertex3f(1.0, 1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(3.0, 0.0, 0.0);
	
	// top side.
	glVertex3f(1.0, 1.0, 1.0);
	glVertex3f(1.0, 1.0, -1.0);
	glVertex3f(3.0, 0.0, 0.0);
	
	// bottom side.
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(3.0, 0.0, 0.0);
	
	glEnd();
	
	glEndList();
	
	weap->d_list_left = glGenLists(1);
	glNewList(weap->d_list_left, GL_COMPILE);
	
	glBegin(GL_QUADS);
	
	// right face
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	// left face.
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	// front face.
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	// back face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	// top face.
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	// bottom face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glEnd();
	
	glBegin(GL_TRIANGLES);
	
	// right side.
	glVertex3f(-1.0, 1.0, 1.0);
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-3.0, 0.0, 0.0);
	
	// left side.
	glVertex3f(-1.0, 1.0, -1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(-3.0, 0.0, 0.0);
	
	// top side.
	glVertex3f(-1.0, 1.0, 1.0);
	glVertex3f(-1.0, 1.0, -1.0);
	glVertex3f(-3.0, 0.0, 0.0);
	
	// bottom side.
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(-3.0, 0.0, 0.0);
	
	glEnd();
	
	glEndList();
	
	weap->d_list_up = glGenLists(1);
	glNewList(weap->d_list_up, GL_COMPILE);
	
	glBegin(GL_QUADS);
	
	// right face
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	// left face.
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	// front face.
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	// back face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	// top face.
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	// bottom face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glEnd();
	
	glBegin(GL_TRIANGLES);
	
	// right side.
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(0.0, 3.0, 0.0);
	
	// left side.
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(0.0, 3.0, 0.0);
	
	// top side.
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(0.0, 3.0, 0.0);
	
	// bottom side.
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(0.0, 3.0, 0.0);
	
	glEnd();
	
	glEndList();
	
	weap->d_list_down = glGenLists(1);
	glNewList(weap->d_list_down, GL_COMPILE);
	
	glBegin(GL_QUADS);
	
	// right face
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	// left face.
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	// front face.
	glVertex3f(1.0, 1.0, 1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	// back face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	// top face.
	glVertex3f(-1.0, 1.0, 1.0);
	
	glVertex3f(-1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, -1.0);
	
	glVertex3f(1.0, 1.0, 1.0);
	
	// bottom face.
	glVertex3f(-1.0, -1.0, 1.0);
	
	glVertex3f(-1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, -1.0);
	
	glVertex3f(1.0, -1.0, 1.0);
	
	glEnd();
	
	glBegin(GL_TRIANGLES);
	
	// right side.
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(0.0, -3.0, 0.0);
	
	// left side.
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(0.0, -3.0, 0.0);
	
	// top side.
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(0.0, -3.0, 0.0);
	
	// bottom side.
	glVertex3f(-1.0, -1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glVertex3f(0.0, -3.0, 0.0);
	
	glEnd();
	
	glEndList();
}

void deleteWeaponList(Weapon *weap)
{
	glDeleteLists(weap->d_list_right, 1);
	glDeleteLists(weap->d_list_left, 1);
	glDeleteLists(weap->d_list_up, 1);
	glDeleteLists(weap->d_list_down, 1);
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
	
	glColor4f(weap->red, weap->green, weap->blue, 0.2);
	
	if (weap->direction == RIGHT)
		glCallList(weap->d_list_right);
	else if (weap->direction == LEFT)
		glCallList(weap->d_list_left);
	else if (weap->direction == UP)
		glCallList(weap->d_list_up);
	else if (weap->direction == DOWN)
		glCallList(weap->d_list_down);
	
	glDisable(GL_BLEND);
	
	glLoadIdentity();
}
