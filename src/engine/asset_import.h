#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// Forward declarations — actual types are in renderer.h and cgltf.h
struct cgltf_data;

// Include renderer.h for RendererTextureHandle full type.
// This is safe: engine links to renderer (PUBLIC), no circular dependency.
#include <renderer.h>

// Extracted texture from a glTF material.
struct ExtractedTexture {
    RendererTextureHandle handle;  // GPU texture handle (valid if id != 0)
    glm::vec3             base_color;  // RGB fallback color from baseColorFactor
};

// Flat extracted mesh data from a single primitive.
// Used by both the glTF and OBJ loaders; consumed by asset_bridge.
struct ImportedMesh {
    std::vector<float>    positions;  // x,y,z per vertex — always populated on success
    std::vector<float>    normals;    // nx,ny,nz per vertex — empty if source had none
    std::vector<float>    uvs;        // u,v per vertex — empty if source had none
    std::vector<uint32_t> indices;    // triangle index list — always populated on success
    RendererTextureHandle texture;    // valid if material has baseColorTexture (id != 0)

    bool empty() const { return positions.empty(); }
};

// Load the largest indexed mesh from a glTF/GLB file by index count.
// A glTF file may contain multiple meshes (body, engines, wings, cockpit);
// the largest one is almost always the main body. Internal geometry from
// smaller meshes would look wrong when rendered alone.
// relative_path is resolved via ASSET_ROOT (e.g. "Asteroid_1a.glb").
// Extracts baseColor textures from glTF materials and uploads them to the
// renderer; returns up to one texture per material with a baseColorTexture.
// Returns empty ImportedMesh on failure; logs [ENGINE] warnings.
ImportedMesh asset_import_gltf(const char* relative_path);

// Extract textures from already-loaded glTF data.
// Called internally by asset_import_gltf() after cgltf_load_buffers().
// Returns vector of ExtractedTexture (one per material with baseColorTexture).
std::vector<ExtractedTexture> asset_extract_textures(cgltf_data* data,
                                                      const char* rel_path);
