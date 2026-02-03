#include "texture_2d.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <graphite/vram_bank.hh>

#include "engine.hpp"

#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

bool Texture2D::load() {
    int tex_width = -1;
    int tex_height = -1;
    int channels = -1;
    unsigned char* data = nullptr;

    data = stbi_load(file_location.get_absolute_path().string().c_str(), &tex_width, &tex_height, &channels, 4);
    if (!data) {
        Log::error(Log::Scope::RENDERER, "failed to load texture at {}.", file_location);
        return false;
    }

    auto& bank = engine.renderer.vram_bank();

    /* Initialise the texture */
    if (const Result r = bank.create_texture("[Particles] Bojan", TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA8Unorm, {(u32)tex_width, (u32)tex_height, 0});
        r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise bojan texture.\nreason: {}", r.unwrap_err().c_str());
        return false;
    } else
        texture = r.unwrap();

    if (const Result r = bank.upload_texture(texture, data, tex_width * tex_height * 4); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to upload bojan texture.\nreason: {}", r.unwrap_err().c_str());
        return false;
    }
    free(data);

    if (const Result r = bank.create_image("[Particles] Bojan Image", texture); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize bojan image.\nreason: {}", r.unwrap_err().c_str());
        return false;
    } else
        image = r.unwrap();

    return true;
}

void Texture2D::unload() {}

}  // namespace tmt
