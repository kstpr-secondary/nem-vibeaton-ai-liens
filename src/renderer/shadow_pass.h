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

// Orthographic frustum half-extent (covers ~50 unit diameter for shadow demo).
static constexpr float k_shadow_ortho_half_size = 25.0f;

// Orthographic near/far clipping planes.
static constexpr float k_shadow_near = -60.0f;
static constexpr float k_shadow_far  = 60.0f;

// Shadow pass GPU state — owns all shadow-map resources.
struct ShadowPassState {
    sg_image  shadow_map_img  = {};  // depth-stencil image (2048×2048)
    sg_view   shadow_ds_view  = {};  // depth-stencil attachment view (write target)
    sg_view   shadow_tex_view = {};  // texture view (read target for shadow shader sampling)
    sg_image  shadow_debug_img = {}; // R32F color image for debug visualization (no compare needed)
    sg_view   shadow_debug_color_view = {}; // color-attachment view (render target for depth-as-RGBA)
    sg_view   shadow_debug_view = {}; // texture view for debug sampler
    sg_sampler shadow_sampler = {};  // comparison sampler (LESS, linear, clamp-to-edge)
    sg_sampler shadow_debug_sampler = {};  // raw sampler for debug visualization (no compare)
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

// Debug overlay: control visibility of the shadow map depth visualization.
void      shadow_debug_set_visible(bool v);
bool      shadow_debug_is_visible();

// Debug overlay: initialize vertex/index buffers for the debug quad.
void      shadow_debug_init();

// Debug overlay: destroy vertex/index buffers for the debug quad.
void      shadow_debug_shutdown();

// Debug overlay: draw the shadow map as a greyscale quad in the bottom-right corner.
void      shadow_debug_draw(sg_pipeline debug_pip);

#endif // VIBEATON_SHADOW_PASS_H
