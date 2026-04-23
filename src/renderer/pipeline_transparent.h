#ifndef VIBEATON_PIPELINE_TRANSPARENT_H
#define VIBEATON_PIPELINE_TRANSPARENT_H

#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates a transparent forward pipeline using the unlit shader with alpha blending.
// Depth write OFF, depth test ON, src_alpha/one_minus_src_alpha blend.
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_transparent(sg_pipeline magenta_fallback);

#endif // VIBEATON_PIPELINE_TRANSPARENT_H
