module material;

/* Voxel material data, used in a palette. */
public struct Material {
    /* `Base Color` Overall color of the material used for diffuse, metal and transmission. */
    public float3 albedo;
    /* `Roughness` Specifies microfacet roughness of the surface for specular reflection and transmission. */
    public float roughness;
    /* `Emission Color` Color of light emitted from the surface. */
    public float3 emission_color;
    /* `Emission` Strength of the emitted light. */
    public float emission;
    /* `Specular Tint` Color tint for specular and metallic reflection. */
    public float3 specular_tint;
    /* `Specular` Adjustment to the IOR to increase or decrease intensity of the specular layer. */
    /* 0.5 means no adjustment, 0 removes all reflections, 1 doubles them at normal incidence. */
    public float specular;
    /* `Metallic` Blends between a dielectric and metallic material model. */
    public float metallic;
    /* `IOR` Index of refraction (IOR) for specular reflection and transmission. */
    public float ior;
    /* `Transmission` Transmission is used to render materials like glass and liquids, */
    /* where the surface both reflects light and transmits it into the interior of the object. */
    public float transmission;
};

/*******************************************************************/
/*                                                                 */
/*  ~ thermite material model                        emission      */
/*                                                      ^          */
/*    ambient medium                                    |          */
/*  +---------------------+---------------------+-------+-------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |    gloss      |  */
/*  |░░░░░░░ metal ░░░░░░░|     translucent     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|        base         +---------------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░diffuse░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  + - - - - - - - - - - +- - - - - - - - - - -+ - - - - - - - +  */
/*                                              <- opaque-base ->  */
/*                        <---------- dielectric-base ---------->  */
/*  <--------------------------- base -------------------------->  */
/*                                                                 */
/*******************************************************************/

/* Evaluate opaque material slabs. (metal, diffuse, & gloss) */
public void opaque_slabs() {

}
