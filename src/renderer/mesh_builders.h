#ifndef VIBEATON_MESH_BUILDERS_H
#define VIBEATON_MESH_BUILDERS_H

#include "renderer.h"

// ---------------------------------------------------------------------------
// Mesh store ref counting — used by engine to track mesh component lifecycle.
// ---------------------------------------------------------------------------

void       mesh_store_ref_inc(uint32_t id);
void       mesh_store_ref_dec(uint32_t id);

#endif // VIBEATON_MESH_BUILDERS_H
