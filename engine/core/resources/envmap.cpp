#include "envmap.hpp"

#include <stb_image.h>
#include <glm/gtc/packing.hpp>

#include <graphite/vram_bank.hh>

#include "engine/engine.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/shared/colorspace.hpp"

namespace tmt {

bool Envmap::load() {
    int tex_width = -1;
    int tex_height = -1;
    int channels = -1;
    float* data = nullptr;

    /* Parse HDR data */
    data = stbi_loadf(file_location.get_absolute_path().string().c_str(), &tex_width, &tex_height, &channels, 3);
    if (!data) {
        return false;
    }

    width = (uint32_t)tex_width;
    height = (uint32_t)tex_height;
    name = file_location.get_relative_path().string();
    auto& bank = engine.renderer.vram_bank();

    /* Init the texture resource */
    const std::string texture_name = name + " Envmap Texture";
    texture = bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA16Sfloat, { width, height, 0u })
                  .expect("failed to initialise envmap texture.");

    // uint32_t* packed_data = new uint32_t[width * height] {};
    uint64_t* packed_data = new uint64_t[width * height] {};
    for (uint32_t i = 0u; i < width * height; ++i) {
        const glm::vec3 unpacked = cs::r709_to_acescg(glm::vec3(data[i * 3u], data[i * 3u + 1u], data[i * 3u + 2u]));
        // packed_data[i] = glm::packF2x11_1x10(unpacked);
        packed_data[i] = glm::packHalf4x16(glm::vec4(unpacked, 0.0f));
    }
    free(data);

    /* Upload envmap texture data */
    bank.upload_texture(texture, packed_data, width * height * sizeof(uint64_t)).expect("failed to upload envmap texture.");
    delete[] packed_data;

    /* Init the image resource */
    const std::string image_name = name + " Envmap Image";
    image = bank.create_image(image_name.c_str(), texture).expect("failed to initialize envmap image.");

    return true;
}

void Envmap::unload() {
    auto& bank = engine.renderer.vram_bank();

    bank.destroy(texture);
    bank.destroy(image);
}

}  // namespace tmt
