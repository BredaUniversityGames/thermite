/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per macrofacet]
 * ...
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;
import color;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
    uint state;
};

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet shading commands buffer. */
/* NOTE: First 4 uints are indirect dispatch args, after that uint64_t commands. */
StructuredBuffer<uint> macrofacet_shading_commands;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<HashCell> macrofacet_illuminance_cache;

/* TEMPORARY */
static const float3 SKY_LUMINANCE = float3(0.2f, 0.5f, 1.0f);
static const float3 SUN_LUMINANCE = float3(1.0f, 0.7f, 0.1f) * 10.0f;
// static const float3 LIGHT_POS = float3(2.0f, 12.0f, 4.0f);
/* TEMPORARY */

/* <https://www.pcg-random.org/> */
uint pcg_u32(inout uint s) {
    s = s * 747796405u + 2891336453u;
    const uint word = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (word >> 22u) ^ word;
}

/* <https://www.pcg-random.org/> */
float pcg_f32(inout uint s) {
    s = s * 747796405u + 2891336453u;
    const uint word = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return float((word >> 22u) ^ word) / 4294967296.0f;
}

float2 pcg2_f32(inout uint s) { return float2(pcg_f32(s), pcg_f32(s)); }

/* Inigo Quilez, Integer Hash - I, 2017 */
uint iqint1(uint n) {
    n = (n << 13) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return n;
}

