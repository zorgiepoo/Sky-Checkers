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
#include "platforms.h"

using namespace metal;

template <class T, class M>
static float4 _positionVertexShader(const ushort vertexID [[ vertex_id ]], const device T *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant M &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	return float4(modelViewProjection * vertices[vertexID]);
}

template <class T>
static T _positionFragmentShader(constant T &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return color;
}

vertex float4 positionVertexShaderHalf(const ushort vertexID [[ vertex_id ]], const device half4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_half4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	return _positionVertexShader(vertexID, vertices, modelViewProjection);
}

fragment half4 positionFragmentShaderHalf(constant half4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return _positionFragmentShader(color);
}

#if PLATFORM_OSX || PLATFORM_SIMULATOR

vertex float4 positionVertexShaderFull(const ushort vertexID [[ vertex_id ]], const device float4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]])
{
	return _positionVertexShader(vertexID, vertices, modelViewProjection);
}

fragment float4 positionFragmentShaderFull(constant float4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return _positionFragmentShader(color);
}

#endif

typedef struct
{
	float4 position [[position]];
	float2 textureCoordinate;
} TextureRasterizerData;

template <class T, class M, class C>
static TextureRasterizerData _texturePositionVertexShader(const ushort vertexID [[ vertex_id ]], const device T *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant M &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]], const device C *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]])
{
	TextureRasterizerData output;
	
	output.position = float4(modelViewProjection * vertices[vertexID]);
	output.textureCoordinate = float2(textureCoordinates[vertexID]);
	
	return output;
}

template <class T>
static T _texturePositionFragmentShader(const TextureRasterizerData input [[stage_in]], const texture2d<half> texture [[ texture(METAL_TEXTURE_INDEX) ]], constant T &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
	
	const half4 colorSample = texture.sample(textureSampler, input.textureCoordinate);
	
	return color * T(colorSample);
}

vertex TextureRasterizerData texturePositionVertexShaderHalf(const ushort vertexID [[ vertex_id ]], const device half4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_half4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]], const device half2 *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]])
{
	return _texturePositionVertexShader(vertexID, vertices, modelViewProjection, textureCoordinates);
}

fragment half4 texturePositionFragmentShaderHalf(const TextureRasterizerData input [[stage_in]], const texture2d<half> texture [[ texture(METAL_TEXTURE_INDEX) ]], constant half4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return _texturePositionFragmentShader(input, texture, color);
}

#if PLATFORM_OSX || PLATFORM_SIMULATOR

vertex TextureRasterizerData texturePositionVertexShaderFull(const ushort vertexID [[ vertex_id ]], const device float4 *vertices [[ buffer(METAL_BUFFER_VERTICES_INDEX) ]], constant matrix_float4x4 &modelViewProjection [[ buffer(METAL_BUFFER_MODELVIEW_PROJECTION_INDEX) ]], const device float2 *textureCoordinates [[ buffer(METAL_BUFFER_TEXTURE_COORDINATES_INDEX) ]])
{
	return _texturePositionVertexShader(vertexID, vertices, modelViewProjection, textureCoordinates);
}

fragment float4 texturePositionFragmentShaderFull(const TextureRasterizerData input [[stage_in]], const texture2d<half> texture [[ texture(METAL_TEXTURE_INDEX) ]], constant float4 &color [[ buffer(METAL_BUFFER_COLOR_INDEX) ]])
{
	return _texturePositionFragmentShader(input, texture, color);
}

#endif
