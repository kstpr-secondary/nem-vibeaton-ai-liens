#include "sokol_gfx.h"
#include "pipeline_shadow.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include "shaders/shadow_depth.glsl.h"

#include <cstdio>
#include <cstddef>

namespace {
    static sg_pipeline s_shadow_pip = {0};
}

sg_pipeline create_pipeline_shadow(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(shadow_depth_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow depth shader creation failed — using magenta fallback\n");
        s_shadow_pip = magenta_fallback;
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Vertex layout stride — must match struct Vertex in renderer.h exactly.
    // sizeof(Vertex) = 44 bytes (11 floats: position[3] + normal[3] + uv[2] + tangent[3]).
    desc.layout.buffers[0].stride = sizeof(Vertex);

    // Shadow depth pass only needs position; normal, uv, and tangent are unused.
    desc.layout.attrs[ATTR_shadow_depth_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_shadow_depth_position].offset = offsetof(Vertex, position);

    // Use BACK culling for shadow pass to fix Peter Panning. We'll handle acne with depth bias in the shader.
    desc.cull_mode           = SG_CULLMODE_BACK;
    desc.depth.pixel_format  = SG_PIXELFORMAT_DEPTH;
    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    // R32F color attachment — captures depth as RGBA for debug visualization.
    // Required because OpenGL 3.3 doesn't allow non-comparison sampling of depth-stencil textures.
    desc.color_count         = 1;
    desc.colors[0].pixel_format = SG_PIXELFORMAT_R32F;
    desc.label                  = "shadow-depth-pipeline";

    s_shadow_pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(s_shadow_pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow depth pipeline creation failed — using magenta fallback\n");
        s_shadow_pip = magenta_fallback;
        return magenta_fallback;
    }

    return s_shadow_pip;
}

sg_pipeline get_shadow_pipeline() {
    return s_shadow_pip;
}