float3 sample_cone(const float3 dir, const float half_angle, const float2 uv) {
    const float cos_theta = 1.0f - uv.x * (1.0f - half_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * 3.14f * uv.y;

    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
    const Basis basis = orthonormal_basis(dir);
    return normalize(local_dir.x * basis.a + local_dir.y * basis.b + local_dir.z * dir);
}

float3 sample_hemisphere(const float3 n, const float2 uv) {
    float r = sqrt(uv.x);
    float phi = 2.0f * 3.14f * uv.y;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(1.0f - uv.x);

    const Basis basis = orthonormal_basis(n);
    return normalize(x * basis.a + y * basis.b + z * n);
}

void integrate_illuminance(float3 sample, inout float3 cache) {
    const float baseHysteresis = 0.97;
    const float changeThreshold = 0.99;
    const float minHysteresis = 0.65;

    // Detect significant change in irradiance
    const float lumStored = dot(cache, float3(0.2126f, 0.7152f, 0.0722f));
    const float lumNew = dot(sample, float3(0.2126f, 0.7152f, 0.0722f));
    const float lumMax = max(lumStored, lumNew);

    const float changeMagnitude = abs(lumNew - lumStored) / max(lumMax, 0.001f);

    // Reduce hysteresis when large changes detected
    const float hysteresis = lerp(minHysteresis, baseHysteresis, saturate(1.0f - changeMagnitude / changeThreshold));

    // First frame handling (stored is zero/invalid)
    // if (lumStored < 0.0001f) hysteresis = 0.0f;
    cache = lerp(sample, cache, hysteresis);
}

void update_resevoir(inout Reservoir r, const float3 sample, const float w, const float r01) {
    // r.w_sum += w;
    // r.m += 1.0f;
    // if (r01 * r.w_sum < w) r.y = encode_rgb9e5(sample);

    float3 current = decode_rgb9e5(r.y);

    // Blend based on weight-adjusted alpha
    const float alpha = 0.05f;
    float effectiveAlpha = alpha * w / max(w, 0.0001);
    float3 blended = lerp(current, sample, effectiveAlpha);

    r.y = encode_rgb9e5(blended);

    r.m += 1.0f;
    r.w_sum += w;
}

void update_probe(inout uint y, inout uint m, float3 sample) {
    // float3 mean = decode_rgb9e5(y);
    // float3 moment = decode_rgb9e5(m);

    // // Variance from moments
    // float3 variance = max(moment - mean * mean, 0.0001f);
    // float avgVariance = dot(variance, 1.0f / 3.0f);

    // // High variance = more reactive
    // float alpha = lerp(0.0005f, 0.005f, saturate(sqrt(avgVariance) * 4.0f));

    // mean = lerp(mean, sample, alpha);
    // moment = lerp(moment, sample * sample, alpha);

    // y = encode_rgb9e5(mean);
    // m = encode_rgb9e5(moment);
    float3 mean = decode_rgb16(uint2(y, m));
    mean = lerp(mean, sample, 0.1f);
    const uint2 p = encode_rgb16(mean);
    y = p.x;
    m = p.y;
}

float2 r2_seq(uint index) {
    index = index % 4096u;
    const float a = (float)index * 0.7548776662f;
    const float b = (float)index * 0.5698402910f;
    return float2(frac(a), frac(b));
}

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= macrofacet_shading_commands[3]) return;

    /* Load macrofacet shading command */
    const uint64_t facet_key = asuint64(uint2(
        macrofacet_shading_commands[4 + thread_id * 2], 
        macrofacet_shading_commands[4 + thread_id * 2 + 1]
    ));

    /* Unpack visibility data */
    const VisData v = unpack_visibility(facet_key);
    const VoxelFacet facet = unpack_geometry(v, objects);

    // uint seed = voxel_hash(v.voxel_index, v.object_index, v.face_index) + render_view.frame_index;
    uint seed = (voxel_hash(v.voxel_index, v.object_index, v.face_index) * 1973u + render_view.frame_index * 9277u) | 1u;
    const Basis basis = orthonormal_basis(facet.normal);

    const float3 light_dir = normalize(float3(sin((float)render_view.frame_index * 0.00025f), 1.0f, cos((float)render_view.frame_index * 0.00025f)));

    float3 illuminance = 0.0f;
    for (int i = 0; i < 8; ++i) {
        /* Generate a random macrofacet sample location */
        const uint quadrant = pcg_u32(seed) & 3u; /* 0..4 */
        const float2 facet_ndc = (float2(quadrant & 1u, (quadrant >> 1) & 1u) - 0.5f) * UNITS_PER_VOXEL;

        {
            const float2 dir_uv = pcg2_f32(seed);
            const float3 sample_dir = sample_hemisphere(facet.normal, dir_uv);
            const float3 sample_origin = facet.center + basis.a * facet_ndc.x + basis.b * facet_ndc.y + sample_dir * UNITS_PER_VOXEL * 1.5f;

            /* Trace light occlusion */
            const Ray occlusion_ray = custom_ray(sample_origin, sample_dir);
            const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 32.0f);
            if (t >= 32.0f) illuminance += srgb_to_acescg(SKY_LUMINANCE);
        }

        {
            const float2 dir_uv = pcg2_f32(seed);
            const float3 sample_dir = sample_cone(light_dir, cos(0.1f), dir_uv);
            const float3 sample_origin = facet.center + basis.a * facet_ndc.x + basis.b * facet_ndc.y + sample_dir * UNITS_PER_VOXEL * 1.5f;

            /* Trace light occlusion */
            const Ray occlusion_ray = custom_ray(sample_origin, sample_dir);
            const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, BIG_F32);
            if (t >= BIG_F32) illuminance += srgb_to_acescg(SUN_LUMINANCE) * saturate(dot(facet.normal, sample_dir));
        }
    }
    illuminance /= 8.0f;

    /* Calculate light direction and distance */
    // const float3 light_vec = LIGHT_POS - facet.center;
    // const float light_dist_sq = dot(light_vec, light_vec);
    // const float light_dist = sqrt(light_dist_sq);
    // const float3 light_dir = light_vec / light_dist;

    /* Trace light occlusion */
    // const Ray occlusion_ray = custom_ray(facet.center + facet.normal * 0.01f, light_dir);
    // const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, light_dist);
    // uint packed_illuminance = 0u;
    // if (t >= light_dist) {
    //     /* Evaluate direct illumination */
    //     const float3 illuminance = LIGHT_LUMINANCE * max(0.0f, dot(facet.normal, light_dir)) / light_dist_sq;
    //     packed_illuminance = encode_rgb9e5(illuminance);
    // }

    /* Evaluate the voxel hash function */
    const uint capacity = 10000000u;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find or insert the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (macrofacet_illuminance_cache[slot].key == facet_key) {
            // macrofacet_illuminance_cache[slot].value = packed_illuminance;
            break;
        }

        /* If the current slot is empty */
        if (macrofacet_illuminance_cache[slot].key == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(macrofacet_illuminance_cache[slot].key, EMPTY_KEY, facet_key, original_key);

            /* If instead this thread succeeded in claiming the slot */
            if (original_key == EMPTY_KEY) {
                // macrofacet_illuminance_cache[slot].value = packed_illuminance;
                // macrofacet_illuminance_cache[slot].value = 0u;
                // macrofacet_illuminance_cache[slot].state = 0u;
                break;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }

    /* Update illuminance cache */ 
    // float3 cached_illuminance = decode_rgb9e5(macrofacet_illuminance_cache[slot].value);

    // Reservoir r = decode_reservoir(macrofacet_illuminance_cache[slot].state, macrofacet_illuminance_cache[slot].value);
    // update_resevoir(r, illuminance, 1.0f, pcg(seed));
    // macrofacet_illuminance_cache[slot].value = r.y;
    // macrofacet_illuminance_cache[slot].state = encode_reservoir(r);

    update_probe(macrofacet_illuminance_cache[slot].value, macrofacet_illuminance_cache[slot].state, illuminance);
}
