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

// b1の受け取り（マテリアルの要素を返す）
/*
cbuffer Material : register(b1)
{
	float4 diffuse; // ディヒューズ色
	float4 specular; // スペキュラ
	float4 ambient; // アンビエント（環境光）
};
*/