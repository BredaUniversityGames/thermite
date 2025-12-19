import view;

struct Particle {
    float3 pos;
    float speed;

    float3 dir;
    float life;
};

static const float3 BILLBOARD[] = {
    float3(-1, -1, 0), // BL
    float3(1, -1, 0),  // BR
    float3(-1, 1, 0),  // TL
    float3(1, -1, 0),  // BR
    float3(1, 1, 0),   // TR
    float3(-1, 1, 0)   // TL
};

struct VertexInput {
    float4 pos;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR0;
};

ConstantBuffer<RenderView> gpu_view;

//StructuredBuffer<Particle> particles;
//StructuredBuffer<uint32_t> alive_list;

[shader("vertex")]
VertexOutput main(VertexInput input, uint32_t instance_id: SV_InstanceID, uint32_t fake_id : SV_VertexID) {
    VertexOutput output;

    //Particle particle = particles[alive_list[instance_id]];

    float3 cam_right = normalize(float3(gpu_view.inv_view[0][0], gpu_view.inv_view[1][0], gpu_view.inv_view[2][0]));
    float3 cam_up = normalize(float3(gpu_view.inv_view[0][1], gpu_view.inv_view[1][1], gpu_view.inv_view[2][1]));

    uint32_t vertex_id = fake_id;
    float3 quad_pos = BILLBOARD[vertex_id];

    float3 world_pos = input.pos.xyz +  
                       cam_right * quad_pos.x /** b.size*/ + 
                       cam_up * quad_pos.y /** b.size*/;

    output.pos = mul(gpu_view.world_to_clip, float4(world_pos, 1.0f));
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);

    return output;
}