#include "shadow_pass.h"

#include "renderer.h"

#include "sokol_gfx.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>

namespace {

static ShadowPassState g_state = {};
static bool g_initialized = false;

// Destroy any resources created so far on failure (cleanup for mid-init errors).
// Same ownership order as shadow_pass_shutdown(): views first (depend on image),
// then sampler, then image.
static void cleanup_shadow_resources() {
    if (g_state.shadow_ds_view.id != 0)          { sg_destroy_view(g_state.shadow_ds_view); g_state.shadow_ds_view = {}; }
    if (g_state.shadow_tex_view.id != 0)         { sg_destroy_view(g_state.shadow_tex_view); g_state.shadow_tex_view = {}; }
    if (g_state.shadow_sampler.id != 0)          { sg_destroy_sampler(g_state.shadow_sampler); g_state.shadow_sampler = {}; }
    if (g_state.shadow_map_img.id != 0)          { sg_destroy_image(g_state.shadow_map_img); g_state.shadow_map_img = {}; }
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

    // --- Build attachments struct for sg_begin_pass --------------------------
    g_state.shadow_attachments.depth_stencil = g_state.shadow_ds_view;

    g_initialized = true;
    return true;
}

void shadow_pass_shutdown() {
    if (!g_initialized) return;

    // Destroy in reverse ownership order: views first (depend on image), then sampler, then image.
    if (g_state.shadow_ds_view.id != 0)          { sg_destroy_view(g_state.shadow_ds_view); g_state.shadow_ds_view = {}; }
    if (g_state.shadow_tex_view.id != 0)         { sg_destroy_view(g_state.shadow_tex_view); g_state.shadow_tex_view = {}; }
    if (g_state.shadow_sampler.id != 0)          { sg_destroy_sampler(g_state.shadow_sampler); g_state.shadow_sampler = {}; }
    if (g_state.shadow_map_img.id != 0)          { sg_destroy_image(g_state.shadow_map_img); g_state.shadow_map_img = {}; }

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
