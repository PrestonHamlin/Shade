cbuffer SceneConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};
cbuffer MeshConstants : register(b1)
{
    float4x4 ModelMatrix;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    // model/view/projection matrix
    float4x4 MVP = mul(mul(ModelMatrix, ViewMatrix), ProjectionMatrix);
    result.position = mul(position, MVP);

    // passthrough per-vertex color for interpolation
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
