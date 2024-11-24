struct ObjectConstants
{
    float4x4 gWorlViewProj;
    uint     matIndex;
    float    gTime;
    float4   gColor;
};
ConstantBuffer<ObjectConstants> gObjectConstants : register(b0);

struct VertexIn
{
    float4 Color : COLOR;
    float3 PosL  : POSITION;
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

float easeInOut(float t)
{
    return t * t * (3 - 2 * t);
}

VertexOut VS(VertexIn vin)
{
    // vin.PosL.xy += .5f * sin(vin.PosL.x) * sin(3.f * gObjectConstants.gTime);
    // vin.PosL.z  *= .6f * .4f * sin(2.f * gObjectConstants.gTime);

    VertexOut vout;
    vout.PosH  = mul(float4(vin.PosL, 1.f), gObjectConstants.gWorlViewProj);

    // float factor = 0.5f * sin(gObjectConstants.gTime) + 0.5f;
    // factor = easeInOut(factor);
    // vout.Color = lerp(vin.Color, gObjectConstants.gColor, factor);
    vout.Color = vin.Color;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // clip(pin.Color.r - 0.5f);
    const float pi = 3.14159;
    float s = 0.5f * sin(2 * gObjectConstants.gTime - 0.25f * pi) + 0.5f;
    float4 c = lerp(pin.Color, gObjectConstants.gColor, s);
    return c;
}