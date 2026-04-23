#include "engine.h"
#include "scene_api.h"

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
    for (auto e : to_destroy)
        reg.destroy(e);
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
    reg.emplace<Mesh>(e, Mesh{renderer_make_sphere_mesh(radius, 16)});
    reg.emplace<EntityMaterial>(e, EntityMaterial{material});

    return e;
}

entt::entity engine_spawn_cube(const glm::vec3& position, float half_extent, const Material& material) {
    auto& reg = engine_registry();
    auto e = reg.create();

    Transform t{};
    t.position = position;
    reg.emplace<Transform>(e, t);
    reg.emplace<Mesh>(e, Mesh{renderer_make_cube_mesh(half_extent)});
    reg.emplace<EntityMaterial>(e, EntityMaterial{material});

    return e;
}
