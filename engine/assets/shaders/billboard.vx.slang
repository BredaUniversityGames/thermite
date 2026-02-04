import view;
import particles;

struct VertexInput {
    float2 pos;
    float2 uv;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    uint32_t tex_index : TEXINDEX0;
};

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<Particle> particles;
StructuredBuffer<uint32_t> alive_list;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t instance_id: SV_InstanceID) {
    VertexOutput output;

    Particle particle = particles[alive_list[instance_id]];

    float3 cam_right = normalize(float3(gpu_view.view_to_world[0][0], gpu_view.view_to_world[1][0], gpu_view.view_to_world[2][0]));
    float3 cam_up = normalize(float3(gpu_view.view_to_world[0][1], gpu_view.view_to_world[1][1], gpu_view.view_to_world[2][1]));

    float3 world_pos = particle.pos.xyz +  
                       cam_right * input.pos.x /** b.size*/ + 
                       cam_up * input.pos.y /** b.size*/;

    output.pos = mul(gpu_view.world_to_clip, float4(world_pos, 1.0f));
    output.uv = input.uv;
    output.tex_index = particle.tex_index;

    return output;
}