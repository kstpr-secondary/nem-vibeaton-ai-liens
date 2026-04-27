#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "sokol_gfx.h"
#include "texture.h"
#include "renderer.h"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Module-level texture store — index 0 reserved (invalid handle id == 0)
// ---------------------------------------------------------------------------

namespace {

static sg_image  s_images[256] = {};
static uint32_t  s_next_id     = 1;

} // namespace

// ---------------------------------------------------------------------------
// Internal accessors
// ---------------------------------------------------------------------------

sg_image texture_get(uint32_t id) {
    if (id == 0 || id >= s_next_id) return {};
    return s_images[id];
}

RendererTextureHandle texture_store_insert(sg_image img) {
    if (s_next_id >= 256) {
        printf("[renderer] ERROR: texture store full (max 255 textures)\n");
        return {};
    }
    uint32_t id    = s_next_id++;
    s_images[id]   = img;
    return { id };
}

void texture_store_shutdown() {
    for (uint32_t i = 1; i < s_next_id; ++i) {
        sg_destroy_image(s_images[i]);
        s_images[i] = {};
    }
    s_next_id = 1;
}

// ---------------------------------------------------------------------------
// Public API — texture upload
// ---------------------------------------------------------------------------

RendererTextureHandle renderer_upload_texture_2d(
    const void* pixels, int width, int height, int channels)
{
    if (!pixels || width <= 0 || height <= 0 || channels <= 0) return {};

    sg_image_desc desc       = {};
    desc.width               = width;
    desc.height              = height;
    desc.pixel_format        = SG_PIXELFORMAT_RGBA8;
    desc.data.mip_levels[0]  = { pixels, (size_t)(width * height * channels) };
    desc.label               = "texture-2d";

    sg_image img = sg_make_image(&desc);
    if (sg_query_image_state(img) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: 2D texture upload failed\n");
        return {};
    }
    return texture_store_insert(img);
}

RendererTextureHandle renderer_upload_texture_from_file(const char* path) {
    if (!path) return {};

    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) {
        printf("[renderer] ERROR: stbi_load failed for '%s'\n", path);
        return {};
    }

    RendererTextureHandle handle = renderer_upload_texture_2d(data, w, h, 4);
    stbi_image_free(data);

    if (!renderer_handle_valid(handle)) {
        printf("[renderer] ERROR: texture upload failed for '%s' (%dx%d)\n", path, w, h);
        return {};
    }

    printf("[renderer] texture loaded: '%s' (%dx%d, 4ch), handle id=%u\n",
           path, w, h, (unsigned)handle.id);
    return handle;
}

RendererTextureHandle renderer_upload_cubemap(
    const void* faces[6], int face_width, int face_height, int channels)
{
    if (!faces || face_width <= 0 || face_height <= 0 || channels <= 0) return {};

    for (int f = 0; f < 6; ++f) {
        if (!faces[f]) {
            printf("[renderer] ERROR: cubemap face %d is null\n", f);
            return {};
        }
    }

    // Cubemap mip_levels[0] must contain all 6 faces packed contiguously
    // in order: +X, -X, +Y, -Y, +Z, -Z (sokol requirement).
    size_t face_bytes = (size_t)(face_width * face_height * channels);
    std::vector<uint8_t> packed(6 * face_bytes);
    for (int f = 0; f < 6; ++f) {
        std::memcpy(packed.data() + f * face_bytes, faces[f], face_bytes);
    }

    sg_image_desc desc      = {};
    desc.type               = SG_IMAGETYPE_CUBE;
    desc.width              = face_width;
    desc.height             = face_height;
    desc.pixel_format       = SG_PIXELFORMAT_RGBA8;
    desc.data.mip_levels[0] = { packed.data(), packed.size() };
    desc.label              = "cubemap";

    printf("[renderer] cubemap: uploading %d faces, %dx%d, %d ch, total %zu bytes\n",
           6, face_width, face_height, channels, packed.size());

    sg_image img = sg_make_image(&desc);
    if (sg_query_image_state(img) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: cubemap upload failed — image state=%d, id=%u\n",
               (int)sg_query_image_state(img), (unsigned)img.id);
        return {};
    }
    printf("[renderer] cubemap: uploaded successfully, handle id=%u\n", (unsigned)img.id);
    return texture_store_insert(img);
}

