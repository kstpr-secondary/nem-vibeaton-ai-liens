#ifndef VIBEATON_PIPELINE_SHADOW_H
#define VIBEATON_PIPELINE_SHADOW_H

#include <cstdint>

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the shadow-depth pipeline using the precompiled shadow_depth.glsl.h shader.
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_shadow(sg_pipeline magenta_fallback);

// Returns the cached shadow-depth pipeline handle.
sg_pipeline get_shadow_pipeline();

#endif // VIBEATON_PIPELINE_SHADOW_H
