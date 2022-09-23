#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�x�[�X)

SamplerState smp:register(s0);//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

float4 BasicPS(Output input) : SV_TARGET{
	float3 light = normalize(float3(1, -1, 1)); // �����̑��ΓI�Ȉʒu�x�N�g���i�E�����j
	float brightness = dot(-light, input.normal); // light�̋t�x�N�g���Ɩ@���̓��ς��v�Z�i�P�x�l���X�J���ŕԂ��j
	float4 texColor = tex.Sample(smp, input.uv); // �e�N�X�`���J���[
	//float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5); // ���ƃs�N�Z���l��uv�ɍ��킹��
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
	float4 sphMap = sph.Sample(smp, sphereMapUV); // �X�t�B�A�}�b�v�i��Z�j
	float4 spaMap = spa.Sample(smp, sphereMapUV); // �X�t�B�A�}�b�v�i���Z�j

	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor;
	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor + saturate(spa.Sample(smp, normalUV)* texColor);
	return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap + spaMap;
	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap;
}