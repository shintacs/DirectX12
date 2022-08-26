struct Output
{
	float4 svpos : SV_POSITION; //システム用頂点座標
	float2 uv :TEXCOORD; //uv値
};

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix mat; // 変換行列
};