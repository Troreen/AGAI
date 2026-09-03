#include "PostprocessStructs.hlsli"
#include "common.hlsli"

// Screen-space search radius for ambient occlusion in pixels
static const float ScreenSpaceRadius = 64.f;

// 16 disk sample offsets used for estimating local ambient occlusion
static const uint SampleCount = 16;
static const float2 SampleDisk[16] =
{
    float2(0.5381f, 0.1856f),
    float2(-0.1379f, 0.8486f),
    float2(0.3371f, -0.5679f),
    float2(-0.6999f, -0.4051f),
    float2(0.0689f, -0.8547f),
    float2(0.7560f, 0.3069f),
    float2(-0.4144f, 0.7351f),
    float2(-0.3552f, -0.6312f),
    float2(0.2464f, 0.9046f),
    float2(-0.8795f, 0.2858f),
    float2(-0.2191f, -0.3662f),
    float2(0.6407f, -0.4859f),
    float2(0.1581f, -0.2798f),
    float2(-0.3533f, 0.6931f),
    float2(0.7818f, -0.1837f),
    float2(-0.5863f, -0.2588f)
};

// Z-prepass depth buffer bound to slot 1
Texture2D ZPrepassDepthTexture : register(t1);

// Interleaved Gradient Noise: generates high-frequency per-pixel noise to rotate sample disks and avoid banding
float InterleavedGradientNoise(float2 pixelPos)
{
    return frac(
        52.9829189f *
        frac(dot(pixelPos, float2(0.06711056f, 0.00583715f)))
    );
}

// Reconstructs view-space 3D coordinates (X, Y, Z) from screen UV and depth buffer value
float3 ReconstructViewPosition(float2 uv, float depth)
{
    float4 clipPos;
    clipPos.x = uv.x * 2.0f - 1.0f;
    clipPos.y = 1.0f - uv.y * 2.0f;
    clipPos.z = depth;
    clipPos.w = 1.0f;

    float4 viewPos = mul(ProjectionToCamera, clipPos);
    return viewPos.xyz / (abs(viewPos.w) > 1e-6f ? viewPos.w : 1e-6f);
}

float InverseSmoothstep(float u)
{
    // Exact analytic inverse of smoothstep: S(x) = 3x^2 - 2x^3 => x = 0.5 - sin(asin(1 - 2u) / 3)
    return 0.5f - sin(asin(clamp(1.0f - 2.0f * u, -1.0f, 1.0f)) / 3.0f);
}

PostProcessPixelOutput main(PostProcessVertexToPixel input)
{
    PostProcessPixelOutput output;

    float centerDepth = ZPrepassDepthTexture.Sample(DefaultSampler, input.uv).r;

    // Background / sky pixels have no occlusion
    if (centerDepth >= 1.0f)
    {
        output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        return output;
    }

    // Reconstruct view position of center pixel
    float3 centerPosVS = ReconstructViewPosition(input.uv, centerDepth);

    // Compute random rotation angle from per-pixel noise
    float2 pixelPos = input.uv * Resolution;
    float noise = InterleavedGradientNoise(pixelPos);
    float angle = noise * 6.28318530718f;
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    float2 uvRadius = float2(  
        ScreenSpaceRadius / Resolution.x,
        ScreenSpaceRadius / Resolution.y
    );

    float3 radiusPosVS = ReconstructViewPosition(input.uv + float2(uvRadius.x, 0.0f), centerDepth);
    float sphereRadius = length(radiusPosVS - centerPosVS);
    if (sphereRadius < 1e-5f)
    {
        output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        return output;
    }

    float occlusion = 0.0f;
    float weightSum = 0.0f;

    // Pairwise disk sampling: evaluate volume occupancy around the surface point
    for (uint i = 0; i < SampleCount; ++i)
    {
        float2 sampleOffset = SampleDisk[i];

        // 2D spatial importance sampling
        float r2 = saturate(dot(sampleOffset, sampleOffset));
        float r0 = max(sqrt(r2), 1e-5f);
        float2 dir = sampleOffset / r0;
        float remappedR = InverseSmoothstep(r2);
        sampleOffset = dir * remappedR;

        // Rotate offset by pixel noise angle
        float2 rotatedOffset = float2(
            sampleOffset.x * cosAngle - sampleOffset.y * sinAngle,
            sampleOffset.x * sinAngle + sampleOffset.y * cosAngle
        );

        float2 uvOffset = rotatedOffset * uvRadius;
        float2 uv[2] = { input.uv + uvOffset, input.uv - uvOffset };

        float dz[2] = { 0.0f, 0.0f };
        float halfDepth[2] = { 0.0f, 0.0f };
        float validity[2] = { 0.0f, 0.0f };

        int side;
        for (side = 0; side < 2; ++side)
        {
            float2 sampleUv = uv[side];
            if (sampleUv.x >= 0.0f && sampleUv.x <= 1.0f && sampleUv.y >= 0.0f && sampleUv.y <= 1.0f)
            {
                float depth = ZPrepassDepthTexture.SampleLevel(DefaultSampler, sampleUv, 0).r;
                if (depth < 1.0f)
                {
                    float3 delta = ReconstructViewPosition(sampleUv, depth) - centerPosVS;
                    float radialDist = length(delta.xy);
                    
                    float radialDistNorm = saturate(radialDist / sphereRadius);
                    float hd = max(sphereRadius * (1.0f - radialDistNorm), 1e-4f * sphereRadius);

                    dz[side] = delta.z;
                    halfDepth[side] = hd;
                    validity[side] = 1.0f - smoothstep(2.0f, 4.0f, -delta.z / sphereRadius);
                }
            }
        }

        if (validity[0] <= 0.0f || validity[1] <= 0.0f)
            continue;

        float combinedWeight = min(validity[0], validity[1]);
        
        for (side = 0; side < 2; ++side)
        {
            int otherSide = 1 - side;

            // Real sample on this side
            if (validity[side] > 0.0f)
            {
                float w = combinedWeight * validity[side];
                float occ = smoothstep(0.0f, 1.0f, (dz[side] + halfDepth[side]) / (2.0f * halfDepth[side]));
                occlusion += w * occ;
                weightSum += w;
            }
            
            // Mirrored sample from opposite side (fills in missing/occluded background geometry)
            float wMirror = combinedWeight * (1.0f - validity[side]);
            if (wMirror > 0.0f)
            {
                float occMirror = smoothstep(0.0f, 1.0f, (-dz[otherSide] + halfDepth[otherSide]) / (2.0f * halfDepth[otherSide]));
                occlusion += wMirror * occMirror;
                weightSum += wMirror;
            }
        }
    }

    float averageOccupancy = 0.5f;
    if (weightSum > 0.0f)
    {
        averageOccupancy = occlusion / weightSum;
    }

    // Map volume occupancy to ambient occlusion factor (1.0 = fully lit, 0.0 = fully occluded)
    float ao = saturate(2.f * averageOccupancy);
    ao = smoothstep(0.f, 1.f, ao);
    output.color = float4(ao, ao, ao, 1.0f);

    return output;
}

