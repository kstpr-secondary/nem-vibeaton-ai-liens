#include "shadow_pass.h"

#include "renderer.h"

#include "sokol_gfx.h"
#include "shaders/shadow_debug.glsl.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>

namespace {

static ShadowPassState g_state = {};
static bool g_initialized = false;

// Destroy any resources created so far on failure (cleanup for mid-init errors).
// Same ownership order as shadow_pass_shutdown(): views first (depend on image),
// then samplers, then images.
static void cleanup_shadow_resources() {
    if (g_state.shadow_ds_view.id != 0)              { sg_destroy_view(g_state.shadow_ds_view); g_state.shadow_ds_view = {}; }
    if (g_state.shadow_tex_view.id != 0)             { sg_destroy_view(g_state.shadow_tex_view); g_state.shadow_tex_view = {}; }
    if (g_state.shadow_debug_color_view.id != 0)     { sg_destroy_view(g_state.shadow_debug_color_view); g_state.shadow_debug_color_view = {}; }
    if (g_state.shadow_debug_view.id != 0)           { sg_destroy_view(g_state.shadow_debug_view); g_state.shadow_debug_view = {}; }
    if (g_state.shadow_sampler.id != 0)              { sg_destroy_sampler(g_state.shadow_sampler); g_state.shadow_sampler = {}; }
    if (g_state.shadow_debug_sampler.id != 0)        { sg_destroy_sampler(g_state.shadow_debug_sampler); g_state.shadow_debug_sampler = {}; }
    if (g_state.shadow_map_img.id != 0)              { sg_destroy_image(g_state.shadow_map_img); g_state.shadow_map_img = {}; }
    if (g_state.shadow_debug_img.id != 0)            { sg_destroy_image(g_state.shadow_debug_img); g_state.shadow_debug_img = {}; }
    memset(&g_state, 0, sizeof(g_state));
}

} // anonymous namespace

