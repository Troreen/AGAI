#include "common.hlsli"

// Scene depth buffer bound to register t8 from Pass 1
Texture2D depthTexture : register(t8);

// Reconstructs 3D world position of underlying geometry using screen UV and scene depth
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos;
    clipPos.x = uv.x * 2.0f - 1.0f;
    clipPos.y = 1.0f - uv.y * 2.0f;
    clipPos.z = depth;
    clipPos.w = 1.0f;

    // Unproject clip space to view space
    float4 viewPos = mul(ProjectionToCamera, clipPos);
    viewPos.xyz /= (abs(viewPos.w) > 1e-6f ? viewPos.w : 1e-6f);

    // Transform view space to world space
    float4 worldPos = mul(CameraToWorld, float4(viewPos.xyz, 1.0f));
    return worldPos.xyz;
}

// Projected Drop Shadow Shader:
// This shader is drawn on a bounding cube proxy centered at the player's position.
// For every pixel of the cube, it samples the ground depth behind it, reconstructs the ground's
// world position, and paints a soft circular shadow. This conforms to uneven ground without shadow mapping.
PixelOutput main(ModelVertexToPixel input)
{
    PixelOutput result;
    result.color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Sample scene depth of the surface behind this pixel
    float2 screenUV = input.position.xy / Resolution;
    float depth = depthTexture.SampleLevel(defaultSampler, screenUV, 0).r;

    if (depth >= 1.0f)
    {
        discard;
        return result;
    }

    // Reconstruct world position of the ground surface beneath the player
    float3 groundWorldPos = ReconstructWorldPosition(screenUV, depth);
    float3 shadowCenter = mul(ObjectToWorld, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;

    float radius = CustomShaderParameters.x > 0.0f ? CustomShaderParameters.x : 150.0f;
    float opacity = CustomShaderParameters.y > 0.0f ? CustomShaderParameters.y : 0.6f;

    // Calculate distance from shadow center to ground point
    float3 delta = groundWorldPos - shadowCenter;
    float dist = length(delta);

    if (dist >= radius)
    {
        discard;
        return result;
    }

    // Smooth radial falloff from center (dark) to radius edge (transparent)
    float normalizedDist = dist / radius;
    float falloff = 1.0f - smoothstep(0.0f, 1.0f, normalizedDist);

    float shadowAlpha = falloff * opacity;

    result.color = float4(0.0f, 0.0f, 0.0f, shadowAlpha);
    return result;
}

