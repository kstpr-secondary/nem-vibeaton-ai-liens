#pragma once

#include "asset_import.h"
#include <renderer.h>

// Convert an ImportedMesh to the renderer's interleaved Vertex layout and
// upload via renderer_upload_mesh(). Returns {0} (invalid handle) on failure.
RendererMeshHandle asset_bridge_upload(const ImportedMesh& mesh);
