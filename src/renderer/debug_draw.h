#ifndef VIBEATON_DEBUG_DRAW_H
#define VIBEATON_DEBUG_DRAW_H

// ---------------------------------------------------------------------------
// Internal types shared between debug_draw.cpp and renderer.cpp
// ---------------------------------------------------------------------------

// Per-vertex layout for line-quad billboards — matches line_quad.glsl inputs.
struct LineQuadVertex {
    float position[3];
    float color[4];
};

// One pre-expanded billboard quad: 4 world-space corner vertices ready for upload.
// 6 indices (0,1,2  0,2,3) are implicit — use a shared static index buffer at draw time.
struct LineQuadCommand {
    LineQuadVertex verts[4];
};

// ---------------------------------------------------------------------------
// Billboard math — used by renderer_enqueue_line_quad inside renderer.cpp
// ---------------------------------------------------------------------------

// Compute CPU-billboarded quad corners from two world-space endpoints.
//   view[16]  — column-major 4x4 view matrix (RendererCamera::view).
//   width     — world-space thickness of the beam.
//   color[4]  — RGBA, copied to every corner vertex.
// Fills cmd->verts[0..3] with the 4 world-space corner positions.
// Returns false for degenerate segments (zero length or camera co-linear).
bool debug_draw_compute_billboard(
    LineQuadCommand*  cmd,
    const float       p0[3],
    const float       p1[3],
    float             width,
    const float       color[4],
    const float       view[16]);

#endif // VIBEATON_DEBUG_DRAW_H
