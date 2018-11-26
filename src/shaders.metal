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

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

vertex float4 positionVertexShader(uint vertexID [[ vertex_id ]], constant float3 *vertices [[ buffer(0) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(1) ]])
{
	return modelViewProjection * float4(vertices[vertexID], 1.0f);
}

fragment float4 positionFragmentShader(constant float4 &color [[ buffer(2) ]])
{
	return color;
}

typedef struct
{
	float4 position [[position]];
	float2 textureCoordinate;
} TextureRasterizerData;

vertex TextureRasterizerData texturePositionVertexShader(uint vertexID [[ vertex_id ]], device float3 *vertices [[ buffer(0) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(1) ]], device float2 *textureCoordinates [[ buffer(2) ]])
{
	TextureRasterizerData output;
	
	output.position = modelViewProjection * float4(vertices[vertexID], 1.0f);
	output.textureCoordinate = textureCoordinates[vertexID];
	
	return output;
}

fragment float4 texturePositionFragmentShader(TextureRasterizerData input [[stage_in]], texture2d<half> texture [[ texture(0) ]], constant float4 &color [[ buffer(3) ]])
{
	constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
	
	const half4 colorSample = texture.sample(textureSampler, input.textureCoordinate);
	
	return color * float4(colorSample);
}
