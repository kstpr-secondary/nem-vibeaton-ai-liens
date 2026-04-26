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

   // Pick the largest indexed mesh by total index count.
    // A glTF file may contain multiple meshes (body, engines, wings, cockpit);
    // the largest one is almost always the main body. Internal geometry from
    // smaller meshes would look wrong when rendered alone.
    size_t best_count = 0;
    cgltf_mesh* best_mesh = nullptr;

    for (size_t m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& mesh = data->meshes[m];
        for (size_t p = 0; p < mesh.primitives_count; ++p) {
            if (mesh.primitives[p].type != cgltf_primitive_type_triangles)
                continue;
            const auto& prim = mesh.primitives[p];
            if (prim.indices == nullptr)
                continue;
            size_t idx_count = prim.indices->count;
            if (idx_count > best_count) {
                best_count = idx_count;
                best_mesh = &data->meshes[m];
            }
        }
    }

    if (best_mesh != nullptr) {
        // Find the largest indexed triangle primitive within the best mesh.
        const cgltf_primitive* best_prim = nullptr;
        for (size_t p = 0; p < best_mesh->primitives_count; ++p) {
            if (best_mesh->primitives[p].type != cgltf_primitive_type_triangles)
                continue;
            if (best_mesh->primitives[p].indices == nullptr)
                continue;
            if (best_mesh->primitives[p].indices->count >= best_count) {
                best_prim = &best_mesh->primitives[p];
                break;
            }
        }

        if (best_prim != nullptr) {
            cgltf_accessor* pos_acc  = nullptr;
            cgltf_accessor* norm_acc = nullptr;
            cgltf_accessor* uv_acc   = nullptr;

            for (size_t a = 0; a < best_prim->attributes_count; ++a) {
                const cgltf_attribute& attr = best_prim->attributes[a];
                switch (attr.type) {
                    case cgltf_attribute_type_position: pos_acc  = attr.data; break;
                    case cgltf_attribute_type_normal:   norm_acc = attr.data; break;
                    case cgltf_attribute_type_texcoord: uv_acc   = attr.data; break;
                    default: break;
                }
            }

            if (pos_acc) {
                size_t n = pos_acc->count;

                out.positions.resize(n * 3);
                cgltf_accessor_unpack_floats(pos_acc, out.positions.data(), n * 3);

                if (norm_acc && norm_acc->count == n) {
                    out.normals.resize(n * 3);
                    cgltf_accessor_unpack_floats(norm_acc, out.normals.data(), n * 3);
                }

                if (uv_acc && uv_acc->count == n) {
                    out.uvs.resize(n * 2);
                    cgltf_accessor_unpack_floats(uv_acc, out.uvs.data(), n * 2);
                }

                if (best_prim->indices) {
                    size_t cnt = best_prim->indices->count;
                    out.indices.resize(cnt);
                    for (size_t i = 0; i < cnt; ++i)
                        out.indices[cnt - 1 - i] = static_cast<uint32_t>(cgltf_accessor_read_index(best_prim->indices, i));
                } else {
                    out.indices.resize(n);
                    std::iota(out.indices.begin(), out.indices.end(), 0u);
                }
            }
        }
    }

    cgltf_free(data);

    if (out.empty())
        fprintf(stderr, "[ENGINE] glTF %s: no triangle primitives found\n", relative_path);

    return out;
}
