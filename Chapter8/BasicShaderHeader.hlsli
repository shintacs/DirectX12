struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 pos : POSITION; // 頂点座標
	float4 normal:NORMAL0; // 法線ベクトル
	float4 vnormal:NORMAL1; // ビュー変換後の法線ベクトル
	float2 uv :TEXCOORD; // uv値
	float3 ray : VECTOR; // ベクトル
};

cbuffer cbuff0 : register(b0) // 定数バッファー
{
	matrix world; // ワールド変換行列
	matrix view; // ビュー行列
	matrix proj; // プロジェクション行列
	float3 eye; // 視点座標
};

// b1の受け取り（マテリアルの要素を返す）

cbuffer Material : register(b1)
{
	float4 diffuse; // ディヒューズ色
	float4 specular; // スペキュラ
	float3 ambient; // アンビエント（環境光）
};

// sph用のテクスチャ変数

Texture2D<float4> sph : register(t1); // 1番スロットに設定されたテクスチャ

// spa用のテクスチャ変数
Texture2D<float4> spa : register(t2); // 2番スロットに設定されたテクスチャ