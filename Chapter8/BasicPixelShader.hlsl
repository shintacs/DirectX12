#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex:register(t0);//0番スロットに設定されたテクスチャ(ベース)

SamplerState smp:register(s0);//0番スロットに設定されたサンプラ

float4 BasicPS(Output input) : SV_TARGET{
	// return float4(tex.Sample(smp, input.uv));
	float3 light = normalize(float3(1, -1, 1)); // 光源の相対的な位置ベクトル（右下奥）
	float brightness = dot(-light, input.normal); // lightの逆ベクトルと法線の内積を計算（輝度値をスカラで返す）
	float4 texColor = tex.Sample(smp, input.uv); // テクスチャカラー
	//return float4(brightness, brightness, brightness, 1);
	return float4(brightness, brightness, brightness, 1) * diffuse * texColor;
}