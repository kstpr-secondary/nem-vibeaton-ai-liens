#pragma once

#include <cstdint>
#include <vector>

// Flat extracted mesh data from a single primitive.
// Used by both the glTF and OBJ loaders; consumed by asset_bridge.
struct ImportedMesh {
    std::vector<float>    positions;  // x,y,z per vertex — always populated on success
    std::vector<float>    normals;    // nx,ny,nz per vertex — empty if source had none
    std::vector<float>    uvs;        // u,v per vertex — empty if source had none
    std::vector<uint32_t> indices;    // triangle index list — always populated on success

    bool empty() const { return positions.empty(); }
};

// Load the first triangle primitive from a glTF/GLB file.
// relative_path is resolved via ASSET_ROOT (e.g. "Asteroid_1a.glb").
// Returns empty ImportedMesh on failure; logs [ENGINE] warnings.
ImportedMesh asset_import_gltf(const char* relative_path);
