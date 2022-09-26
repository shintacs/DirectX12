#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�x�[�X)

SamplerState smp:register(s0);// 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��
SamplerState smpToon:register(s1);// 1�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

float4 BasicPS(Output input) : SV_TARGET{
	float3 light = normalize(float3(1, -1, 1)); // �����̑��ΓI�Ȉʒu�x�N�g���i�E�����j
	//float3 lightColor = float3(1, 1, 1);
	float brightness = dot(-light, input.normal); // light�̋t�x�N�g���Ɩ@���̓��ς��v�Z�i�P�x�l���X�J���ŕԂ��j

	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));
	
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 sphMap = sph.Sample(smp, sphereMapUV); // �X�t�B�A�}�b�v�i��Z�j
	float4 spaMap = spa.Sample(smp, sphereMapUV); // �X�t�B�A�}�b�v�i���Z�j

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	float4 texColor = tex.Sample(smp, input.uv); // �e�N�X�`���J���[

	// return float4(brightness, brightness, brightness, 1) * diffuse * texColor;
	// return float4(brightness, brightness, brightness, 1) * diffuse * texColor + saturate(spa.Sample(smp, normalUV)* texColor);
	// return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap;
	// �X�y�L���������f���Ă݂�
	//return max(diffuseB * diffuse * texColor + float4(specularB * specular.rgb, 1), float4(ambient*texColor, 1));
	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap + spaMap + texColor*ambient;
	return //max(
		saturate(toonDif // �P�x
		//diffuseB
			* diffuse // �f�B�t���[�Y�F
			* texColor // �e�N�X�`���J���[
			* sphMap) // �X�t�B�A�}�b�v�i��Z�j
		+ saturate(spaMap * texColor // �X�t�B�A�}�b�v�i���Z�j
			+ float4(specularB * specular.rgb, 1)); // �X�y�L����
		//+ float4(texColor* ambient, 1);
}