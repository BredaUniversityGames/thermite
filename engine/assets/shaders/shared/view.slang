module view;

/* Represents a renderable view in the scene. */
public struct RenderView {
    /* World-space to view-space transformation matrix. */
    public float4x4 world_to_view;
    /* .xyz = Camera origin ~ .w = Focal length */
    float4 origin_focal_length;
    /* .xy = Camera resolution ~ .z = Sensor size ~ .w = Aspect ratio (height / width) */
    float4 resolution_sensor_size_aspect_ratio;

    public property float3 origin { get { return origin_focal_length.xyz; } }
    public property float focal_length { get { return origin_focal_length.w; } }
    public property uint2 resolution { get { return asuint(resolution_sensor_size_aspect_ratio.xy); } }
    public property float sensor_size { get { return resolution_sensor_size_aspect_ratio.z; } }
    public property float aspect_ratio { get { return resolution_sensor_size_aspect_ratio.w; } }
};

/* Represents a ray that can be traced. */
public struct Ray {
    public float3 origin;
    public float3 dir;
    public float3 rcp_dir;
};

/* Convert a pixel coordinate to a normalized device coordinate. */
inline float2 pixel_to_ndc(int2 pixel, uint2 resolution) {
    const float2 ndc = ((float2(pixel) + 0.5) / resolution) * 2.0 - 1.0;
    return ndc * float2(1.0, -1.0);
}

/* Generate a ray for a pin hole camera. */
public inline Ray pinhole_ray(RenderView view, int2 pixel) {
    /* Calculate the local direction for the given NDC */
    const float2 sensor_size = float2(view.sensor_size, view.sensor_size * view.aspect_ratio);
    const float2 sensor_pos = pixel_to_ndc(pixel, view.resolution) * sensor_size;
    const float3 local_dir = normalize(float3(sensor_pos, view.focal_length));

    Ray ray; /* Fill in the ray struct */
    ray.origin = view.origin;
    ray.dir = normalize(mul(view.world_to_view, float4(local_dir, 0.0)).xyz);
    ray.rcp_dir = rcp(ray.dir);
    return ray;
}
