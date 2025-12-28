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
};

namespace cache {

/* Increment occupancy in the cache. */
/* Returns true if this was the first occurrence of the given key. */
public bool inc_occupancy(
    RWStructuredBuffer<CacheEntry> entries, 
    const ConstantBuffer<CacheInfo> info, 
    const uint2 key
) {
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(key);
    /* Evaluate the voxel hash function */
    uint hash = voxel_hash(v.voxel_index, v.object_index, v.face_index) % info.capacity;

    for (uint i = 0u; i < 32u; ++i) {
        const uint2 entry_key = entries[hash].key;
        const bool is_free = all(entry_key == 0u);

        /* Claim this cache entry if it is free */
        if (is_free) entries[hash].key = key;

        /* Update the cache entry */
        if (is_free || all(entry_key == key)) {
            /* Atomic increment the occupancy counter */
            uint occupancy = 0u;
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

    for (uint i = 0u; i < 32u; ++i) {
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

    for (uint i = 0u; i < 32u; ++i) {
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
