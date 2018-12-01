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
#include "metal_indices.h"

using namespace metal;

vertex float4 positionVertexShader(ushort vertexID [[ vertex_id ]], constant packed_float3 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	return modelViewProjection * float4(vertices[vertexID], 1.0f);
}

fragment float4 positionFragmentShader(constant float4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return color;
}

typedef struct
{
	float4 position [[position]];
	float2 textureCoordinate;
} TextureRasterizerData;

vertex TextureRasterizerData texturePositionVertexShader(ushort vertexID [[ vertex_id ]], device packed_float3 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]], device packed_float2 *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]])
{
	TextureRasterizerData output;
	
	output.position = modelViewProjection * float4(vertices[vertexID], 1.0f);
	output.textureCoordinate = textureCoordinates[vertexID];
	
	return output;
}

fragment float4 texturePositionFragmentShader(TextureRasterizerData input [[stage_in]], texture2d<half> texture [[ texture(METAL_TEXTURE1_INDEX) ]], constant float4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
	
	const half4 colorSample = texture.sample(textureSampler, input.textureCoordinate);
	
	return color * float4(colorSample);
}

typedef struct
{
	float4 position [[position]];
	float2 textureCoordinate;
	ushort instanceID;
} TilesRasterizerData;

vertex TilesRasterizerData tilesVertexShader(ushort vertexID [[ vertex_id ]], ushort instanceID [[instance_id]], device packed_float3 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], device packed_float2 *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]], constant matrix_float4x4 *modelViewProjections [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	TilesRasterizerData output;
	
	output.position = modelViewProjections[instanceID] * float4(vertices[vertexID], 1.0f);
	output.textureCoordinate = textureCoordinates[vertexID];
	output.instanceID = instanceID;
	
	return output;
}

fragment float4 tilesFragmentShader(TilesRasterizerData input [[stage_in]], texture2d_array<half> textures [[ texture(METAL_TEXTURE1_INDEX) ]], constant uint *textureIndices [[ buffer(METAL_BUFFER_TEXTURE_INDICES_INDEX) ]], constant packed_float4 *colors [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);

	const half4 colorSample = textures.sample(textureSampler, input.textureCoordinate, textureIndices[input.instanceID]);

	return colors[input.instanceID] * float4(colorSample);
}
