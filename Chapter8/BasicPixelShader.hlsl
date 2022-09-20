#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�x�[�X)

SamplerState smp:register(s0);//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

float4 BasicPS(Output input) : SV_TARGET{
	// return float4(tex.Sample(smp, input.uv));
	float3 light = normalize(float3(1, -1, 1)); // �����̑��ΓI�Ȉʒu�x�N�g���i�E�����j
	float brightness = dot(-light, input.normal); // light�̋t�x�N�g���Ɩ@���̓��ς��v�Z�i�P�x�l���X�J���ŕԂ��j
	float4 texColor = tex.Sample(smp, input.uv); // �e�N�X�`���J���[
	//return float4(brightness, brightness, brightness, 1);
	return float4(brightness, brightness, brightness, 1) * diffuse * texColor;
}