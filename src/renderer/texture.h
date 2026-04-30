#ifndef VIBEATON_TEXTURE_H
#define VIBEATON_TEXTURE_H

#include "renderer.h"

// Forward declarations — skipped if sokol_gfx.h was already included.
#ifndef SOKOL_GFX_INCLUDED
struct sg_image { uint32_t id; };
struct sg_view  { uint32_t id; };
struct sg_sampler { uint32_t id; };
#endif

// ---------------------------------------------------------------------------
// Internal texture-store accessors — used by renderer.cpp and skybox.cpp.
// Not part of the public renderer API.
// ---------------------------------------------------------------------------

sg_view               texture_get_view(uint32_t id);
sg_image              texture_get(uint32_t id);
sg_sampler            texture_get_sampler(uint32_t id);
RendererTextureHandle texture_store_insert(sg_image img);
void                  texture_store_shutdown();

// Thin wrapper around renderer_upload_texture_2d — same signature, no file loading.
// Used by engine-side glTF texture extraction (Approach B).
RendererTextureHandle renderer_upload_texture_from_memory(
    const void* pixels, int width, int height, int channels);

#endif // VIBEATON_TEXTURE_H
