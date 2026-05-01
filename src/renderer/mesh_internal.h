#pragma once

// Requires sokol_gfx.h to be included before this header (no include guard in
// sokol_gfx.h — including it twice in one TU triggers redefinition errors).
// ---------------------------------------------------------------------------
// AABB — world-space bounding box (center + half-extents per axis)
// ---------------------------------------------------------------------------

struct MeshAABB {
    float center[3] = {0.0f, 0.0f, 0.0f};
    float half      = 0.0f;
};

// ---------------------------------------------------------------------------
// Internal mesh-store accessors — used by renderer.cpp for draw dispatch.
// Not part of the public API.
// ---------------------------------------------------------------------------

sg_buffer  mesh_vbuf_get(uint32_t id);
sg_buffer  mesh_ibuf_get(uint32_t id);
uint32_t   mesh_index_count_get(uint32_t id);
MeshAABB   mesh_aabb_get(uint32_t id);
void       mesh_store_shutdown();
