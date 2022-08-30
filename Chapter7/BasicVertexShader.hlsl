#include "BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0番スロットに設定されたサンプラー

Output BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT) {
	Output output;
	output.svpos = mul(mul(viewproj, world), pos); // 行列変換（シェーダーでは列優先なので順番が逆になる（なんで合わせないの！？））
	normal.w = 0; // 平行移動成分を無効にする（法線ベクトルは移動による影響を受けないため）
	output.normal = mul(world, normal); // 法線ベクトルの値（法線にもワールド変換を行う）
	output.uv = uv;
	return output;
}