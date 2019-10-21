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

#pragma once

#include "maincore.h"
#include "math_3d.h"
#include "renderer.h"

void initText(Renderer *renderer);

// Deprecated
void drawStringf(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float width, float height, const char *format, ...);
// Deprecated
void drawString(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float width, float height, const char *string);

void drawStringScaled(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float scale, const char *string);

void drawStringLeftAligned(Renderer *renderer, mat4_t modelViewMatrix, color4_t color, float scale, const char *string);

int cacheString(Renderer *renderer, const char *string);