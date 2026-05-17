#ifndef VIBEATON_SHADOW_PASS_H
#define VIBEATON_SHADOW_PASS_H

#include <cstdint>

// shadow_pass.h uses sokol_gfx handle types as struct members (not pointers),
// so the full types must be available. Include sokol_gfx.h if it hasn't been
// included yet (i.e., SOKOL_GFX_INCLUDED is not defined).
#if !defined(SOKOL_GFX_INCLUDED)
#include "sokol_gfx.h"
#endif

// Pull in DirectionalLight and glm::mat4 — renderer.h has no circular dep on shadow_pass.
#include "renderer.h"

// Shadow map resolution.
static constexpr int k_shadow_map_size = 2048;

// Orthographic frustum half-extent (covers ~1.2 km diameter for game arena).
static constexpr float k_shadow_ortho_half_size = 600.0f;

// Orthographic near/far clipping planes.
static constexpr float k_shadow_near = -1200.0f;
static constexpr float k_shadow_far  = 1200.0f;

// Shadow pass GPU state — owns all shadow-map resources.
struct ShadowPassState {
    sg_image  shadow_map_img  = {};  // depth-stencil image (2048×2048)
    sg_view   shadow_ds_view  = {};  // depth-stencil attachment view (write target)
    sg_view   shadow_tex_view = {};  // texture view (read target for shader sampling)
    sg_sampler shadow_sampler = {};  // comparison sampler (LESS, linear, clamp-to-edge)
    sg_attachments shadow_attachments = {};  // attachments struct for sg_begin_pass
};

// Initialize all shadow map GPU resources. Must be called after sg_setup().
// Returns true if initialization succeeded (shadow_map_img handle is non-zero).
bool shadow_pass_init();

// Destroy all shadow map GPU resources. Must be called before sg_shutdown().
void shadow_pass_shutdown();

// Return a pointer to the live ShadowPassState (non-null after init, null before/after shutdown).
const ShadowPassState* shadow_pass_state();

// Compute the light-space view-projection matrix for the orthographic shadow map.
// scene_center is hardcoded to {0,0,0} in Phase 1; parameters are configurable.
glm::mat4 shadow_compute_light_view_proj(const DirectionalLight& light,
                                          float ortho_half_size,
                                          float ortho_near,
                                          float ortho_far);

#endif // VIBEATON_SHADOW_PASS_H
