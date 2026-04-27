#include "sokol_gfx.h"
#include "pipeline_blinnphong.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include "shaders/blinnphong.glsl.h"

#include <cstdio>
#include <cstddef>

namespace {
    static sg_pipeline s_blinnphong_pip = {0};
}

sg_pipeline create_pipeline_blinnphong(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(blinnphong_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: blinnphong shader creation failed — using magenta fallback\n");
        s_blinnphong_pip = magenta_fallback;
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Full Vertex layout — must match struct Vertex in renderer.h exactly.
    desc.layout.buffers[0].stride = sizeof(Vertex);

    // Blinn-Phong shader consumes position, normal, and texcoord.
    desc.layout.attrs[ATTR_blinnphong_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_blinnphong_position].offset = offsetof(Vertex, position);
    desc.layout.attrs[ATTR_blinnphong_normal].format   = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_blinnphong_normal].offset   = offsetof(Vertex, normal);
    desc.layout.attrs[ATTR_blinnphong_texcoord0].format = SG_VERTEXFORMAT_FLOAT2;
    desc.layout.attrs[ATTR_blinnphong_texcoord0].offset = offsetof(Vertex, uv);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    desc.cull_mode           = SG_CULLMODE_BACK;
    desc.label               = "blinnphong-pipeline";

    s_blinnphong_pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(s_blinnphong_pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: blinnphong pipeline creation failed — using magenta fallback\n");
        s_blinnphong_pip = magenta_fallback;
        return magenta_fallback;
    }

    return s_blinnphong_pip;
}

sg_pipeline get_blinnphong_pipeline() {
    return s_blinnphong_pip;
}
