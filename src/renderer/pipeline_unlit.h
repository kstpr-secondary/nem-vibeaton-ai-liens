#ifndef VIBEATON_PIPELINE_UNLIT_H
#define VIBEATON_PIPELINE_UNLIT_H

#include <cstdint>

// Manual forward declaration of sokol_gfx handles to avoid header redefinitions.
// These match the definitions in sokol_gfx.h.
#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the unlit forward pipeline using the precompiled unlit.glsl.h shader.
// On sg_make_shader or sg_make_pipeline failure, logs an error and returns
// magenta_fallback so the caller always gets a usable pipeline handle.
sg_pipeline create_pipeline_unlit(sg_pipeline magenta_fallback);

#endif // VIBEATON_PIPELINE_UNLIT_H
