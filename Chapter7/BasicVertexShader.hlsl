#include "BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); // 0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

Output BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT) {
	Output output;
	output.svpos = mul(mul(viewproj, world), pos); // �s��ϊ��i�V�F�[�_�[�ł͗�D��Ȃ̂ŏ��Ԃ��t�ɂȂ�i�Ȃ�ō��킹�Ȃ��́I�H�j�j
	normal.w = 0; // ���s�ړ������𖳌��ɂ���i�@���x�N�g���͈ړ��ɂ��e�����󂯂Ȃ����߁j
	output.normal = mul(world, normal); // �@���x�N�g���̒l�i�@���ɂ����[���h�ϊ����s���j
	output.uv = uv;
	return output;
}