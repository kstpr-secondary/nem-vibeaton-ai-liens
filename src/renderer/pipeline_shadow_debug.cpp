#include "sokol_gfx.h"
#include "pipeline_shadow_debug.h"
#include "renderer.h"

#include "shaders/shadow_debug.glsl.h"

#include <cstdio>

namespace {
    static sg_pipeline s_shadow_debug_pip = {};
}

sg_pipeline create_pipeline_shadow_debug(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(shadow_debug_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug shader creation failed — using magenta fallback\n");
        s_shadow_debug_pip = magenta_fallback;
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Debug quad vertex layout: 4 floats per vertex (x, y, u, v) — 16 bytes stride.
    desc.layout.buffers[0].stride = 4 * sizeof(float);
    desc.layout.attrs[ATTR_shadow_debug_position].format = SG_VERTEXFORMAT_FLOAT2;
    desc.layout.attrs[ATTR_shadow_debug_position].offset = 0;          // x, y at offset 0
    desc.layout.attrs[ATTR_shadow_debug_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    desc.layout.attrs[ATTR_shadow_debug_texcoord0].offset = 2 * sizeof(float);  // u, v at offset 8

    // Overlay: depth test disabled (always pass), no depth write.
    desc.depth.compare       = SG_COMPAREFUNC_ALWAYS;
    desc.depth.write_enabled = false;
    desc.cull_mode           = SG_CULLMODE_NONE;

    // No blending — raw greyscale texture visualization (alpha = 1.0 always).
    desc.colors[0].blend.enabled = false;

    desc.label = "shadow-debug-pipeline";

    s_shadow_debug_pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(s_shadow_debug_pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug pipeline creation failed — using magenta fallback\n");
        s_shadow_debug_pip = magenta_fallback;
        return magenta_fallback;
    }

    return s_shadow_debug_pip;
}
