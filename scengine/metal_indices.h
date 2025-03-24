/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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

#define METAL_MAX_TEXTURE_COUNT 16

#endif /* metal_indices_h */
