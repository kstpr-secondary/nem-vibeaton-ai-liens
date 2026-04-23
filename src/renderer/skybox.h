#ifndef VIBEATON_SKYBOX_H
#define VIBEATON_SKYBOX_H

#include "renderer.h"

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
struct sg_image    { uint32_t id; };
#endif

// ---------------------------------------------------------------------------
// Skybox module — cubemap-based environment backdrop.
//
// Lifecycle: call skybox_init_resources() after sg_setup(), skybox_shutdown()
// before sg_shutdown(). draw_skybox_pass() is called from renderer_end_frame()
// when a skybox handle has been set.
// ---------------------------------------------------------------------------

// Create the skybox pipeline.  Pass the magenta fallback so any failure
// returns a usable pipeline handle (same pattern as create_pipeline_unlit).
sg_pipeline skybox_create_pipeline(sg_pipeline magenta_fallback);

// Allocate unit-cube VBO / IBO and the cube-map sampler.
// Must be called after sg_setup().
void skybox_init_resources();

// Destroy VBO / IBO / sampler.
void skybox_shutdown();

// Draw the skybox for the current frame.
//   pip          — skybox pipeline (created by skybox_create_pipeline)
//   cubemap_img  — sg_image returned by texture_get(skybox_handle.id)
//   proj[16]     — column-major projection matrix
//   view[16]     — column-major view matrix (translation is stripped internally)
void draw_skybox_pass(
    sg_pipeline pip,
    sg_image    cubemap_img,
    const float proj[16],
    const float view[16]);

#endif // VIBEATON_SKYBOX_H
