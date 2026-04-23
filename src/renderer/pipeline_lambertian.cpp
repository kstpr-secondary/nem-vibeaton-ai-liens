#include "sokol_gfx.h"
#include "pipeline_lambertian.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include "shaders/lambertian.glsl.h"

#include <cstdio>
#include <cstddef>

namespace {
    static sg_pipeline s_lambertian_pip = {0};
}

sg_pipeline create_pipeline_lambertian(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(lambertian_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: lambertian shader creation failed — using magenta fallback\n");
        s_lambertian_pip = magenta_fallback;
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Full Vertex layout — must match struct Vertex in renderer.h exactly.
    desc.layout.buffers[0].stride = sizeof(Vertex);

    // Lambertian shader consumes position and normal.
    desc.layout.attrs[ATTR_lambertian_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_lambertian_position].offset = offsetof(Vertex, position);
    desc.layout.attrs[ATTR_lambertian_normal].format   = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_lambertian_normal].offset   = offsetof(Vertex, normal);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    desc.cull_mode           = SG_CULLMODE_BACK;
    desc.label               = "lambertian-pipeline";

    s_lambertian_pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(s_lambertian_pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: lambertian pipeline creation failed — using magenta fallback\n");
        s_lambertian_pip = magenta_fallback;
        return magenta_fallback;
    }

    return s_lambertian_pip;
}

sg_pipeline get_lambertian_pipeline() {
    return s_lambertian_pip;
}
