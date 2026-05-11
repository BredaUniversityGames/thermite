import colorspace;
import visibility;
import math;

/* Luminance buffer. */
Texture2D<float3> lbuffer;
Texture2D<float3> tbuffer;
/* Render Target. */
RWTexture2D<float4> render_target;

float3 hue2rgb(float hue) {
    hue = fract(hue);
    return saturate(float3(abs(hue * 6.0f - 3.0f) - 1.0f, 2.0f - abs(hue * 6.0f - 2.0f), 2.0f - abs(hue * 6.0f - 4.0f)));
}

float3 rgb2hsl(float3 c) {
    float cMin = min(min(c.r, c.g), c.b), cMax = max(max(c.r, c.g), c.b), delta = cMax - cMin;
    float3 hsl = float3(0., 0., (cMax + cMin) / 2.);
    if (delta != 0.0) {                          // If it has chroma and isn't gray.
        if (hsl.z < .5) {
            hsl.y = delta / (cMax + cMin);       // Saturation.
        } else {
            hsl.y = delta / (2. - cMax - cMin);  // Saturation.
        }
        float deltaR = (((cMax - c.r) / 6.) + (delta / 2.)) / delta, deltaG = (((cMax - c.g) / 6.) + (delta / 2.)) / delta, deltaB = (((cMax - c.b) / 6.) + (delta / 2.)) / delta;
        // Hue.
        if (c.r == cMax) {
            hsl.x = deltaB - deltaG;
        } else if (c.g == cMax) {
            hsl.x = (1. / 3.) + deltaR - deltaB;
        } else {  // if(c.b==cMax){
            hsl.x = (2. / 3.) + deltaG - deltaR;
        }
        hsl.x = fract(hsl.x);
    }
    return hsl;
}

float3 hsl2rgb(float3 hsl) {
    if (hsl.y == 0.) {
        return float3(hsl.z);  // Luminance.
    } else {
        float b;
        if (hsl.z < .5) {
            b = hsl.z * (1. + hsl.y);
        } else {
            b = hsl.z + hsl.y - hsl.y * hsl.z;
        }
        float a = 2. * hsl.z - b;
        return a + hue2rgb(hsl.x) * (b - a);
    }
}

[[vk::push_constant]]
float3 hsl_packed;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float3 luminance = lbuffer[thread_id];
    luminance += tbuffer[thread_id];

    /* Convert from ACEScg to r709 */
    luminance = cs::acescg_to_r709(luminance);

    /* Apply AGX tonemapping */
    luminance = cs::agx(luminance);
    luminance = cs::agx_look(luminance);
    luminance = cs::agx_eotf(luminance);

    /* Output device transform */

    /* HSL editor */
    float3 hsl = rgb2hsl(luminance);

    hsl.x += hsl_packed.x;
    hsl.y *= hsl_packed.y;
    hsl.z *= hsl_packed.z;

    /* Wrap hue around */
    hsl.x = fmod(hsl.x, 1.0f);

    luminance = hsl2rgb(hsl);
    luminance = cs::delinearize(luminance);

    render_target[thread_id] = float4(luminance, 1.0f);
}
