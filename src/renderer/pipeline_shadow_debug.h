#ifndef VIBEATON_PIPELINE_SHADOW_DEBUG_H
#define VIBEATON_PIPELINE_SHADOW_DEBUG_H

#include <cstdint>

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the shadow debug overlay pipeline using the precompiled
// shadow_debug.glsl.h shader. On failure, returns magenta_fallback.
sg_pipeline create_pipeline_shadow_debug(sg_pipeline magenta_fallback);

#endif // VIBEATON_PIPELINE_SHADOW_DEBUG_H
