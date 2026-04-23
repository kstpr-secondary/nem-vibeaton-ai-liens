#include "sokol_gfx.h"
#include "pipeline_line_quad.h"

#include <glm/glm.hpp>
#include "shaders/line_quad.glsl.h"

#include <cstdio>
#include <cstddef>

// Matches LineQuadVertex in renderer.cpp
struct LineQuadVertex {
    float position[3];
    float color[4];
};

sg_pipeline create_pipeline_line_quad(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(line_quad_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad shader creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    desc.layout.buffers[0].stride = sizeof(LineQuadVertex);
    desc.layout.attrs[ATTR_line_quad_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_line_quad_position].offset = offsetof(LineQuadVertex, position);
    desc.layout.attrs[ATTR_line_quad_color].format    = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[ATTR_line_quad_color].offset    = offsetof(LineQuadVertex, color);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;
    desc.cull_mode           = SG_CULLMODE_NONE;
    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.label = "line-quad-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad pipeline creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    return pip;
}
