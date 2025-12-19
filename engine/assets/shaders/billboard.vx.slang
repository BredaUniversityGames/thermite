import view;

struct Particle {
    float3 pos;
    float age;

    float3 velocity;
    float lifetime;
};

struct VertexInput {
    float2 pos;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR0;
};

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<Particle> particles;
StructuredBuffer<uint32_t> alive_list;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t instance_id: SV_InstanceID) {
    VertexOutput output;

    Particle particle = particles[alive_list[instance_id]];

    float3 cam_right = normalize(float3(gpu_view.inv_view[0][0], gpu_view.inv_view[1][0], gpu_view.inv_view[2][0]));
    float3 cam_up = normalize(float3(gpu_view.inv_view[0][1], gpu_view.inv_view[1][1], gpu_view.inv_view[2][1]));

    float3 world_pos = particle.pos.xyz +  
                       cam_right * input.pos.x /** b.size*/ + 
                       cam_up * input.pos.y /** b.size*/;

    output.pos = mul(gpu_view.world_to_clip, float4(world_pos, 1.0f));
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return output;
}