struct Output
{
	float4 svpos : SV_POSITION; // �V�X�e���p���_���W
	float4 normal:NORMAL; // �@���x�N�g��
	float2 uv :TEXCOORD; // uv�l
};

cbuffer cbuff0 : register(b0) // �萔�o�b�t�@�[
{
	matrix world; // ���[���h�ϊ��s��
	matrix viewproj; // �r���[�v���W�F�N�V�����s��
};

// b1�̎󂯎��i�}�e���A���̗v�f��Ԃ��j
/*
cbuffer Material : register(b1)
{
	float4 diffuse; // �f�B�q���[�Y�F
	float4 specular; // �X�y�L����
	float4 ambient; // �A���r�G���g�i�����j
};
*/