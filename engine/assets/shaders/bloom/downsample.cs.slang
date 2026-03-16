RWTexture2D<float4> lbuffer;

[[vk::push_constant]]
uint32_t curr_mip;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {

}
