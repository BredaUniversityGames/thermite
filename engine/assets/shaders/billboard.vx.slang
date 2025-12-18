import view;

struct Billboard {
    float4 color;
    float3 pos;
    float size;
};

struct VertexInput {
    float2 quad_pos;
};

struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR0;
};

ConstantBuffer<RenderView> gpu_view;

StructuredBuffer<Billboard> billboards;

[shader("vertex")]
VertexOutput main(VertexInput input, uint instance_id: SV_InstanceID) {
    VertexOutput output;

    float3 cam_right = normalize(float3(gpu_view.inv_view[0][0], gpu_view.inv_view[1][0], gpu_view.inv_view[2][0]));
    float3 cam_up = normalize(float3(gpu_view.inv_view[0][1], gpu_view.inv_view[1][1], gpu_view.inv_view[2][1]));

    Billboard b = billboards[instance_id];
    float3 world_pos = b.pos +  
                       cam_right * input.quad_pos.x * b.size + 
                       cam_up * input.quad_pos.y * b.size;

    output.pos = mul(gpu_view.world_to_clip, float4(world_pos, 1.0f));
    output.color = b.color;

    return output;
}