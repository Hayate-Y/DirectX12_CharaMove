//�O���[�o��
cbuffer global
{
	matrix g_w;//���[���h�s��
	matrix g_wvp; //���[���h����ˉe�܂ł̕ϊ��s��
	float4 g_lightDir;//���C�g�̕����x�N�g��	
	float4 g_eye;	//�J�����i���_�j
	float4 g_ambient;//����
	float4 g_diffuse;//�g�U���ˌ�
	float4 g_specular;//���ʔ��ˌ�
};

Texture2D g_Tex: register(t0);
SamplerState g_Sampler : register(s0);

//�\����
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 light : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 eyeVector : TEXCOORD2;
	float2 uv : TEXCOORD3;
};
//�o�[�e�b�N�X�V�F�[�_�[
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
//�s�N�Z���V�F�[�_�[
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