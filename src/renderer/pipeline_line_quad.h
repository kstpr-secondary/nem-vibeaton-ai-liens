#ifndef VIBEATON_PIPELINE_LINE_QUAD_H
#define VIBEATON_PIPELINE_LINE_QUAD_H

#ifndef SOKOL_GFX_INCLUDED
struct sg_pipeline { uint32_t id; };
#endif

// Creates the line-quad billboard pipeline with alpha blending.
// Vertex layout: position (float3) + color (float4).
// On failure, returns magenta_fallback.
sg_pipeline create_pipeline_line_quad(sg_pipeline magenta_fallback);

#endif // VIBEATON_PIPELINE_LINE_QUAD_H
