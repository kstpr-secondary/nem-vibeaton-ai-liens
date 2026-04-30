#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "asset_import.h"
#include <paths.h>
#include <renderer.h>
#include <texture.h>

// stb_image — single-include implementation in this TU only.
// STBI_ONLY_RGB + STBI_ONLY_RGBA minimize code size; we always convert to RGBA.
#define STBI_ONLY_RGB
#define STBI_ONLY_RGBA
#define STBI_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>
#include <numeric>
#include <string>
#include <unordered_map>

// Cache texture handles so the same glTF file loaded multiple times
// does not re-upload identical GPU textures.
static std::unordered_map<uint64_t, RendererTextureHandle>& texture_cache() {
    static std::unordered_map<uint64_t, RendererTextureHandle> cache;
    return cache;
}

std::vector<ExtractedTexture> asset_extract_textures(cgltf_data* data,
                                                      const char* rel_path) {
    std::vector<ExtractedTexture> result;

    for (cgltf_size m = 0; m < data->materials_count; ++m) {
        const cgltf_material& mat = data->materials[m];
        const cgltf_texture_view& tex_view =
            mat.pbr_metallic_roughness.base_color_texture;

        if (!tex_view.texture || !tex_view.texture->image)
            continue;

        const cgltf_image* img = tex_view.texture->image;
        if (!img || !img->buffer_view)
            continue;

        const cgltf_buffer_view* bv = img->buffer_view;
        const uint8_t* raw_bytes =
            reinterpret_cast<const uint8_t*>(bv->buffer->data) + bv->offset;
        cgltf_size byte_count = bv->size;

        // Use buffer_view pointer as cache key to avoid re-uploading.
        uint64_t cache_key =
            reinterpret_cast<uint64_t>(bv->buffer->data) ^
            (static_cast<uint64_t>(bv->offset) << 16);

        auto cached = texture_cache().find(cache_key);
        if (cached != texture_cache().end()) {
            ExtractedTexture et;
            et.handle = cached->second;
            const auto& pbr = mat.pbr_metallic_roughness;
            if (pbr.base_color_factor[0] != 1.f ||
                pbr.base_color_factor[1] != 1.f ||
                pbr.base_color_factor[2] != 1.f ||
                pbr.base_color_factor[3] != 1.f) {
                et.base_color = glm::vec3(pbr.base_color_factor[0],
                                          pbr.base_color_factor[1],
                                          pbr.base_color_factor[2]);
            } else {
                et.base_color = glm::vec3(1.f);
            }
            result.push_back(std::move(et));
            continue;
        }

        // Decode texture from raw PNG/JPEG bytes.
        int w = 0, h = 0, ch = 0;
        uint8_t* pixels = stbi_load_from_memory(raw_bytes,
            static_cast<int>(byte_count), &w, &h, &ch, 4);

        if (!pixels) {
            fprintf(stderr, "[ENGINE] glTF %s: stbi_load_from_memory failed "
                "for material[%zu] (%zu bytes)\n", rel_path, m, byte_count);
            continue;
        }

        RendererTextureHandle tex = renderer_upload_texture_from_memory(
            pixels, w, h, 4);
        stbi_image_free(pixels);

        if (!renderer_handle_valid(tex)) {
            fprintf(stderr, "[ENGINE] glTF %s: renderer_upload_texture_from_memory "
                "failed for material[%zu]\n", rel_path, m);
            continue;
        }

        texture_cache()[cache_key] = tex;

        ExtractedTexture et;
        et.handle = tex;
        const auto& pbr = mat.pbr_metallic_roughness;
        if (pbr.base_color_factor[0] != 1.f ||
            pbr.base_color_factor[1] != 1.f ||
            pbr.base_color_factor[2] != 1.f ||
            pbr.base_color_factor[3] != 1.f) {
            et.base_color = glm::vec3(pbr.base_color_factor[0],
                                      pbr.base_color_factor[1],
                                      pbr.base_color_factor[2]);
        } else {
            et.base_color = glm::vec3(1.f);
        }
        result.push_back(std::move(et));
    }

    return result;
}

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

            // Extract texture from the selected primitive's material.
            if (best_prim->material) {
                const cgltf_material& mat = *best_prim->material;
                const cgltf_texture_view& tex_view = mat.pbr_metallic_roughness.base_color_texture;

                if (tex_view.texture && tex_view.texture->image) {
                    const cgltf_image* img = tex_view.texture->image;
                    if (img && img->buffer_view) {
                        const cgltf_buffer_view* bv = img->buffer_view;
                        const uint8_t* raw_bytes =
                            reinterpret_cast<const uint8_t*>(bv->buffer->data) + bv->offset;
                        cgltf_size byte_count = bv->size;

                        uint64_t cache_key =
                            reinterpret_cast<uint64_t>(bv->buffer->data) ^
                            (static_cast<uint64_t>(bv->offset) << 16);

                        auto cached = texture_cache().find(cache_key);
                        if (cached != texture_cache().end()) {
                            out.texture = cached->second;
                        } else {
                            int w = 0, h = 0, ch = 0;
                            uint8_t* pixels = stbi_load_from_memory(raw_bytes,
                                static_cast<int>(byte_count), &w, &h, &ch, 4);

                            if (pixels) {
                                RendererTextureHandle tex = renderer_upload_texture_from_memory(
                                    pixels, w, h, 4);
                                stbi_image_free(pixels);

                                if (renderer_handle_valid(tex)) {
                                    texture_cache()[cache_key] = tex;
                                    out.texture = tex;
                                }
                            } else {
                                fprintf(stderr, "[ENGINE] glTF %s: stbi_load_from_memory failed "
                                    "for best_prim material\n", relative_path);
                            }
                        }
                    }
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
