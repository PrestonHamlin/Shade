cbuffer SceneConstantBuffer : register(b0)
{
    float4 angles;
    float4x4 MVP;
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
    result.position = mul(position, MVP);

    // passthrough per-vertex color for interpolation
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
