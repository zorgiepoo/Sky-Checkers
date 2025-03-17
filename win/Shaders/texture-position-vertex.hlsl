
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
