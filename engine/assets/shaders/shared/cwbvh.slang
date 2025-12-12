module bvh2;

import view;
import voxel;

inline uint extract_byte(const uint i, const uint n) { return (i >> (n * 8)) & 0xFF; }
inline uint sign_extend_s8x4(const uint i) {
    // asm("prmt.b32 %0, %1, 0x0, 0x0000BA98;" : "=r"(v) : "r"(i)); // BA98: 1011`1010`1001`1000
    // with the given parameters, prmt will extend the sign to all bits in a byte.
    uint b0 = (i & 0b10000000000000000000000000000000u) != 0u ? 0xff000000u : 0u;
    uint b1 = (i & 0b00000000100000000000000000000000u) != 0u ? 0x00ff0000u : 0u;
    uint b2 = (i & 0b00000000000000001000000000000000u) != 0u ? 0x0000ff00u : 0u;
    uint b3 = (i & 0b00000000000000000000000010000000u) != 0u ? 0x000000ffu : 0u;
    return b0 + b1 + b2 + b3;  // probably can do better than this.
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
    const uint octinv = 7u - ((ray.dir.x < 0.0f ? 4u : 0u) | (ray.dir.y < 0.0f ? 2u : 0u) | (ray.dir.z < 0.0f ? 1u : 0u));
    uint2 node_group = 0u, prim_group = 0u;
    uint stack_ptr = 0u;

    for (;;) {
        if (DEBUG) hit.steps += 1;

        /* If there's nodes in the current group to intersect */
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

            const uint slot_index = (child_bit_index - 24u) ^ octinv;
            const uint octinv4 = octinv * 0x01010101u;
            const uint relative_index = countbits(imask & ~(0xFFFFFFFFu << slot_index));
            const uint child_node_index = child_base_index + relative_index;

            /* Fetch the compressed node data */
            const float4 n0 = nodes[child_node_index * 5u + 0u];
            const float4 n1 = nodes[child_node_index * 5u + 1u];
            const float4 n2 = nodes[child_node_index * 5u + 2u];
            const float4 n3 = nodes[child_node_index * 5u + 3u];
            const float4 n4 = nodes[child_node_index * 5u + 4u];

            const float3 p = n0.xyz;
            const int3 e = int3(
                (asint(n0.w) >> 24u) & 0xFFu,
                (asint(n0.w) >> 16u) & 0xFFu,
                (asint(n0.w) >> 8u ) & 0xFFu
            );
        }
    }

    // Hit hit = Hit::miss();
    // uint2 traversalStack[32];
    // uint hitAddr = 0, stackPtr = 0;
    // float tmin = 0, tmax = 1e30f;
    // const uint octinv = (7 - ((ray.dir.x < 0 ? 4 : 0) | (ray.dir.y < 0 ? 2 : 0) | (ray.dir.z < 0 ? 1 : 0))) * 0x1010101;
    // uint2 ngroup = uint2(0, 0b10000000000000000000000000000000), tgroup = uint2(0);
    // for (uint qjk = 0u; qjk < 64u; ++qjk) {
    //     hit.steps += 1;
    //     if (ngroup.y > 0x00FFFFFF) {
    //         const uint hits = ngroup.y; 
    //         uint imask = ngroup.y;
    //         const uint child_bit_index = firstbithigh(hits), child_node_base_index = ngroup.x;
    //         ngroup.y &= ~(1 << child_bit_index);
    //         if (ngroup.y > 0x00FFFFFF) { traversalStack[stackPtr++] = ngroup; }
    //         {
    //             const uint slot_index = (child_bit_index - 24) ^ (octinv & 255);
    //             const uint relative_index = countbits(imask & ~(0xFFFFFFFF << slot_index));
    //             const uint child_node_index = child_node_base_index + relative_index;
    //             const float4 n0 = nodes[child_node_index * 5 + 0], n1 = nodes[child_node_index * 5 + 1];
    //             const float4 n2 = nodes[child_node_index * 5 + 2], n3 = nodes[child_node_index * 5 + 3];
    //             const float4 n4 = nodes[child_node_index * 5 + 4], p = n0;
    //             int3 e;
    //             // e.x = (int32_t) * ((int8_t*)&n0.w + 0), e.y = (int32_t) * ((int8_t*)&n0.w + 1), e.z = (int32_t) * ((int8_t*)&n0.w + 2);
    //             e.x = (asint(n0.w) >> 0) & 0xFF;
    //             e.y = (asint(n0.w) >> 8) & 0xFF;
    //             e.z = (asint(n0.w) >> 16) & 0xFF;
    //             ngroup.x = asuint(n1.x);
    //             tgroup.x = asuint(n1.y); 
    //             tgroup.y = 0;
    //             uint hitmask = 0;
    //             const uint vx = (e.x + 127) << 23u; const float adjusted_idirx = asfloat(vx) * ray.rcp_dir.x;
    //             const uint vy = (e.y + 127) << 23u; const float adjusted_idiry = asfloat(vy) * ray.rcp_dir.y;
    //             const uint vz = (e.z + 127) << 23u; const float adjusted_idirz = asfloat(vz) * ray.rcp_dir.z;
    //             const float origx = -(ray.origin.x - p.x) * ray.rcp_dir.x;
    //             const float origy = -(ray.origin.y - p.y) * ray.rcp_dir.y;
    //             const float origz = -(ray.origin.z - p.z) * ray.rcp_dir.z;
    //             {  // First 4
    //                 const uint meta4 = asuint(n1.z), is_inner4 = (meta4 & (meta4 << 1)) & 0x10101010;
    //                 const uint inner_mask4 = sign_extend_s8x4(is_inner4 << 3);
    //                 const uint bit_index4 = (meta4 ^ (octinv & inner_mask4)) & 0x1F1F1F1F;
    //                 const uint child_bits4 = (meta4 >> 5) & 0x07070707;
    //                 uint swizzledLox = (ray.rcp_dir.x < 0) ? asuint(n3.z) : asuint(n2.x), swizzledHix = (ray.rcp_dir.x < 0) ? asuint(n2.x) : asuint(n3.z);
    //                 uint swizzledLoy = (ray.rcp_dir.y < 0) ? asuint(n4.x) : asuint(n2.z), swizzledHiy = (ray.rcp_dir.y < 0) ? asuint(n2.z) : asuint(n4.x);
    //                 uint swizzledLoz = (ray.rcp_dir.z < 0) ? asuint(n4.z) : asuint(n3.x), swizzledHiz = (ray.rcp_dir.z < 0) ? asuint(n3.x) : asuint(n4.z);
    //                 float tminx[4], tminy[4], tminz[4], tmaxx[4], tmaxy[4], tmaxz[4];
    //                 tminx[0] = ((swizzledLox >> 0) & 0xFF) * adjusted_idirx + origx; tminx[1] = ((swizzledLox >> 8) & 0xFF) * adjusted_idirx + origx; tminx[2] = ((swizzledLox >> 16) & 0xFF) * adjusted_idirx + origx;
    //                 tminx[3] = ((swizzledLox >> 24) & 0xFF) * adjusted_idirx + origx; tminy[0] = ((swizzledLoy >> 0) & 0xFF) * adjusted_idiry + origy; tminy[1] = ((swizzledLoy >> 8) & 0xFF) * adjusted_idiry + origy;
    //                 tminy[2] = ((swizzledLoy >> 16) & 0xFF) * adjusted_idiry + origy; tminy[3] = ((swizzledLoy >> 24) & 0xFF) * adjusted_idiry + origy; tminz[0] = ((swizzledLoz >> 0) & 0xFF) * adjusted_idirz + origz;
    //                 tminz[1] = ((swizzledLoz >> 8) & 0xFF) * adjusted_idirz + origz; tminz[2] = ((swizzledLoz >> 16) & 0xFF) * adjusted_idirz + origz; tminz[3] = ((swizzledLoz >> 24) & 0xFF) * adjusted_idirz + origz;
    //                 tmaxx[0] = ((swizzledHix >> 0) & 0xFF) * adjusted_idirx + origx; tmaxx[1] = ((swizzledHix >> 8) & 0xFF) * adjusted_idirx + origx; tmaxx[2] = ((swizzledHix >> 16) & 0xFF) * adjusted_idirx + origx;
    //                 tmaxx[3] = ((swizzledHix >> 24) & 0xFF) * adjusted_idirx + origx; tmaxy[0] = ((swizzledHiy >> 0) & 0xFF) * adjusted_idiry + origy; tmaxy[1] = ((swizzledHiy >> 8) & 0xFF) * adjusted_idiry + origy;
    //                 tmaxy[2] = ((swizzledHiy >> 16) & 0xFF) * adjusted_idiry + origy; tmaxy[3] = ((swizzledHiy >> 24) & 0xFF) * adjusted_idiry + origy; tmaxz[0] = ((swizzledHiz >> 0) & 0xFF) * adjusted_idirz + origz;
    //                 tmaxz[1] = ((swizzledHiz >> 8) & 0xFF) * adjusted_idirz + origz; tmaxz[2] = ((swizzledHiz >> 16) & 0xFF) * adjusted_idirz + origz; tmaxz[3] = ((swizzledHiz >> 24) & 0xFF) * adjusted_idirz + origz;
    //                 for (int32_t i = 0; i < 4; i++) {
    //                     // Use VMIN, VMAX to compute the slabs
    //                     const float cmin = max(max(max(tminx[i], tminy[i]), tminz[i]), tmin);
    //                     const float cmax = min(min(min(tmaxx[i], tmaxy[i]), tmaxz[i]), tmax);
    //                     if (cmin <= cmax) hitmask |= extract_byte(child_bits4, i) << extract_byte(bit_index4, i);
    //                 }
    //             }
    //             {  // Second 4
    //                 const uint meta4 = asuint(n1.w), is_inner4 = (meta4 & (meta4 << 1)) & 0x10101010;
    //                 const uint inner_mask4 = sign_extend_s8x4(is_inner4 << 3);
    //                 const uint bit_index4 = (meta4 ^ (octinv & inner_mask4)) & 0x1F1F1F1F;
    //                 const uint child_bits4 = (meta4 >> 5) & 0x07070707;
    //                 uint swizzledLox = (ray.rcp_dir.x < 0) ? asuint(n3.w) : asuint(n2.y), swizzledHix = (ray.rcp_dir.x < 0) ? asuint(n2.y) : asuint(n3.w);
    //                 uint swizzledLoy = (ray.rcp_dir.y < 0) ? asuint(n4.y) : asuint(n2.w), swizzledHiy = (ray.rcp_dir.y < 0) ? asuint(n2.w) : asuint(n4.y);
    //                 uint swizzledLoz = (ray.rcp_dir.z < 0) ? asuint(n4.w) : asuint(n3.y), swizzledHiz = (ray.rcp_dir.z < 0) ? asuint(n3.y) : asuint(n4.w);
    //                 float tminx[4], tminy[4], tminz[4], tmaxx[4], tmaxy[4], tmaxz[4];
    //                 tminx[0] = ((swizzledLox >> 0) & 0xFF) * adjusted_idirx + origx; tminx[1] = ((swizzledLox >> 8) & 0xFF) * adjusted_idirx + origx; tminx[2] = ((swizzledLox >> 16) & 0xFF) * adjusted_idirx + origx;
    //                 tminx[3] = ((swizzledLox >> 24) & 0xFF) * adjusted_idirx + origx; tminy[0] = ((swizzledLoy >> 0) & 0xFF) * adjusted_idiry + origy; tminy[1] = ((swizzledLoy >> 8) & 0xFF) * adjusted_idiry + origy;
    //                 tminy[2] = ((swizzledLoy >> 16) & 0xFF) * adjusted_idiry + origy; tminy[3] = ((swizzledLoy >> 24) & 0xFF) * adjusted_idiry + origy; tminz[0] = ((swizzledLoz >> 0) & 0xFF) * adjusted_idirz + origz;
    //                 tminz[1] = ((swizzledLoz >> 8) & 0xFF) * adjusted_idirz + origz; tminz[2] = ((swizzledLoz >> 16) & 0xFF) * adjusted_idirz + origz; tminz[3] = ((swizzledLoz >> 24) & 0xFF) * adjusted_idirz + origz;
    //                 tmaxx[0] = ((swizzledHix >> 0) & 0xFF) * adjusted_idirx + origx; tmaxx[1] = ((swizzledHix >> 8) & 0xFF) * adjusted_idirx + origx; tmaxx[2] = ((swizzledHix >> 16) & 0xFF) * adjusted_idirx + origx;
    //                 tmaxx[3] = ((swizzledHix >> 24) & 0xFF) * adjusted_idirx + origx; tmaxy[0] = ((swizzledHiy >> 0) & 0xFF) * adjusted_idiry + origy; tmaxy[1] = ((swizzledHiy >> 8) & 0xFF) * adjusted_idiry + origy;
    //                 tmaxy[2] = ((swizzledHiy >> 16) & 0xFF) * adjusted_idiry + origy; tmaxy[3] = ((swizzledHiy >> 24) & 0xFF) * adjusted_idiry + origy; tmaxz[0] = ((swizzledHiz >> 0) & 0xFF) * adjusted_idirz + origz;
    //                 tmaxz[1] = ((swizzledHiz >> 8) & 0xFF) * adjusted_idirz + origz; tmaxz[2] = ((swizzledHiz >> 16) & 0xFF) * adjusted_idirz + origz; tmaxz[3] = ((swizzledHiz >> 24) & 0xFF) * adjusted_idirz + origz;
    //                 for (int32_t i = 0; i < 4; i++) {
    //                     const float cmin = max(max(max(tminx[i], tminy[i]), tminz[i]), tmin);
    //                     const float cmax = min(min(min(tmaxx[i], tmaxy[i]), tmaxz[i]), tmax);
    //                     if (cmin <= cmax) hitmask |= extract_byte(child_bits4, i) << extract_byte(bit_index4, i);
    //                 }
    //             }
    //             ngroup.y = (hitmask & 0xFF000000) | (asuint(n0.w) >> 24);
    //             tgroup.y = hitmask & 0x00FFFFFF;
    //         }
    //     } else {
    //         tgroup = ngroup; 
    //         ngroup = uint2(0);
    //     }
    //     while (tgroup.y != 0) {
    //         uint primIndex = firstbithigh(tgroup.y);
    //         tgroup.y -= (1u << primIndex);
    //         int32_t primAddr = tgroup.x + primIndex;
    //         const VoxelObject prim = prims[indices[primAddr]];

    //         // const float t = intersect_aabb(ray, prim.min, prim.max);
    //         const Hit prim_hit = intersect_voxel_object<DEBUG>(ray, prim);
    //         if (DEBUG) hit.steps += prim_hit.steps;
    //         // if (prim_hit.t < hit.t) {
    //         //     hit.t = prim_hit.t;
    //         //     hit.albedo = prim_hit.albedo;
    //         // }

    //         if (prim_hit.t < 0.0f || prim_hit.t >= tmax) continue;
    //         tmax = prim_hit.t;
    //     }
    //     if (ngroup.y > 0x00FFFFFF) continue;
    //     if (stackPtr > 0) { ngroup = traversalStack[--stackPtr]; }
    //     else {
    //         hit.t = tmax;
    //         break;
    //     }
    // }

    // return hit;
}
