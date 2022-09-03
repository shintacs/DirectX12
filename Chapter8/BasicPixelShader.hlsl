#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET{
	// return float4(tex.Sample(smp, input.uv));
	float3 light = normalize(float3(1, -1, 1)); // �����̑��ΓI�Ȉʒu�x�N�g���i�E�����j
	float brightness = dot(-light, input.normal); // light�̋t�x�N�g���Ɩ@���̓��ς��v�Z�i�P�x�l���X�J���ŕԂ��j
	// return float4(brightness, brightness, brightness, 1) * diffuse;
	return float4(brightness, brightness, brightness, 1);
}