/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per macrofacet]
 * ...
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
    uint samples;
};

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<HashCell> macrofacet_illuminance_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;

/* TEMPORARY */
static const float3 SUN_LUMINANCE = float3(1.0f, 0.1f, 0.1f);
static const float3 AO_LUMINANCE = float3(1.0f, 0.1f, 0.1f) * 0.25f;
static const float3 LIGHT_DIR = normalize(float3(1.0f, 1.5f, 1.1f));
/* TEMPORARY */

float3 sample_cone(const float3 dir, const float half_angle, const float2 uv) {
    const float cos_theta = 1.0f - uv.x * (1.0f - half_angle);
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    const float phi = 2.0f * 3.14f * uv.y;

    const float3 local_dir = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
    const Basis basis = orthonormal_basis(dir);
    return normalize(local_dir.x * basis.a + local_dir.y * basis.b + local_dir.z * dir);
}

float2 r2(const uint n) {
    const float g = 1.32471795724f;
    const float a1 = 1.0f / g;
    const float a2 = 1.0f / (g * g);
    return frac(float2(a1, a2) * n + 0.5f);
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

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Unpack visibility data */
    const VisData v = unpack_visibility(facet_key);
    const GeoData geo = unpack_geometry(v, objects, ray);
    const float3 hit_point = ray.origin + ray.dir * geo.depth;

    /* Calculate light direction and distance */
    // const float3 light_vec = LIGHT_POS - hit_point;
    // const float light_dist_sq = dot(light_vec, light_vec);
    // const float light_dist = sqrt(light_dist_sq);
    // const float3 light_dir = light_vec / light_dist;

    const uint n = thread_id.x + thread_id.y * render_view.resolution.x + render_view.frame_index * render_view.resolution.x * render_view.resolution.y;
    const float3 ao_dir = sample_hemisphere(geo.normal, r2(n % 512));
    const float3 di_dir = sample_cone(LIGHT_DIR, cos(0.2f), r2((n + 1) % 512));
    float3 illuminance = 0.0f;

    {
        /* Trace light occlusion */
        const Ray occlusion_ray = custom_ray(hit_point + geo.normal * 0.001f, ao_dir);
        const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 1e30f);
        if (t >= 1e30f) {
            illuminance += AO_LUMINANCE;
        }
    }

    // {
    //     /* Trace light occlusion */
    //     const Ray occlusion_ray = custom_ray(hit_point + geo.normal * 0.001f, di_dir);
    //     const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 1e30f);
    //     if (t >= 1e30f) {
    //         illuminance += SUN_LUMINANCE * max(0.0f, dot(geo.normal, di_dir));
    //     }
    // }
    const uint fixed_illuminance = (uint)(dot(illuminance, 1.0f) * 1e3f);

    /* Evaluate the voxel hash function */
    const uint capacity = 10000000u;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find or insert the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which already exists matching our key */
        if (macrofacet_illuminance_cache[slot].key == facet_key) {
            InterlockedAdd(macrofacet_illuminance_cache[slot].value, fixed_illuminance);
            InterlockedAdd(macrofacet_illuminance_cache[slot].samples, 1u);
            break;
        }

        /* If the current slot is empty */
        if (macrofacet_illuminance_cache[slot].key == EMPTY_KEY) {
            uint64_t original_key = 0u; /* Try to claim this slot */
            InterlockedCompareExchange(macrofacet_illuminance_cache[slot].key, EMPTY_KEY, facet_key, original_key);

            /* If instead this thread succeeded in claiming the slot */
            if (original_key == EMPTY_KEY) {
                InterlockedExchange(macrofacet_illuminance_cache[slot].value, fixed_illuminance);
                InterlockedExchange(macrofacet_illuminance_cache[slot].samples, 1u);
                break;
            }
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }
}
