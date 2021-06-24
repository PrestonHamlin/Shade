cbuffer SceneConstantBuffer : register(b0)
{
    float4 angles;
    matrix MVP;
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
    result.position = mul(MVP, position);
    //result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
