/*
 * Copyright 2018 Mayur Pawashe
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

#ifdef __cplusplus
extern "C" {
#endif

#include "renderer_types.h"
#include "platforms.h"

#define PROJECTION_FOV_ANGLE 45.0f // in degrees
#define PROJECTION_NEAR_VIEW_DISTANCE 10.0f
#define PROJECTION_FAR_VIEW_DISTANCE 300.0f

ZGFloat computeProjectionAspectRatio(Renderer *renderer);

void updateGLProjectionMatrix(Renderer *renderer);

#ifdef MAC_OS_X
void updateMetalProjectionMatrix(Renderer *renderer);
#endif

#ifdef __cplusplus
}
#endif
