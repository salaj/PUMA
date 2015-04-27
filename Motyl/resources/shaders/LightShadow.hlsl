Texture2D lightMap : register(t0);
Texture2D shadowMap : register(t1);
SamplerState colorSampler : register(s0);

cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2
{
	matrix projMatrix;
};

cbuffer cbLightVS : register(b3) //Vertex Shader constant buffer slot 3
{
	float4 lightPosVS;
};

cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0
{
	float4 surfaceColor;
};

cbuffer cbMapTransform : register(b1) //Pixel Shader constant buffer slot 1
{
	matrix mapMatrix;
};

cbuffer cbLightPS : register(b2) //Pixel Shader constant buffer slot 2
{
	float4 lightPosPS;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float4 worldPos : POSITION;
	float3 viewVec : TEXCOORD0;
	float3 lightVec : TEXCOORD1;
};

PSInput VS_Main(VSInput i)
{
	PSInput o = (PSInput)0;
	matrix worldView = mul(viewMatrix, worldMatrix);
	o.worldPos = float4(i.pos, 1.0f);
	o.worldPos = mul(worldMatrix, o.worldPos);


	float4 viewPos = mul(viewMatrix, o.worldPos);
	o.pos = mul(projMatrix, viewPos);

	o.norm = mul(worldView, float4(i.norm, 0.0f)).xyz;
	o.norm = normalize(o.norm);
	
	o.viewVec = normalize(-viewPos.xyz);
	o.lightVec = normalize((mul(viewMatrix, lightPosVS) - viewPos).xyz);
	return o;
}

static const float3 ambientColor = float3(0.3f, 0.3f, 0.3f);
static const float3 kd = 0.7f, m = 0.1f;
static const float4 defLightColor = float4(0.3f, 0.3f, 0.3f, 0.0f);

float4 PS_Main(PSInput i) : SV_TARGET
{
	//TODO: calculate texture coordinates

	float4 position = mul(mapMatrix, i.worldPos);
	position.x /= position.w;
	position.y /= position.w;
	position.z /= position.w;


	//TODO: determine light color based on light map, clipping plane and shadow map.
	float4 lightCol = lightMap.Sample(colorSampler, float2(position.x, position.y));
	float4 lightColor = lightCol < defLightColor ? defLightColor : lightCol;

		if (i.worldPos.y > lightPosPS.y)
			lightColor = defLightColor;

	float shadow = shadowMap.Sample(colorSampler, float2(position.x, position.y));
	if (shadow < position.z)
		lightColor = defLightColor;
	float3 viewVec = normalize(i.viewVec);
	float3 normal = normalize(i.norm);
	float3 lightVec = normalize(i.lightVec);
	float3 halfVec = normalize(viewVec + lightVec);
	float3 color = surfaceColor * ambientColor;

	
	color += lightColor.xyz * surfaceColor * kd * saturate(dot(normal, lightVec)) +
			 lightColor * lightColor.a * pow(saturate(dot(normal, halfVec)), m);
	return float4(saturate(color), surfaceColor.a);
}