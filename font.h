/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#pragma once

#include "maincore.h"

SDL_bool initFont(void);

void reloadGlyphs(void);

void drawStringf(GLfloat width, GLfloat height, const char *format, ...);
void drawString(GLfloat width, GLfloat height, char *string);
void drawGLUTStringf(const char *format, ...);

GLuint loadString(char *string);
