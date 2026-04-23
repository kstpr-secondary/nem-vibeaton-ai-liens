#ifndef VIBEATON_PIPELINE_LAMBERTIAN_H
#define VIBEATON_PIPELINE_LAMBERTIAN_H

#include <cstdint>

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the lambertian forward pipeline using the precompiled lambertian.glsl.h shader.
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_lambertian(sg_pipeline magenta_fallback);

// Returns the cached lambertian pipeline handle.
sg_pipeline get_lambertian_pipeline();

#endif // VIBEATON_PIPELINE_LAMBERTIAN_H
