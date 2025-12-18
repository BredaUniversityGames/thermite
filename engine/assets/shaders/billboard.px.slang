struct VertexOutput {
    float4 pos : SV_Position;
    float4 color : COLOR0;
};

[shader("fragment")]
float4 main(VertexOutput input) : SV_Target0 {
    return float4(input.color);
}