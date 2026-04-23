#ifndef VIBEATON_PIPELINE_SKYBOX_H
#define VIBEATON_PIPELINE_SKYBOX_H

#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the skybox pipeline: position-only vertex layout, depth write OFF,
// depth test LESS_EQUAL, front-face culling (renders inside of unit cube).
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_skybox(sg_pipeline magenta_fallback);

#endif // VIBEATON_PIPELINE_SKYBOX_H