bool shadow_pass_init() {
    if (g_initialized) {
        return g_state.shadow_map_img.id != 0;
    }

    int size = k_shadow_map_size;

    // --- Create depth-stencil image ------------------------------------------
    sg_image_desc img_desc = {};
    img_desc.width           = size;
    img_desc.height          = size;
    img_desc.pixel_format    = SG_PIXELFORMAT_DEPTH;
    img_desc.usage.depth_stencil_attachment = true;
    img_desc.sample_count    = 1;
    img_desc.label           = "shadow-map-img";

    g_state.shadow_map_img = sg_make_image(&img_desc);
    if (sg_query_image_state(g_state.shadow_map_img) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow map image creation failed\n");
        cleanup_shadow_resources();
        return false;
    }
    printf("[renderer] shadow pass init completed — shadow map image handle %u\n",
           g_state.shadow_map_img.id);

    // --- Create depth-stencil attachment view (write target) ------------------
    sg_view_desc ds_vd = {};
    ds_vd.depth_stencil_attachment.image = g_state.shadow_map_img;
    ds_vd.label = "shadow-ds-view";

    g_state.shadow_ds_view = sg_make_view(&ds_vd);
    if (sg_query_view_state(g_state.shadow_ds_view) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow depth-stencil view creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // --- Create texture view (read target for shader sampling) ----------------
    sg_view_desc tex_vd = {};
    tex_vd.texture.image  = g_state.shadow_map_img;
    tex_vd.label          = "shadow-tex-view";

    g_state.shadow_tex_view = sg_make_view(&tex_vd);
    if (sg_query_view_state(g_state.shadow_tex_view) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow texture view creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // --- Create comparison sampler -------------------------------------------
    sg_sampler_desc smp_desc = {};
    smp_desc.compare       = SG_COMPAREFUNC_LESS;
    smp_desc.min_filter    = SG_FILTER_LINEAR;
    smp_desc.mag_filter    = SG_FILTER_LINEAR;
    smp_desc.wrap_u        = SG_WRAP_CLAMP_TO_EDGE;
    smp_desc.wrap_v        = SG_WRAP_CLAMP_TO_EDGE;
    smp_desc.label         = "shadow-sampler";

    g_state.shadow_sampler = sg_make_sampler(&smp_desc);
    if (sg_query_sampler_state(g_state.shadow_sampler) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow sampler creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // --- Create debug sampler (raw, no comparison — for texture visualization) --
    sg_sampler_desc debug_smp_desc = {};
    debug_smp_desc.min_filter     = SG_FILTER_LINEAR;
    debug_smp_desc.mag_filter     = SG_FILTER_LINEAR;
    debug_smp_desc.wrap_u         = SG_WRAP_CLAMP_TO_EDGE;
    debug_smp_desc.wrap_v         = SG_WRAP_CLAMP_TO_EDGE;
    debug_smp_desc.compare        = SG_COMPAREFUNC_NEVER;  // disables comparison sampling
    debug_smp_desc.label          = "shadow-debug-sampler";

    g_state.shadow_debug_sampler = sg_make_sampler(&debug_smp_desc);
    if (sg_query_sampler_state(g_state.shadow_debug_sampler) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug sampler creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // --- Create R32F color image for debug visualization ---------------------
    // OpenGL 3.3 does not allow sampling a depth texture with a non-comparison sampler.
    // We render depth as RGBA into an R32F color attachment, then sample that with a
    // regular (non-compare) sampler for the debug overlay.
    sg_image_desc dbg_img_desc = {};
    dbg_img_desc.width           = size;
    dbg_img_desc.height          = size;
    dbg_img_desc.pixel_format    = SG_PIXELFORMAT_R32F;
    dbg_img_desc.sample_count    = 1;
    dbg_img_desc.usage.color_attachment = true;
    dbg_img_desc.label           = "shadow-debug-color-img";

    g_state.shadow_debug_img = sg_make_image(&dbg_img_desc);
    if (sg_query_image_state(g_state.shadow_debug_img) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug color image creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // Color-attachment view for the R32F image (render target).
    sg_view_desc dbg_color_vd = {};
    dbg_color_vd.color_attachment.image        = g_state.shadow_debug_img;
    dbg_color_vd.label                         = "shadow-debug-color-att-view";

    g_state.shadow_debug_color_view = sg_make_view(&dbg_color_vd);
    if (sg_query_view_state(g_state.shadow_debug_color_view) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug color attachment view creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // Texture view for the R32F image (readable by regular sampler in debug shader).
    sg_view_desc dbg_tex_vd = {};
    dbg_tex_vd.texture.image  = g_state.shadow_debug_img;
    dbg_tex_vd.label          = "shadow-debug-tex-view";

    g_state.shadow_debug_view = sg_make_view(&dbg_tex_vd);
    if (sg_query_view_state(g_state.shadow_debug_view) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug texture view creation failed\n");
        cleanup_shadow_resources();
        return false;
    }

    // --- Build attachments struct for sg_begin_pass --------------------------
    g_state.shadow_attachments.depth_stencil = g_state.shadow_ds_view;
    g_state.shadow_attachments.colors[0]     = g_state.shadow_debug_color_view;

    g_initialized = true;
    return true;
}

void shadow_pass_shutdown() {
    if (!g_initialized) return;

    // Destroy in reverse ownership order: views first (depend on image), then samplers, then images.
    if (g_state.shadow_ds_view.id != 0)              { sg_destroy_view(g_state.shadow_ds_view); g_state.shadow_ds_view = {}; }
    if (g_state.shadow_tex_view.id != 0)             { sg_destroy_view(g_state.shadow_tex_view); g_state.shadow_tex_view = {}; }
    if (g_state.shadow_debug_color_view.id != 0)     { sg_destroy_view(g_state.shadow_debug_color_view); g_state.shadow_debug_color_view = {}; }
    if (g_state.shadow_debug_view.id != 0)           { sg_destroy_view(g_state.shadow_debug_view); g_state.shadow_debug_view = {}; }
    if (g_state.shadow_sampler.id != 0)              { sg_destroy_sampler(g_state.shadow_sampler); g_state.shadow_sampler = {}; }
    if (g_state.shadow_debug_sampler.id != 0)        { sg_destroy_sampler(g_state.shadow_debug_sampler); g_state.shadow_debug_sampler = {}; }
    if (g_state.shadow_map_img.id != 0)              { sg_destroy_image(g_state.shadow_map_img); g_state.shadow_map_img = {}; }
    if (g_state.shadow_debug_img.id != 0)            { sg_destroy_image(g_state.shadow_debug_img); g_state.shadow_debug_img = {}; }

    // sg_attachments is a plain struct — no destroy function.
    memset(&g_state, 0, sizeof(g_state));
    g_initialized = false;
}

const ShadowPassState* shadow_pass_state() {
    return g_initialized ? &g_state : nullptr;
}

glm::mat4 shadow_compute_light_view_proj(const DirectionalLight& light,
                                          float ortho_half_size,
                                          float ortho_near,
                                          float ortho_far) {
    // Scene center (hardcoded for Phase 1).
    const glm::vec3 scene_center(0.0f, 0.0f, 0.0f);

    // Normalize light direction.
    glm::vec3 dir = glm::normalize(glm::make_vec3(light.direction));

    // Light "eye" position — placed on the light side of the scene (dir is surface→light).
    const float eye_dist = ortho_far * 0.5f;
    glm::vec3 eye_pos = scene_center + dir * eye_dist;

    // Up-vector selection: if light is nearly vertical (along ±Y), use X as up.
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(dir, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Light view matrix: lookAt from eye_pos toward scene_center.
    glm::mat4 light_view = glm::lookAt(eye_pos, scene_center, up);

    // Light projection: symmetric orthographic frustum.
    glm::mat4 light_proj = glm::ortho(-ortho_half_size, ortho_half_size,
                                       -ortho_half_size, ortho_half_size,
                                        ortho_near, ortho_far);

    return light_proj * light_view;
}

 // ---------------------------------------------------------------------------
// Debug overlay state (file-static — accessible via public API declarations)
// ---------------------------------------------------------------------------

static bool g_shadow_debug_visible = false;
static sg_buffer g_shadow_debug_vbuf = {};
static sg_buffer g_shadow_debug_ibuf = {};

void shadow_debug_set_visible(bool v) {
    g_shadow_debug_visible = v;
}

bool shadow_debug_is_visible() {
    return g_shadow_debug_visible;
}

void shadow_debug_init() {
    if (g_shadow_debug_vbuf.id != 0) return;  // already initialized

    // Static vertex buffer: 4 vertices × 4 floats (NDC XY + UV)
    // Quad in bottom-right of a 1280×720 window: NDC X ∈ [0.5, 1.0], Y ∈ [-0.5, 0.0]
    float quad_verts[16] = {
        //  x     y     u     v
         0.5f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.0f, 1.0f,
    };

    sg_buffer_desc vdesc = {};
    vdesc.size               = sizeof(quad_verts);
    vdesc.usage.immutable    = true;
    vdesc.data               = { quad_verts, sizeof(quad_verts) };
    vdesc.label              = "shadow-debug-vbuf";

    g_shadow_debug_vbuf = sg_make_buffer(&vdesc);
    if (sg_query_buffer_state(g_shadow_debug_vbuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug VBO creation failed\n");
        return;
    }

    // Index buffer: 6 indices for two triangles
    uint32_t quad_indices[6] = { 0, 1, 2, 0, 2, 3 };

  sg_buffer_desc idesc = {};
    idesc.size             = sizeof(quad_indices);
    idesc.usage.immutable  = true;
    idesc.data             = { quad_indices, sizeof(quad_indices) };
    idesc.label            = "shadow-debug-ibuf";

    g_shadow_debug_ibuf = sg_make_buffer(&idesc);
    if (sg_query_buffer_state(g_shadow_debug_ibuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: shadow debug IBO creation failed\n");
        sg_destroy_buffer(g_shadow_debug_vbuf);
        g_shadow_debug_vbuf = {};
        return;
    }

    printf("[renderer] shadow_debug_init completed (VBO %u, IBO %u)\n",
           g_shadow_debug_vbuf.id, g_shadow_debug_ibuf.id);
}

void shadow_debug_shutdown() {
    if (g_shadow_debug_ibuf.id != 0) {
        sg_destroy_buffer(g_shadow_debug_ibuf);
        g_shadow_debug_ibuf = {};
    }
    if (g_shadow_debug_vbuf.id != 0) {
        sg_destroy_buffer(g_shadow_debug_vbuf);
        g_shadow_debug_vbuf = {};
    }
}

void shadow_debug_draw(sg_pipeline debug_pip) {
    // Early-return if any required resource is missing (init failed or not yet initialized).
    if (debug_pip.id == 0 || g_shadow_debug_vbuf.id == 0 || g_shadow_debug_ibuf.id == 0) return;

    // Verify R32F color texture view and debug sampler are available.
    const ShadowPassState* sps = shadow_pass_state();
    if (!sps || sps->shadow_debug_view.id == 0 || sps->shadow_debug_sampler.id == 0) return;

    sg_bindings bind = {};
    bind.vertex_buffers[0] = g_shadow_debug_vbuf;
    bind.index_buffer      = g_shadow_debug_ibuf;
    bind.views[VIEW_shadow_tex] = sps->shadow_debug_view;
    bind.samplers[SMP_debug_smp] = sps->shadow_debug_sampler;

    sg_apply_pipeline(debug_pip);
    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);
}
