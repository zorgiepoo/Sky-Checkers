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

#ifndef metal_indices_h
#define metal_indices_h

typedef enum
{
	METAL_BUFFER_VERTICES_INDEX = 0,
	METAL_BUFFER_MODELVIEW_PROJECTION_INDEX = 1,
	METAL_BUFFER_COLOR_INDEX = 2,
	METAL_BUFFER_TEXTURE_COORDINATES_INDEX = 3,
	METAL_BUFFER_TEXTURE_INDICES_INDEX = 4
} MetalBufferIndex;

typedef enum
{
	METAL_TEXTURE_INDEX = 0
} MetalTextureIndex;

#endif /* metal_indices_h */
