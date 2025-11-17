RWStructuredBuffer<float> data_buffer : register(u0);

[numthreads(64, 1, 1)]
void main_B(uint3 dispatchThreadID : SV_DispatchThreadID)
{
  uint i = dispatchThreadID.x;
  data_buffer[i] = data_buffer[i] * 2.0f;
}