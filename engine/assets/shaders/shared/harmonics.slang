module harmonics;

/* 2-band spherical harmonics. */
public struct Sh4 {
    float3 ca;
    float3 cx;
    float3 cy;
    float3 cz;
};

/* Accumulate radiance into 2-band spherical harmonics. */
public [ForceInline] void accumulate_sh4(inout Sh4 sh, const float3 rad, const float3 dir) {
    /* L0 band (ambient) */
    sh.ca += rad;
    /* L1 band */
    sh.cx += rad * dir.x;
    sh.cy += rad * dir.y;
    sh.cz += rad * dir.z;
}

/* Sample radiance from 2-band spherical harmonics. */
public [ForceInline] float3 sample_sh4(const Sh4 sh, const float3 dir) {
    return (
        /* L0 band (ambient) */
        sh.ca +
        /* L1 band */
        (sh.cx * dir.x +
        sh.cy * dir.y + 
        sh.cz * dir.z) * 3.0f
    );
}
