#include "sokol_gfx.h"
#include "pipeline_skybox.h"

#include <glm/glm.hpp>
#include "shaders/skybox.glsl.h"

#include <cstdio>

sg_pipeline create_pipeline_skybox(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(skybox_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: skybox shader creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    // Skybox uses a 1-unit cube with position-only vertices.
    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    desc.layout.buffers[0].stride = 3 * sizeof(float);
    desc.layout.attrs[ATTR_skybox_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_skybox_position].offset = 0;

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;
    desc.cull_mode           = SG_CULLMODE_FRONT; // render inside of cube
    desc.label = "skybox-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: skybox pipeline creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    return pip;
}
