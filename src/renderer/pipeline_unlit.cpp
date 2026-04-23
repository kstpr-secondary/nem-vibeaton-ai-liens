#include "sokol_gfx.h"
#include "pipeline_unlit.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include "shaders/unlit.glsl.h"

#include <cstdio>
#include <cstddef>

sg_pipeline create_pipeline_unlit(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(unlit_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: unlit shader creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Full Vertex layout — must match struct Vertex in renderer.h exactly.
    desc.layout.buffers[0].stride = sizeof(Vertex);

    desc.layout.attrs[ATTR_unlit_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_unlit_position].offset  = offsetof(Vertex, position);

    desc.layout.attrs[ATTR_unlit_normal].format    = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_unlit_normal].offset    = offsetof(Vertex, normal);

    desc.layout.attrs[ATTR_unlit_uv].format        = SG_VERTEXFORMAT_FLOAT2;
    desc.layout.attrs[ATTR_unlit_uv].offset        = offsetof(Vertex, uv);

    desc.layout.attrs[ATTR_unlit_tangent].format   = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_unlit_tangent].offset   = offsetof(Vertex, tangent);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    desc.cull_mode           = SG_CULLMODE_BACK;
    desc.label               = "unlit-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: unlit pipeline creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    return pip;
}
