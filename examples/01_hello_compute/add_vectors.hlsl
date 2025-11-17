RWStructuredBuffer<float> a : register(u0);
RWStructuredBuffer<float> b : register(u1);
RWStructuredBuffer<float> c : register(u2);

[numthreads(64, 1, 1)]
void main_cs(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint i = dispatchThreadID.x;

  c[i] = a[i] + b[i];
}