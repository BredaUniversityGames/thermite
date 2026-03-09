import colorspace;

/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* History buffer. */
RWTexture2D<float4> hbuffer;
/* Render target. */
RWTexture2D<float4> render_target;

[[vk::push_constant]]
uint32_t taa_flag;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 curr_luminance = lbuffer[thread_id];
    float3 prev_luminance = hbuffer[thread_id].rgb;

    float3 output = float3(0.0f, 0.0f, 0.0f);
    if (taa_flag == 1) {
        output = lerp(prev_luminance, curr_luminance, 0.05f);

        hbuffer[thread_id] = float4(output, 1.0f);
    } else {
        output = curr_luminance;
    }

    /* Convert from ACEScg to r709 */
    output = cs::acescg_to_r709(output);

    /* Apply AGX tonemapping */
    output = cs::agx(output);
    output = cs::agx_look(output);
    output = cs::agx_eotf(output);

    /* Output device transform */
    output = cs::delinearize(output);

    render_target[thread_id] = float4(output, 1.0f);
}
