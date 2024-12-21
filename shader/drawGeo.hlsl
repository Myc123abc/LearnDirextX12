struct Object {
  float4x4 world;
};
ConstantBuffer<Object> g_object : register(b0);

struct Frame {
  float4x4 viewProj;
};
ConstantBuffer<Frame> g_frame : register(b1);

struct VIn {
  float3 pos : POSITION;
  float4 col : COLOR;
};

struct VOut {
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

VOut vs(VIn vin) {
  VOut vout;
  float4 posWorld = mul(float4(vin.pos, 1.f), g_object.world);
  vout.pos = mul(posWorld, g_frame.viewProj);
  vout.col = vin.col;
  return vout;
}

float4 ps(VOut vin) : SV_Target {
  return vin.col;
}