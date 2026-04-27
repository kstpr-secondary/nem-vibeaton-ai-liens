#ifndef VIBEATON_PIPELINE_BLINNPHONG_H
#define VIBEATON_PIPELINE_BLINNPHONG_H

#include <cstdint>

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the Blinn-Phong forward pipeline using the precompiled blinnphong.glsl.h shader.
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_blinnphong(sg_pipeline magenta_fallback);

// Returns the cached Blinn-Phong pipeline handle.
sg_pipeline get_blinnphong_pipeline();

#endif // VIBEATON_PIPELINE_BLINNPHONG_H
