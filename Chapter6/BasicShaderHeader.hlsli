struct Output
{
	float4 svpos : SV_POSITION; //�V�X�e���p���_���W
	float2 uv :TEXCOORD; //uv�l
};

cbuffer cbuff0 : register(b0) // �萔�o�b�t�@�[
{
	matrix mat; // �ϊ��s��
};