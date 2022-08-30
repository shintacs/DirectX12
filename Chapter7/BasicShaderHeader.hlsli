struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 normal:NORMAL; // 法線ベクトル
	float2 uv :TEXCOORD; // uv値
};

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix world; // ワールド変換行列
	matrix viewproj; // ビュープロジェクション行列
};