#ifndef VIBEATON_MESH_BUILDERS_H
#define VIBEATON_MESH_BUILDERS_H

#include "renderer.h"

// ---------------------------------------------------------------------------
// Internal mesh-store accessors — used by renderer.cpp for draw dispatch.
// Not part of the public renderer API.
// Requires sokol_gfx.h to be included before this header.
// ---------------------------------------------------------------------------

sg_buffer  mesh_vbuf_get(uint32_t id);
sg_buffer  mesh_ibuf_get(uint32_t id);
uint32_t   mesh_index_count_get(uint32_t id);
void       mesh_store_shutdown();

#endif // VIBEATON_MESH_BUILDERS_H
