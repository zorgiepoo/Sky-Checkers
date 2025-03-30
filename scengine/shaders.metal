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

#include <metal_stdlib>
#include <simd/simd.h>
#include "metal_indices.h"
#include "float.h"

using namespace metal;

vertex float4 positionVertexShader(const ushort vertexID [[ vertex_id ]], const device ZGFloat4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant ZGFloat_matrix4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	return float4(modelViewProjection * vertices[vertexID]);
}

fragment ZGFloat4 positionFragmentShader(constant ZGFloat4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return color;
}

typedef struct
{
	float4 position [[position]];
	float2 textureCoordinate;
} TextureRasterizerData;

vertex TextureRasterizerData texturePositionVertexShader(const ushort vertexID [[ vertex_id ]], const device ZGFloat4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant ZGFloat_matrix4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]], const device ZGFloat2 *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]])
{
	TextureRasterizerData output;
	
	output.position = float4(modelViewProjection * vertices[vertexID]);
	output.textureCoordinate = float2(textureCoordinates[vertexID]);
	
	return output;
}

fragment ZGFloat4 texturePositionFragmentShader(const TextureRasterizerData input [[stage_in]], const texture2d<ZGFloat> texture [[ texture(METAL_TEXTURE_INDEX) ]], constant ZGFloat4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
	
	const ZGFloat4 colorSample = texture.sample(textureSampler, input.textureCoordinate);
	
	return color * ZGFloat4(colorSample);
}
