module cache;

import visibility;
import voxel;

/* Cache information. (e.g. capacity) */
public struct CacheInfo {
    public uint capacity;
};

/* Cache entry data. */
public struct CacheEntry {
    public uint2 key;
    public uint occupancy;
    public uint last_used;
};

namespace cache {

/* Increment occupancy in the cache. */
/* Returns true if this was the first occurrence of the given key. */
public bool inc_occupancy(
    RWStructuredBuffer<CacheEntry> entries, 
    const ConstantBuffer<CacheInfo> info, 
    const uint2 key, const uint frame_index
) {
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(key);
    /* Evaluate the voxel hash function */
    uint hash = voxel_hash(v.voxel_index, v.object_index, v.face_index) % info.capacity;

    for (uint i = 0u; i < 8u; ++i) {
        // /* Check if the slot is free by comparing frame indices */
        // const bool is_free = (frame_index - entries[hash].last_used) > 10u;

        // /* Claim this cache entry if it is free */
        // if (is_free) entries[hash].key = key;

        // /* Update the cache entry */
        // if (is_free || all(entries[hash].key == key)) {
        //     InterlockedCompareExchange(,);
        //     entries[hash].last_used = frame_index;
        //     /* Atomic increment the occupancy counter */
        //     uint occupancy = 0u;
        //     InterlockedAdd(entries[hash].occupancy, 1u, occupancy);
        //     return occupancy == 0u;
        // }

        const uint stale_threshold = frame_index - 30u;
        uint original_frame;
        InterlockedMax(entries[hash].last_used, frame_index, original_frame);

        if (original_frame <= stale_threshold) {
            /* We claimed a stale slot, initialize it */
            entries[hash].key = key;
            entries[hash].occupancy = 1u;
            return true;
        } else if (all(entries[hash].key == key)) {
            /* Found existing match, increment it */
            uint occupancy;
            InterlockedAdd(entries[hash].occupancy, 1u, occupancy);
            return occupancy == 0u;
        }

        /* Move to the next hash cache slot */
        hash += 1u; if (hash >= info.capacity) hash = 0u;
    }

    return false; /* Unreachable */
}

/* Read from the cache. */
public CacheEntry read(
    RWStructuredBuffer<CacheEntry> entries, 
    const ConstantBuffer<CacheInfo> info, 
    const uint2 key
) {
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(key);
    /* Evaluate the voxel hash function */
    uint hash = voxel_hash(v.voxel_index, v.object_index, v.face_index) % info.capacity;

    for (uint i = 0u; i < 8u; ++i) {
        const uint2 entry_key = entries[hash].key;

        /* Check if the key in this entry matches */
        if (all(entry_key == key)) {
            return entries[hash];
        }

        /* Move to the next hash cache slot */
        hash += 1u; if (hash >= info.capacity) hash = 0u;
    }

    return CacheEntry({}, 100000000u); /* Unreachable */
}

/* Reset occupancy in the cache. */
/* Returns the occupancy number that was stored in the cache. */
public uint reset_occupancy(
    RWStructuredBuffer<CacheEntry> entries, 
    const ConstantBuffer<CacheInfo> info, 
    const uint2 key
) {
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(key);
    /* Evaluate the voxel hash function */
    uint hash = voxel_hash(v.voxel_index, v.object_index, v.face_index) % info.capacity;

    for (uint i = 0u; i < 8u; ++i) {
        const uint2 entry_key = entries[hash].key;

        /* Check if the key in this entry matches */
        if (all(entry_key == key)) {
            const uint occupancy = entries[hash].occupancy;
            entries[hash].occupancy = 0u; /* <- Reset */
            return occupancy;
        }

        /* Move to the next hash cache slot */
        hash += 1u; if (hash >= info.capacity) hash = 0u;
    }

    return 0u; /* Unreachable */
}

} // cache
