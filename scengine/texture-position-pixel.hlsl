

struct PixelInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

float4 color : register (c0);
Texture2D shaderTexture : register (t0);

SamplerState textureSampler : register (s0);

float4 main(PixelInput input) : SV_TARGET
{
	return color * shaderTexture.Sample(textureSampler, input.texCoord);
}
