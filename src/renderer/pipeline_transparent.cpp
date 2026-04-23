#include "sokol_gfx.h"
#include "pipeline_transparent.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include "shaders/unlit.glsl.h"

#include <cstdio>
#include <cstddef>

sg_pipeline create_pipeline_transparent(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(unlit_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: transparent shader creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    desc.layout.buffers[0].stride = sizeof(Vertex);
    desc.layout.attrs[ATTR_unlit_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_unlit_position].offset = offsetof(Vertex, position);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;
    desc.cull_mode           = SG_CULLMODE_BACK;
    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.label = "transparent-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: transparent pipeline creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    return pip;
}
