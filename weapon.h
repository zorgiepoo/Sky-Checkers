/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

typedef struct
{
	GLfloat x, y, z;
	GLfloat red, green, blue;
	
	SDL_bool drawingState;
	SDL_bool animationState;
	
	// display lists.
	GLuint d_list_right;
	GLuint d_list_left;
	GLuint d_list_up;
	GLuint d_list_down;
	
	int direction;
} Weapon;

void initWeapon(Weapon *weap);

void buildWeaponList(Weapon *weap);
void deleteWeaponList(Weapon *weap);

void drawWeapon(Weapon *weap);
