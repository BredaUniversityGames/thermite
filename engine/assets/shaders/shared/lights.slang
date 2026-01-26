module lights;

public struct Light {
    float4 origin_source_radius;
    float4 luminance_attenuation_radius;

    public property float3 origin { get { return origin_source_radius.xyz; } }
    public property float source_radius { get { return origin_source_radius.w; } }
    public property float3 luminance { get { return luminance_attenuation_radius.xyz; } }
    public property float attenuation_radius { get { return luminance_attenuation_radius.w; } }
};
