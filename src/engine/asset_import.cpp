#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "asset_import.h"
#include <paths.h>

#include <cstdio>
#include <numeric>
#include <string>

ImportedMesh asset_import_gltf(const char* relative_path) {
    ImportedMesh out;

    std::string full_path = std::string(ASSET_ROOT) + "/" + relative_path;

    cgltf_options opts{};
    cgltf_data*   data = nullptr;

    if (cgltf_parse_file(&opts, full_path.c_str(), &data) != cgltf_result_success) {
        fprintf(stderr, "[ENGINE] glTF load failed (parse): %s\n", full_path.c_str());
        return out;
    }

    // MANDATORY: parse only reads the JSON header; buffers are loaded separately.
    if (cgltf_load_buffers(&opts, data, full_path.c_str()) != cgltf_result_success) {
        fprintf(stderr, "[ENGINE] glTF load failed (buffers): %s\n", full_path.c_str());
        cgltf_free(data);
        return out;
    }

    // Warn about unsupported features — don't access their data
    if (data->skins_count > 0)
        fprintf(stderr, "[ENGINE] glTF %s: skeletal animation unsupported, geometry only\n", relative_path);
    if (data->animations_count > 0)
        fprintf(stderr, "[ENGINE] glTF %s: animations unsupported, geometry only\n", relative_path);

    // Walk meshes → find first triangle primitive
    for (size_t m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& mesh = data->meshes[m];
        for (size_t p = 0; p < mesh.primitives_count; ++p) {
            const cgltf_primitive& prim = mesh.primitives[p];

            if (prim.type != cgltf_primitive_type_triangles) {
                fprintf(stderr, "[ENGINE] glTF %s: non-triangle primitive skipped\n", relative_path);
                continue;
            }

            // Collect attribute accessors
            cgltf_accessor* pos_acc  = nullptr;
            cgltf_accessor* norm_acc = nullptr;
            cgltf_accessor* uv_acc   = nullptr;

            for (size_t a = 0; a < prim.attributes_count; ++a) {
                const cgltf_attribute& attr = prim.attributes[a];
                switch (attr.type) {
                    case cgltf_attribute_type_position: pos_acc  = attr.data; break;
                    case cgltf_attribute_type_normal:   norm_acc = attr.data; break;
                    case cgltf_attribute_type_texcoord: uv_acc   = attr.data; break;
                    default: break;
                }
            }

            if (!pos_acc) {
                fprintf(stderr, "[ENGINE] glTF %s: primitive missing POSITION, skipping\n", relative_path);
                continue;
            }

            size_t n = pos_acc->count;

            // Positions (required)
            out.positions.resize(n * 3);
            cgltf_accessor_unpack_floats(pos_acc, out.positions.data(), n * 3);

            // Normals (optional)
            if (norm_acc && norm_acc->count == n) {
                out.normals.resize(n * 3);
                cgltf_accessor_unpack_floats(norm_acc, out.normals.data(), n * 3);
            }

            // UVs (optional)
            if (uv_acc && uv_acc->count == n) {
                out.uvs.resize(n * 2);
                cgltf_accessor_unpack_floats(uv_acc, out.uvs.data(), n * 2);
            }

            // Indices
            if (prim.indices) {
                size_t cnt = prim.indices->count;
                out.indices.resize(cnt);
                for (size_t i = 0; i < cnt; ++i)
                    out.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i));
            } else {
                // Unindexed mesh — generate sequential indices
                out.indices.resize(n);
                std::iota(out.indices.begin(), out.indices.end(), 0u);
            }

            cgltf_free(data);
            return out;  // first valid primitive wins
        }
    }

    fprintf(stderr, "[ENGINE] glTF %s: no triangle primitives found\n", relative_path);
    cgltf_free(data);
    return out;
}
