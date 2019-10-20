/*
* Copyright 2019 Mayur Pawashe
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
#include "renderer_types.h"

#define FONT_POINT_SIZE 144
// This font is "goodfish.ttf" and is intentionally obfuscated in source by author's request
// A license to embed the font was acquired (for me, Mayur, only) from http://typodermicfonts.com/goodfish/
#define FONT_PATH "Data/Fonts/typelib.dat"

typedef void* TextData;

void initFont(void);

TextData createTextData(const char *string);
void releaseTextData(TextData textData);

void *getTextDataPixels(TextData textData);

int32_t getTextDataWidth(TextData textData);
int32_t getTextDataHeight(TextData textData);
PixelFormat getPixelFormat(TextData textData);
