struct Output
{
	float4 svpos : SV_POSITION; // �V�X�e���p���_���W
	float4 pos : POSITION; // ���_���W
	float4 normal:NORMAL0; // �@���x�N�g��
	float4 vnormal:NORMAL1; // �r���[�ϊ���̖@���x�N�g��
	float2 uv :TEXCOORD; // uv�l
	float3 ray : VECTOR; // �x�N�g��
};

cbuffer cbuff0 : register(b0) // �萔�o�b�t�@�[
{
	matrix world; // ���[���h�ϊ��s��
	matrix view; // �r���[�s��
	matrix proj; // �v���W�F�N�V�����s��
	float3 eye; // ���_���W
};

// b1�̎󂯎��i�}�e���A���̗v�f��Ԃ��j

cbuffer Material : register(b1)
{
	float4 diffuse; // �f�B�q���[�Y�F
	float4 specular; // �X�y�L����
	float3 ambient; // �A���r�G���g�i�����j
};

// sph�p�̃e�N�X�`���ϐ�

Texture2D<float4> sph : register(t1); // 1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

// spa�p�̃e�N�X�`���ϐ�
Texture2D<float4> spa : register(t2); // 2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��