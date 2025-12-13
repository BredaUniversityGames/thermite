module bvh2;

import view;
import voxel;

struct CwBvhNode {
    float4 cqf0; /* .xyz = Minimum bounds ~ .w = Log extent & Intersection mask */
    float4 cqf1; /* .x = Child base index ~ .y = Prim base index ~ .zw = Child meta data */
    float4 cqf2; /* .xy = Quantized min x ~ .zw = Quantized min y */
    float4 cqf3; /* .xy = Quantized min z ~ .zw = Quantized max x */
    float4 cqf4; /* .xy = Quantized max y ~ .zw = Quantized max z */
};

inline uint extract_byte(const uint a, const uint b) { return (a >> (b * 8u)) & 0xFFu; }
inline int extract_byte(const int a, const int b) { return (a >> (b * 8u)) & 0xFFu; }

inline uint sign_extend_s8x4(const uint i) {
    // asm("prmt.b32 %0, %1, 0x0, 0x0000BA98;" : "=r"(v) : "r"(i)); // BA98: 1011`1010`1001`1000
    // with the given parameters, prmt will extend the sign to all bits in a byte.
    uint b0 = (i & 0b10000000000000000000000000000000u) != 0u ? 0xff000000u : 0u;
    uint b1 = (i & 0b00000000100000000000000000000000u) != 0u ? 0x00ff0000u : 0u;
    uint b2 = (i & 0b00000000000000001000000000000000u) != 0u ? 0x0000ff00u : 0u;
    uint b3 = (i & 0b00000000000000000000000010000000u) != 0u ? 0x000000ffu : 0u;
    return b0 + b1 + b2 + b3;  // probably can do better than this.
}

inline uint ray_octant_inv4(float3 dir) {
    return (dir.x < 0.0f ? 0u : 0x04040404u) |
           (dir.y < 0.0f ? 0u : 0x02020202u) |
           (dir.z < 0.0f ? 0u : 0x01010101u);
}

