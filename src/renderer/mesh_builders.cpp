#include "sokol_gfx.h"
#include "mesh_builders.h"
#include "renderer.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Module-level mesh store — index 0 reserved (invalid handle id == 0)
// ---------------------------------------------------------------------------

namespace {

static sg_buffer  s_vbufs[512]        = {};
static sg_buffer  s_ibufs[512]        = {};
static uint32_t   s_index_counts[512] = {};
static uint32_t   s_next_id           = 1;

} // namespace

// ---------------------------------------------------------------------------
// Internal accessors (used by renderer.cpp for draw dispatch)
// ---------------------------------------------------------------------------

sg_buffer mesh_vbuf_get(uint32_t id)        { return s_vbufs[id]; }
sg_buffer mesh_ibuf_get(uint32_t id)        { return s_ibufs[id]; }
uint32_t  mesh_index_count_get(uint32_t id) { return s_index_counts[id]; }

void mesh_store_shutdown() {
    for (uint32_t i = 1; i < s_next_id; ++i) {
        sg_destroy_buffer(s_vbufs[i]);
        sg_destroy_buffer(s_ibufs[i]);
        s_vbufs[i] = {};
        s_ibufs[i] = {};
    }
    s_next_id = 1;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_upload_mesh(
    const Vertex*   vertices,
    uint32_t        vertex_count,
    const uint32_t* indices,
    uint32_t        index_count)
{
    if (s_next_id >= 512) {
        printf("[renderer] ERROR: mesh store full (max 511 meshes)\n");
        return {0};
    }

    sg_buffer_desc vdesc = {};
    vdesc.usage.vertex_buffer = true;
    vdesc.usage.immutable     = true;
    vdesc.data  = { vertices, vertex_count * sizeof(Vertex) };
    vdesc.label = "mesh-vbuf";

    sg_buffer vbuf = sg_make_buffer(&vdesc);
    if (sg_query_buffer_state(vbuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: vertex buffer creation failed\n");
        return {0};
    }

    sg_buffer_desc idesc = {};
    idesc.usage.index_buffer = true;
    idesc.usage.immutable    = true;
    idesc.data  = { indices, index_count * sizeof(uint32_t) };
    idesc.label = "mesh-ibuf";

    sg_buffer ibuf = sg_make_buffer(&idesc);
    if (sg_query_buffer_state(ibuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: index buffer creation failed\n");
        sg_destroy_buffer(vbuf);
        return {0};
    }

    uint32_t id = s_next_id++;
    s_vbufs[id]        = vbuf;
    s_ibufs[id]        = ibuf;
    s_index_counts[id] = index_count;

    return { id };
}

// Stubs — full implementations added in R-021 and R-022
RendererMeshHandle renderer_make_sphere_mesh(float /*radius*/, int /*subdivisions*/) {
    return {};
}

RendererMeshHandle renderer_make_cube_mesh(float /*half_extent*/) {
    return {};
}
