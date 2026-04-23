#pragma once

#include "asset_import.h"  // ImportedMesh

// Load mesh from an OBJ file.
// relative_path is resolved via ASSET_ROOT.
// Returns empty ImportedMesh on failure; logs [ENGINE] warnings.
ImportedMesh asset_import_obj(const char* relative_path);
