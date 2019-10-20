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

void initConsole(void);

void buildBlackBorderDisplayList(void);
void deleteBlackBorderDisplayList(void);

void drawConsole(Renderer *renderer);

void writeConsoleText(Uint8 text);

bool performConsoleBackspace(void);

void drawConsoleText(Renderer *renderer);

void executeConsoleCommand(void);

void clearConsole(void);
