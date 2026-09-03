#include "Common.hlsli"
#include "LambertFunctions.hlsli"

// 18 pre-computed Poisson disk offsets used for soft shadow edge filtering
static const uint PoissonSamplesCount = 18;
static const float2 PoissonSamples[18] =
{
    { -0.7393085f, 3.280662f },
    { -2.47004f, 2.328731f },
    { -0.6732481f, 1.042242f },
    { 0.6072469f, 1.525136f },
    { 0.9831414f, 3.14807f },
    { 1.894908f, 0.6981092f },
    { 0.5978739f, 0.0825575f },
    { 2.06167f, -0.6915861f },
    { -1.294738f, -0.2353872f },
    { 3.23345f, 1.27049f },
    { -2.976625f, 0.1078734f },
    { -1.566728f, -2.490001f },
    { 0.022746f, -1.93031f },
    { -2.484528f, -1.378844f },
    { 1.984003f, -2.342571f },
    { -0.2734823f, -3.234874f },
    { 3.35825f, -0.3363621f },
    { 2.090277f, 2.286526f }
};

// SSAO texture rendered in Pass 3 is bound on register t9
Texture2D ssaoTexture : register(t9);

PixelOutput main(ModelVertexToPixel input)
{
	PixelOutput result;

	float2 scaledUV = input.texCoord0;
	
	float3 toEye = normalize(CameraToWorld._m03_m13_m23 - input.worldPosition.xyz);
    float4 albedo = albedoTexture.Sample(defaultSampler, scaledUV).rgba;

	if (albedo.a <= AlphaTestThreshold)
	{
		discard;
		result.color = float4(0.f, 0.f, 0.f, 0.f);
		return result;
	}

	// Normal map decoding and TBN tangent space transform
	float3 normal = normalTexture.Sample(defaultSampler, scaledUV).xyy;
	normal.xy = 2.0f * normal.xy - 1.0f;
	normal.z = sqrt(1 - saturate(normal.x * normal.x + normal.y * normal.y));
	normal = normalize(normal);

	float3x3 TBN = float3x3(
		normalize(input.tangent.xyz),
		normalize(-input.binormal.xyz),
		normalize(input.normal.xyz)
	);

	TBN = transpose(TBN);
	float3 pixelNormal = normalize(mul(TBN, normal));

    float3 fx = fxTexture.Sample(defaultSampler, scaledUV).rgb;
    float emissive = fx.r;

    // SSAO sampling: sample occlusion value using screen-space UV coordinates
    float ssaoFactor = 1.0f;
    if (CustomShaderParameters.x > 0.5f)
    {
        float2 screenUV = input.position.xy / Resolution;
        ssaoFactor = ssaoTexture.Sample(defaultSampler, screenUV).r;
    }

    // Ambient light term modulated by SSAO factor (darkens corners and contact areas)
    float3 ambiance = ssaoFactor * AmbientLightColor.rgb * EvaluateAmbianceLambert(
		environmentTexture, pixelNormal, albedo.rgb
	);

    // Directional light evaluation
	float3 lightDir = -normalize(DirectionalLightToWorldTransform._m02_m12_m22);
	float3 directionalLight;
	if (DirectionalLightSoftness == 0.f)
	{
        directionalLight = EvaluateDirectionalLightLambert(
			albedo.rgb, pixelNormal,
			DirectionalLightColor.xyz, lightDir);
    }
	else 
	{
        directionalLight = EvaluateSoftDirectionalLightLambert(
			albedo.rgb, pixelNormal, DirectionalLightSoftness,
			DirectionalLightColor.xyz, lightDir);
	}

    // Directional shadow map evaluation with Poisson disk filtering
    float shadowFactor = 1.0f;
    if (CustomShaderParameters.x > 0.5f)
    {
        shadowFactor = PoissonSamplesCount;
        float4 pos = input.worldPosition;
        
        // Normal bias: offset world position along surface normal to eliminate shadow acne
        float normalBias = 1.f;
        pos.xyz += normalBias * normalize(input.normal.xyz);
        
        // Project world position into directional light shadow clip space
        float4 directionalLightProjectedPositionTemp = mul(DirectionalWorldToLightTransform, pos);
        float3 directionalLightProjectedPosition = directionalLightProjectedPositionTemp.xyz / directionalLightProjectedPositionTemp.w;
        
        float w;
        float h;
        directionalLightShadowMap.GetDimensions(w, h);
	
        // Sample shadow map across Poisson disk filter taps
        float2 scale = { 0.71f / w, 0.71f / h };
        for (int i = 0; i < PoissonSamplesCount; i++)
        {
            float2 adjustedPosition = 0.5f + float2(0.5f, -0.5f) * directionalLightProjectedPosition.xy + scale * PoissonSamples[i];
			
            if (clamp(adjustedPosition.x, -1.0, 1.0) == adjustedPosition.x &&
                clamp(adjustedPosition.y, -1.0, 1.0) == adjustedPosition.y)
            {
                float computedZ = directionalLightProjectedPosition.z;
                float shadowMapZ = directionalLightShadowMap.SampleLevel(defaultSampler, adjustedPosition, 0).r;

                float bias = 0.0005f;

                shadowFactor -= (computedZ > shadowMapZ + bias);
            }
        }
        shadowFactor /= PoissonSamplesCount;
        shadowFactor = saturate(shadowFactor);
    }
	
	float3 pointLights = 0;
	for(unsigned int p = 0; p < NumberOfLights; p++)
	{
		if (PointLights[p].radius == 0.f)
		{
            pointLights += EvaluatePointLightLambert(
				albedo.rgb, pixelNormal,
				PointLights[p].color.rgb, PointLights[p].range, PointLights[p].position.xyz,
				input.worldPosition.xyz);
		}
		else
		{
			pointLights += EvaluateSoftAreaLightLambert(
				albedo.rgb, pixelNormal,
				PointLights[p].color.rgb, PointLights[p].radius, PointLights[p].range, PointLights[p].position.xyz,
				input.worldPosition.xyz);
		}
    }
	
	// Combine direct sun lighting (shadowed) + ambient (occluded by SSAO) + point lights + emissive
	float3 emissiveAlbedo = albedo.rgb * emissive;
	float3 radiance = shadowFactor * directionalLight + ambiance + pointLights + emissiveAlbedo;

    result.color.rgb = radiance;
	result.color.a = albedo.a;
	return result;
}

