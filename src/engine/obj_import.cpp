#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "obj_import.h"
#include <paths.h>

#include <cstdio>
#include <numeric>
#include <string>

ImportedMesh asset_import_obj(const char* relative_path) {
    ImportedMesh out;

    std::string full_path = std::string(ASSET_ROOT) + "/" + relative_path;

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                               full_path.c_str(), /*mtl_basedir=*/nullptr,
                               /*triangulate=*/true);

    if (!warn.empty())
        fprintf(stderr, "[ENGINE] OBJ %s warning: %s\n", relative_path, warn.c_str());
    if (!ok) {
        fprintf(stderr, "[ENGINE] OBJ load failed: %s — %s\n", relative_path, err.c_str());
        return out;
    }
    if (shapes.empty()) {
        fprintf(stderr, "[ENGINE] OBJ %s: no shapes found\n", relative_path);
        return out;
    }

    // First shape only (MVP)
    const tinyobj::shape_t& shape = shapes[0];

    bool has_normals  = !attrib.normals.empty();
    bool has_texcoord = !attrib.texcoords.empty();

    size_t index_offset = 0;
    for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
        int fv = static_cast<int>(shape.mesh.num_face_vertices[f]);
        if (fv != 3) {
            // triangulate=true guarantees triangles; skip any stray non-tri
            index_offset += static_cast<size_t>(fv);
            continue;
        }

        for (int v = 0; v < 3; ++v) {
            const tinyobj::index_t& idx = shape.mesh.indices[index_offset + v];

            // Position (always present)
            out.positions.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
            out.positions.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
            out.positions.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

            // Normal — fall back to Y-up if absent
            if (has_normals && idx.normal_index >= 0) {
                out.normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
                out.normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
                out.normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
            } else {
                out.normals.push_back(0.f);
                out.normals.push_back(1.f);
                out.normals.push_back(0.f);
            }

            // UV — fall back to (0,0) if absent
            if (has_texcoord && idx.texcoord_index >= 0) {
                out.uvs.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                out.uvs.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
            } else {
                out.uvs.push_back(0.f);
                out.uvs.push_back(0.f);
            }
        }

        index_offset += 3;
    }

    // OBJ expansion produces unique vertices per face corner — indices are sequential
    size_t vert_count = out.positions.size() / 3;
    out.indices.resize(vert_count);
    std::iota(out.indices.begin(), out.indices.end(), 0u);

    return out;
}
