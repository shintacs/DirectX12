#include "BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD) {
	Output output;
	output.svpos = mul(mat, pos); // s—ñ•ÏŠ·
	output.uv = uv;
	return output;
}