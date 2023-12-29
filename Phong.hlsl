//グローバル
cbuffer global
{
	matrix g_w;//ワールド行列
	matrix g_wvp; //ワールドから射影までの変換行列
	float4 g_lightDir;//ライトの方向ベクトル	
	float4 g_eye;	//カメラ（視点）
	float4 g_ambient;//環境光
	float4 g_diffuse;//拡散反射光
	float4 g_specular;//鏡面反射光
};

Texture2D g_Tex: register(t0);
SamplerState g_Sampler : register(s0);

//構造体
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 light : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 eyeVector : TEXCOORD2;
	float2 uv : TEXCOORD3;
};
//バーテックスシェーダー
//
VS_OUTPUT VS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.pos = mul(pos, g_wvp);
	output.normal = abs(mul(normal, (float3x3)g_w));
	output.light = g_lightDir;
	float3 posWorld = mul(pos, g_w);
	output.eyeVector = g_eye - posWorld;
	output.uv = uv;

	return output;
}
//ピクセルシェーダー
//
float4 PS(VS_OUTPUT input) : SV_Target
{
	float3 normal = normalize(input.normal);
	float3 lightDir = normalize(input.light);
	float3 viewDir = normalize(input.eyeVector);
	float4 nl = saturate(dot(normal, lightDir));

	float3 reflect = normalize(2 * nl * normal - lightDir);
	float4 specular = g_specular * pow(saturate(dot(reflect, viewDir)),2);

	float4 texel = g_Tex.Sample(g_Sampler, input.uv);

	float4 color = (g_diffuse + texel) * nl + specular;

	return color;
}