#include "engine.h"
#include "scene_api.h"
#include "asset_import.h"
#include "obj_import.h"
#include "asset_bridge.h"
#include "mesh_builders.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Entity lifecycle (T008)
// ---------------------------------------------------------------------------

entt::entity engine_create_entity() {
    return engine_registry().create();
}

void engine_destroy_entity(entt::entity e) {
    if (!engine_registry().valid(e)) return;
    engine_registry().emplace_or_replace<DestroyPending>(e);
}

void scene_flush_pending_destroys() {
    auto& reg = engine_registry();
    std::vector<entt::entity> to_destroy;
    for (auto e : reg.view<DestroyPending>())
        to_destroy.push_back(e);
    for (auto e : to_destroy) {
        if (reg.valid(e) && reg.all_of<Mesh>(e)) {
            mesh_store_ref_dec(reg.get<Mesh>(e).handle.id);
        }
        reg.destroy(e);
    }
}

// ---------------------------------------------------------------------------
// Procedural shape spawners (T009)
// ---------------------------------------------------------------------------

entt::entity engine_spawn_sphere(const glm::vec3& position, float radius, const Material& material) {
    auto& reg = engine_registry();
    auto e = reg.create();

    Transform t{};
    t.position = position;
    reg.emplace<Transform>(e, t);
    Mesh m{renderer_make_sphere_mesh(radius, 16)};
    mesh_store_ref_inc(m.handle.id);
    reg.emplace<Mesh>(e, m);
    reg.emplace<EntityMaterial>(e, EntityMaterial{material});

    return e;
}

entt::entity engine_spawn_cube(const glm::vec3& position, float half_extent, const Material& material) {
    auto& reg = engine_registry();
    auto e = reg.create();

    Transform t{};
    t.position = position;
    reg.emplace<Transform>(e, t);
    Mesh m{renderer_make_cube_mesh(half_extent)};
    mesh_store_ref_inc(m.handle.id);
    reg.emplace<Mesh>(e, m);
    reg.emplace<EntityMaterial>(e, EntityMaterial{material});

    return e;
}

// ---------------------------------------------------------------------------
// Asset loading (T016) — path-cached to avoid duplicate uploads
// ---------------------------------------------------------------------------

static std::unordered_map<std::string, RendererMeshHandle> s_gltf_cache;
static std::unordered_map<std::string, RendererMeshHandle> s_obj_cache;

RendererMeshHandle engine_load_gltf(const char* relative_path) {
    auto it = s_gltf_cache.find(relative_path);
    if (it != s_gltf_cache.end())
        return it->second;

    ImportedMesh mesh = asset_import_gltf(relative_path);
    if (mesh.empty()) {
        fprintf(stderr, "[ENGINE] engine_load_gltf: failed to load %s\n", relative_path);
        return {};
    }
    RendererMeshHandle handle = asset_bridge_upload(mesh);
    s_gltf_cache[relative_path] = handle;
    return handle;
}

RendererMeshHandle engine_load_obj(const char* relative_path) {
    auto it = s_obj_cache.find(relative_path);
    if (it != s_obj_cache.end())
        return it->second;

    ImportedMesh mesh = asset_import_obj(relative_path);
    if (mesh.empty()) {
        fprintf(stderr, "[ENGINE] engine_load_obj: failed to load %s\n", relative_path);
        return {};
    }
    RendererMeshHandle handle = asset_bridge_upload(mesh);
    s_obj_cache[relative_path] = handle;
    return handle;
}

entt::entity engine_spawn_from_asset(const char*      relative_path,
                                     const glm::vec3& position,
                                     const glm::quat& rotation,
                                     const Material&  material) {
    // Detect format by extension
    std::string path(relative_path);
    RendererMeshHandle handle{};

    auto ends_with = [&](const char* suffix) {
        size_t sl = path.size(), el = strlen(suffix);
        return sl >= el && path.compare(sl - el, el, suffix) == 0;
    };

    if (ends_with(".glb") || ends_with(".gltf")) {
        handle = engine_load_gltf(relative_path);
    } else if (ends_with(".obj")) {
        handle = engine_load_obj(relative_path);
    } else {
        fprintf(stderr, "[ENGINE] spawn_from_asset: unknown format for %s\n", relative_path);
        return entt::null;
    }

    if (!renderer_handle_valid(handle)) {
        fprintf(stderr, "[ENGINE] spawn_from_asset: mesh load failed for %s\n", relative_path);
        return entt::null;
    }

    auto& reg = engine_registry();
    auto e = reg.create();

    Transform t{};
    t.position = position;
    t.rotation = rotation;
    reg.emplace<Transform>(e, t);
    mesh_store_ref_inc(handle.id);
    reg.emplace<Mesh>(e, Mesh{handle});
    reg.emplace<EntityMaterial>(e, EntityMaterial{material});

    return e;
}
