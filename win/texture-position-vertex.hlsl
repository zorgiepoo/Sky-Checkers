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

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

float4x4 modelViewProjectionMatrix : register(c0);

VertexOutput main(float4 position : POSITION, float2 texCoord : TEXCOORD0)
{
	VertexOutput output;
	output.position = mul(modelViewProjectionMatrix, position);
	output.texCoord = texCoord;
	return output;
}
