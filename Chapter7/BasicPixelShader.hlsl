#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET{
	// return float4(tex.Sample(smp, input.uv));
	return float4(0, 0, 0, 1); //‰½ŒÌ^‚Á•‚É‚·‚é‚Ì‚¾‚ë‚¤HH
}