#include <GL/gl.h>
#include "sokol_gfx.h"
#include "skybox.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shaders/skybox.glsl.h"

#include <cstdio>
#include <cstddef>

// ---------------------------------------------------------------------------
// Unit cube — 24 vertices (4 per face), positions only.
// The vertex position IS the cubemap lookup direction.
// Winding is CCW as seen from outside; pipeline uses CULLMODE_FRONT so the
// inside faces (visible to the camera inside the box) are rendered.
// ---------------------------------------------------------------------------

namespace {

// clang-format off
static const float s_cube_verts[24 * 3] = {
    // +Z face
    -1,-1, 1,   1,-1, 1,   1, 1, 1,  -1, 1, 1,
    // -Z face
     1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1,
    // +X face
     1,-1, 1,   1,-1,-1,   1, 1,-1,   1, 1, 1,
    // -X face
    -1,-1,-1,  -1,-1, 1,  -1, 1, 1,  -1, 1,-1,
    // +Y face
    -1, 1, 1,   1, 1, 1,   1, 1,-1,  -1, 1,-1,
    // -Y face
    -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,
};

static const uint16_t s_cube_indices[36] = {
     0, 1, 2,  0, 2, 3,   // +Z
     4, 5, 6,  4, 6, 7,   // -Z
     8, 9,10,  8,10,11,   // +X
    12,13,14, 12,14,15,   // -X
    16,17,18, 16,18,19,   // +Y
    20,21,22, 20,22,23,   // -Y
};
// clang-format on

static sg_buffer  s_vbuf    = {};
static sg_buffer  s_ibuf    = {};
static sg_sampler s_sampler = {};

} // namespace

// ---------------------------------------------------------------------------
// Pipeline creation
// ---------------------------------------------------------------------------

sg_pipeline skybox_create_pipeline(sg_pipeline magenta_fallback) {
    sg_shader shd = sg_make_shader(skybox_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: skybox shader creation failed — using magenta fallback\n");
        return magenta_fallback;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT16;

    // Position only — 3 floats per vertex, tightly packed.
    desc.layout.buffers[0].stride              = 3 * sizeof(float);
    desc.layout.attrs[ATTR_skybox_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_skybox_position].offset = 0;

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;  // xyww trick puts skybox at z=1.0 (cleared depth) — no need to write

    // Sokol's default face_winding is SG_FACEWINDING_CW (see sg_query_pipeline_defaults).
    // The cube is wound CCW from outside, so the inner faces present CW screen-space winding —
    // sokol classifies those as FRONT. Disabling culling avoids the winding-vs-default trap and
    // matches the sokol cubemap sample (sample-skybox-code/cubemap-jpeg-sapp.c).
    desc.cull_mode = SG_CULLMODE_NONE;
    desc.label     = "skybox-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: skybox pipeline creation failed — using magenta fallback\n");
        return magenta_fallback;
    }
    return pip;
}

// ---------------------------------------------------------------------------
// Resource management
// ---------------------------------------------------------------------------

void skybox_init_resources() {
    sg_buffer_desc vdesc  = {};
    vdesc.usage.vertex_buffer = true;
    vdesc.usage.immutable     = true;
    vdesc.data                = SG_RANGE(s_cube_verts);
    vdesc.label               = "skybox-vbuf";
    s_vbuf = sg_make_buffer(&vdesc);

    sg_buffer_desc idesc          = {};
    idesc.usage.index_buffer      = true;
    idesc.usage.immutable         = true;
    idesc.data                    = SG_RANGE(s_cube_indices);
    idesc.label                   = "skybox-ibuf";
    s_ibuf = sg_make_buffer(&idesc);

    sg_sampler_desc sdesc  = {};
    sdesc.min_filter       = SG_FILTER_LINEAR;
    sdesc.mag_filter       = SG_FILTER_LINEAR;
    sdesc.wrap_u           = SG_WRAP_CLAMP_TO_EDGE;
    sdesc.wrap_v           = SG_WRAP_CLAMP_TO_EDGE;
    sdesc.wrap_w           = SG_WRAP_CLAMP_TO_EDGE;
    sdesc.label            = "skybox-sampler";
    s_sampler = sg_make_sampler(&sdesc);
}

void skybox_shutdown() {
    sg_destroy_buffer(s_vbuf);
    sg_destroy_buffer(s_ibuf);
    sg_destroy_sampler(s_sampler);
    s_vbuf    = {};
    s_ibuf    = {};
    s_sampler = {};
}

// ---------------------------------------------------------------------------
// Per-frame draw pass
// ---------------------------------------------------------------------------

void draw_skybox_pass(
    sg_pipeline pip,
    sg_image    cubemap_img,
    const float proj[16],
    const float view[16])
{
    if (pip.id == 0 || cubemap_img.id == 0) {
        printf("[renderer] skybox: skipped — pip=%u img=%u\n", (unsigned)pip.id, (unsigned)cubemap_img.id);
        return;
    }
    if (sg_query_image_state(cubemap_img) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] skybox: image not valid (state=%d)\n", (int)sg_query_image_state(cubemap_img));
        return;
    }

    // Strip translation from view: rotation-only matrix keeps skybox at infinity.
    glm::mat4 view_mat      = glm::make_mat4(view);
    glm::mat4 proj_mat      = glm::make_mat4(proj);
    glm::mat4 vp_no_trans   = proj_mat * glm::mat4(glm::mat3(view_mat));

    sg_apply_pipeline(pip);

    // Create a view wrapping the cubemap image for shader binding.
    // A view is cheap to create and is destroyed after the draw.
    sg_view_desc vdesc   = {};
    vdesc.texture.image  = cubemap_img;
    sg_view cubemap_view = sg_make_view(&vdesc);

    sg_bindings bind             = {};
    bind.vertex_buffers[0]       = s_vbuf;
    bind.index_buffer            = s_ibuf;
    bind.views[VIEW_skybox_tex]  = cubemap_view;
    bind.samplers[SMP_smp]       = s_sampler;
    sg_apply_bindings(&bind);

    skybox_vs_params_t vs_p       = { vp_no_trans };
    sg_range    vs_p_range = SG_RANGE(vs_p);
    sg_apply_uniforms(UB_skybox_vs_params, &vs_p_range);

    sg_draw(0, 36, 1);

    GLuint glerr = glGetError();
    if (glerr != GL_NO_ERROR) {
        printf("[renderer] skybox: GL error after sg_draw: 0x%x\n", (unsigned)glerr);
    }

    sg_destroy_view(cubemap_view);
}

