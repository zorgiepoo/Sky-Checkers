
float4x4 modelViewProjectionMatrix : register(c0);

float4 main(const float4 position : POSITION) : SV_POSITION
{
	return mul(modelViewProjectionMatrix, position);
}
