// Converts a 4x3 horizontal cube cross image into the 6 faces of a cubemap.
// Supported layout (4 columns x 3 rows):
//   Row 0: [ empty,     +Y (Top),    empty,      empty     ]
//   Row 1: [ -X (Left), +Z (Front),  +X (Right), -Z (Back) ]
//   Row 2: [ empty,     -Y (Bottom), empty,      empty     ]

Texture2D crossTexture : register(t0);
RWTexture2DArray<float4> outputCubemap : register(u0);

cbuffer CrossConstants : register(b0)
{
    uint FaceResolution; // Pixel width/height of one face (image.width / 4)
    float3 Padding;
};

// Returns top-left tile origin in the 2D cross image for a given cubemap face
uint2 GetTileOffset(uint face, uint faceSize)
{
    uint2 tile = uint2(0, 0);
    switch (face)
    {
    case 0: tile = uint2(2, 1); break; // +X (Right)
    case 1: tile = uint2(0, 1); break; // -X (Left)
    case 2: tile = uint2(1, 0); break; // +Y (Top)
    case 3: tile = uint2(1, 2); break; // -Y (Bottom)
    case 4: tile = uint2(1, 1); break; // +Z (Front)
    case 5: tile = uint2(3, 1); break; // -Z (Back)
    }
    return tile * faceSize;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint face = dispatchThreadId.z;
    uint2 texel = dispatchThreadId.xy;

    if (texel.x >= FaceResolution || texel.y >= FaceResolution || face >= 6)
        return;

    uint2 offset = GetTileOffset(face, FaceResolution);
    uint2 srcCoords = offset + texel;

    float4 color = crossTexture.Load(int3(srcCoords, 0));
    outputCubemap[uint3(texel, face)] = color;
}
