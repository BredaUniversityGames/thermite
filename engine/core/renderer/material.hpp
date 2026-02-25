#pragma once

#include <graphite/resources/handle.hh>

namespace tmt {

/* Voxel material data, used in a palette. */
struct Material {
    float albedo_r = 0.0f;
    float albedo_g = 0.0f;
    float albedo_b = 0.0f;
    /* Padding */
    uint32_t : 32;
};

/* Material palette index. */
using MaterialIndex = uint8_t;

constexpr MaterialIndex AIR_INDEX = 0xFFu;

/* Voxel material palette with 256 entries. */
struct MaterialPalette {
    static constexpr size_t ENTRY_COUNT = (1u << (sizeof(MaterialIndex) * 8u));
    Material entries[ENTRY_COUNT] {};

    MaterialIndex material_to_index(const Material* material) const { return static_cast<MaterialIndex>(material - entries); }
};

/* Generate/precompute directional albedo look-up texture. */
void generate_e_lut(Texture& out_texture, const uint32_t resolution = 32u);

}  // namespace tmt