/* Trace a ray through a CWBVH. */
public inline Hit trace_cwbvh<let DEBUG : bool>(
    StructuredBuffer<float4> nodes,
    StructuredBuffer<uint> indices,
    StructuredBuffer<VoxelObject> prims,
    Ray ray
) {
    Hit hit = Hit::miss();

    /* Traversal state */
    uint2 stack[32];
    uint2 node_group = uint2(0u, 0b10000000000000000000000000000000u);
    uint2 prim_group = 0u;
    uint stack_ptr = 0u;

    const uint inv_oct4 = ray_octant_inv4(ray.dir);

    for (;;) {
        if (DEBUG) hit.steps += 1;

        /* If there's nodes in the current group to intersect */
        /* Nodes are in the 8 most significant bits of the node_group.y */
        if (node_group.y > 0x00FFFFFFu) {
            const uint imask = node_group.y;

            /* Find the index of the child node and get its base index */
            const uint child_bit_index = firstbithigh(imask);
            const uint child_base_index = node_group.x;

            /* Update the node group intersection mask */
            node_group.y &= ~(1u << child_bit_index);

            /* If there's more nodes to intersect in the current group */
            if (node_group.y > 0x00FFFFFFu) {
                /* Push this group back onto the stack */
                stack[stack_ptr++] = node_group;
            }

            /* Find the node index of the child node */
            const uint child_index = child_bit_index - 24u;
            const uint relative_index = countbits(imask & ~(0xFFFFFFFFu << (child_index ^ (inv_oct4 & 0xFFu))));
            const uint node_index = child_base_index + relative_index;

            /* Fetch the compressed quad float node data */
            const float4 cqf0 = nodes[node_index * 5u + 0u]; /* .xyz = Minimum bounds ~ .w = Log extent & Intersection mask */
            const float4 cqf1 = nodes[node_index * 5u + 1u]; /* .x = Child base index ~ .y = Prim base index ~ .zw = Child meta data */
            const float4 cqf2 = nodes[node_index * 5u + 2u]; /* .xy = Quantized min x ~ .zw = Quantized min y */
            const float4 cqf3 = nodes[node_index * 5u + 3u]; /* .xy = Quantized min z ~ .zw = Quantized max x */
            const float4 cqf4 = nodes[node_index * 5u + 4u]; /* .xy = Quantized max y ~ .zw = Quantized max z */

            /* Extract the minimum bounds and log extent of the node */
            const float3 min_bounds = cqf0.xyz;
            const uint3 log_extent = uint3(
                // (asint(cqf0.w) >> 24u) & 0xFFu,
                // (asint(cqf0.w) >> 16u) & 0xFFu,
                // (asint(cqf0.w) >> 8u ) & 0xFFu
                extract_byte(asuint(cqf0.w), 0u),
                extract_byte(asuint(cqf0.w), 1u),
                extract_byte(asuint(cqf0.w), 2u)
            );

            uint hitmask = 0u;

            /* Transform the ray into local quantized space */
            const float3 lq_rcp_dir = asfloat((log_extent + 127) << 23) * ray.rcp_dir;
            const float3 lq_origin = (min_bounds - ray.origin) * ray.rcp_dir;

            [ForceUnroll] /* Process 4 nodes at a time */
            for (int i = 0; i < 2; i++) {
                const uint meta4 = asuint((i == 0) ? cqf1.z : cqf1.w);

                const uint is_inner4 = (meta4 & (meta4 << 1u)) & 0x10101010u;
                const uint inner_mask4 = (is_inner4 >> 4u) * 0xFFu;
                const uint bit_index4 = (meta4 ^ (inv_oct4 & inner_mask4)) & 0x1F1F1F1Fu;
                const uint child_bits4 = (meta4 >> 5u) & 0x07070707u;

                // const uint meta4 = asuint((i == 0) ? cqf1.z : cqf1.w);
                // const uint is_inner4 = (meta4 & (meta4 << 1u)) & 0x10101010u;
                // const uint inner_mask4 = sign_extend_s8x4(is_inner4 << 3u);
                // const uint bit_index4 = (meta4 ^ (inv_oct4 & inner_mask4)) & 0x1F1F1F1Fu;
                // const uint child_bits4 = (meta4 >> 5u) & 0x07070707u;

                /* Load quantized low and high bounds for child nodes */
                const uint q_lo_x = asuint((i == 0) ? cqf2.x : cqf2.y);
                const uint q_lo_y = asuint((i == 0) ? cqf2.z : cqf2.w);
                const uint q_lo_z = asuint((i == 0) ? cqf3.x : cqf3.y);
                const uint q_hi_x = asuint((i == 0) ? cqf3.z : cqf3.w);
                const uint q_hi_y = asuint((i == 0) ? cqf4.x : cqf4.y);
                const uint q_hi_z = asuint((i == 0) ? cqf4.z : cqf4.w);

                /* Select min and max based on ray octant */
                const uint x_min = (ray.dir.x < 0.0f) ? q_hi_x : q_lo_x;
                const uint x_max = (ray.dir.x < 0.0f) ? q_lo_x : q_hi_x;
                const uint y_min = (ray.dir.y < 0.0f) ? q_hi_y : q_lo_y;
                const uint y_max = (ray.dir.y < 0.0f) ? q_lo_y : q_hi_y;
                const uint z_min = (ray.dir.z < 0.0f) ? q_hi_z : q_lo_z;
                const uint z_max = (ray.dir.z < 0.0f) ? q_lo_z : q_hi_z;

                [ForceUnroll] /* Perform ray AABB intersection for each child node */
                for (int j = 0; j < 4; j++) {
                    const float3 bmin = float3(extract_byte(x_min, j), extract_byte(y_min, j), extract_byte(z_min, j));
                    const float3 bmax = float3(extract_byte(x_max, j), extract_byte(y_max, j), extract_byte(z_max, j));
                    const float3 t_to_min = bmin * lq_rcp_dir + lq_origin;
                    const float3 t_to_max = bmax * lq_rcp_dir + lq_origin;
                    const float tmin = max(max(max(t_to_min.x, t_to_min.y), t_to_min.z), 0.0f);
                    const float tmax = min(min(min(t_to_max.x, t_to_max.y), t_to_max.z), hit.t);

                    /* If we hit the AABB, update the hit mask */
                    if (tmin <= tmax) {
                        const uint child_bits = extract_byte(child_bits4, j);
                        const uint bit_index = extract_byte(bit_index4, j);
                        hitmask |= child_bits << bit_index;
                    }
                }
            }

            /* Store the child and primitive base indices inside the groups */
            node_group.x = asuint(cqf1.x); /* <- Child base index */
            prim_group.x = asuint(cqf1.y); /* <- Prim base index */

            /* Update the intersection mask stored in the node and primitive groups */
            node_group.y = (hitmask & 0xFF000000u) | extract_byte(asuint(cqf0.w), 3u);  // (asuint(cqf0.w) & 0xFFu);
            prim_group.y = hitmask & 0x00FFFFFFu;
        } else {
            /* No nodes left in the current group */
            prim_group = node_group;
            node_group = 0u;
        }

        /* Process all primitives in the primitive group */
        for (; prim_group.y != 0u;) {
            const uint local_prim_index = firstbithigh(prim_group.y);
            prim_group.y &= ~(1u << local_prim_index);

            const uint prim_index = prim_group.x + local_prim_index;
            const VoxelObject prim = prims[indices[prim_index]];

            const Hit prim_hit = intersect_voxel_object<DEBUG>(ray, prim);
            if (DEBUG) hit.steps++;

            if (prim_hit.t < hit.t) {
                hit.t = prim_hit.t;
            }
        }

        /* Pop the stack if there are entries left */
        if ((node_group.y & 0xFF000000u) == 0u) {
            if (stack_ptr == 0u) break;
            node_group = stack[--stack_ptr];
        }
    }

    return hit;
}
