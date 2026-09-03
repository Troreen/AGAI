#include "CubemapCommon.hlsli"

Texture2D panoramaTexture : register(t0);
SamplerState defaultSampler : register(s0);

RWTexture2DArray<float4> outputCubemap : register(u0);

cbuffer ConvConstants : register(b0)
{
    uint FaceResolution;
    float3 Padding;
};

// Convert 3D Cartesian direction to 2D equirectangular spherical UV coordinates
float2 DirectionToEquirectangularUV(float3 v)
{
    float phi = atan2(v.z, v.x);
    float theta = asin(clamp(v.y, -1.0f, 1.0f));

    float u = (phi / (2.0f * PI)) + 0.5f;
    float v_coord = 0.5f - (theta / PI);

    return float2(u, v_coord);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint face = dispatchThreadId.z;
    uint2 texel = dispatchThreadId.xy;

    if (texel.x >= FaceResolution || texel.y >= FaceResolution || face >= 6)
        return;

    float2 uv = (float2(texel) + 0.5f) / float(FaceResolution);
    uv = uv * 2.0f - 1.0f;

    float3 dir = GetCubeDirection(face, uv);
    float2 panoUV = DirectionToEquirectangularUV(dir);

    float4 color = panoramaTexture.SampleLevel(defaultSampler, panoUV, 0);
    outputCubemap[uint3(texel, face)] = color;
}
