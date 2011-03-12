/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

void mt_init();
unsigned long mt_random();

void loadTexture(const char *filePath, GLuint *tex);

void zgTexCoord2f(GLfloat x, GLfloat y);
void zgPrint(const char *format, ...);

void SDL_Terminate(void);
