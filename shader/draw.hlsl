struct ObjectInfo
{
    float4x4 world;
};
ConstantBuffer<ObjectInfo> g_objectInfo : register(b0);

struct FrameData 
{
    float4x4 view;
    float4x4 InvView;
    float4x4 proj;
    float4x4 InvProj;
    float4x4 viewProj;
    float4x4 InvViewProj;
    float3   cameraPosW;
    float    pad0; // TODO: try delete this, shader will auto alignment
    float2   renderTargetSize;
    float2   InvRenderTargetSize;
    float    nearZ;
    float    farZ;
    float    totalTime;
    float    deltaTime;
};
ConstantBuffer<FrameData> g_frameData : register(b1);

struct VertexIn
{
	float3 posLocal  : POSITION;
    float4 color     : COLOR;
};

struct VertexOut
{
	float4 posHomogeneous : SV_POSITION;
    float4 color          : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    float4 posWorld = mul(float4(vin.posLocal, 1.0f), g_objectInfo.world);
    vout.posHomogeneous = mul(posWorld, g_frameData.viewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.color = vin.color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.color;
}