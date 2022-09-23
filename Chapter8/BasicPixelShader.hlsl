#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex:register(t0);//0番スロットに設定されたテクスチャ(ベース)

SamplerState smp:register(s0);//0番スロットに設定されたサンプラ

float4 BasicPS(Output input) : SV_TARGET{
	float3 light = normalize(float3(1, -1, 1)); // 光源の相対的な位置ベクトル（右下奥）
	float brightness = dot(-light, input.normal); // lightの逆ベクトルと法線の内積を計算（輝度値をスカラで返す）
	float4 texColor = tex.Sample(smp, input.uv); // テクスチャカラー
	//float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5); // 軸とピクセル値をuvに合わせる
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
	float4 sphMap = sph.Sample(smp, sphereMapUV); // スフィアマップ（乗算）
	float4 spaMap = spa.Sample(smp, sphereMapUV); // スフィアマップ（加算）

	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor;
	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor + saturate(spa.Sample(smp, normalUV)* texColor);
	return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap + spaMap;
	//return float4(brightness, brightness, brightness, 1) * diffuse * texColor * sphMap;
}