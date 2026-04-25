#ifndef VIBEATON_TEXTURE_H
#define VIBEATON_TEXTURE_H

#include "renderer.h"

// Forward declarations — skipped if sokol_gfx.h was already included.
#ifndef SOKOL_GFX_INCLUDED
struct sg_image { uint32_t id; };
#endif

// ---------------------------------------------------------------------------
// Internal texture-store accessors — used by renderer.cpp and skybox.cpp.
// Not part of the public renderer API.
// ---------------------------------------------------------------------------

sg_image              texture_get(uint32_t id);
RendererTextureHandle texture_store_insert(sg_image img);
void                  texture_store_shutdown();

#endif // VIBEATON_TEXTURE_H
