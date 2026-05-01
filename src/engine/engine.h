#pragma once

#include "components.h"
#include "physics.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

// =========================================================================
// Query result types
// =========================================================================

struct RaycastHit {
    entt::entity entity;
    glm::vec3    point;
    glm::vec3    normal;
    float        distance;
};

// =========================================================================
// Configuration
// =========================================================================

struct EngineConfig {
    float    physics_hz   = 120.0f;  // substep frequency
    float    dt_cap       = 0.1f;    // max accumulated dt per frame (100ms)
    uint32_t max_entities = 1024;    // advisory; no hard enforcement
};

// =========================================================================
// Lifecycle
// =========================================================================

void engine_init(const EngineConfig& config);
void engine_tick(float dt);
void engine_shutdown();

// =========================================================================
// Registry access — direct entt::registry& for game-layer view<> iteration
// =========================================================================

entt::registry& engine_registry();

// =========================================================================
// Scene API — Entity Lifecycle
// =========================================================================

entt::entity engine_create_entity();
void         engine_destroy_entity(entt::entity e);  // deferred to end-of-tick

// =========================================================================
// Scene API — Component Operations (header-only templates)
// =========================================================================

// decltype(auto) preserves the return type: T& for non-empty components,
// void for empty tag types (Static, Dynamic, Interactable, etc.).
template<typename T, typename... Args>
decltype(auto) engine_add_component(entt::entity e, Args&&... args) {
    return engine_registry().emplace<T>(e, std::forward<Args>(args)...);
}

template<typename T>
T& engine_get_component(entt::entity e) {
    return engine_registry().get<T>(e);
}

template<typename T>
T* engine_try_get_component(entt::entity e) {
    return engine_registry().try_get<T>(e);
}

template<typename T>
void engine_remove_component(entt::entity e) {
    engine_registry().remove<T>(e);
}

template<typename T>
bool engine_has_component(entt::entity e) {
    return engine_registry().all_of<T>(e);
}

// =========================================================================
// Procedural Shape Spawners
// =========================================================================

entt::entity engine_spawn_sphere(
    const glm::vec3& position,
    float            radius,
    const Material&  material
);

entt::entity engine_spawn_cube(
    const glm::vec3& position,
    float            half_extent,
    const Material&  material
);

// =========================================================================
// Asset Loading
// =========================================================================

RendererMeshHandle engine_load_gltf(const char* relative_path,
                                     RendererTextureHandle* out_texture = nullptr);
RendererMeshHandle engine_load_obj(const char* relative_path);

entt::entity engine_spawn_from_asset(
    const char*      relative_path,
    const glm::vec3& position,
    const glm::quat& rotation,
    const Material&  material
);

// =========================================================================
// Time
// =========================================================================

double engine_now();        // seconds since engine_init()
float  engine_delta_time(); // last frame dt in seconds

// =========================================================================
// Input (polled state)
// =========================================================================

bool      engine_key_down(int keycode);     // sapp_keycode values
glm::vec2 engine_mouse_delta();
bool      engine_mouse_button(int button);  // 0=left, 1=right, 2=middle
glm::vec2 engine_mouse_position();

// =========================================================================
// Physics — Force / Impulse API
// =========================================================================

void engine_apply_force(entt::entity e, const glm::vec3& force);
void engine_apply_impulse(entt::entity e, const glm::vec3& impulse);
void engine_apply_impulse_at_point(
    entt::entity     e,
    const glm::vec3& impulse,
    const glm::vec3& world_point
);

// =========================================================================
// Physics — Inertia Tensor Helpers (for game-layer RigidBody setup)
// =========================================================================

glm::mat3 make_box_inv_inertia_body(float mass, glm::vec3 half_extents);
glm::mat3 make_sphere_inv_inertia_body(float mass, float radius);
void update_world_inertia(RigidBody& rb, const glm::quat& rotation,
                          const glm::mat3& inv_inertia_body);

// =========================================================================
// Physics Queries
// =========================================================================

std::optional<RaycastHit> engine_raycast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float            max_distance
);

std::vector<entt::entity> engine_overlap_aabb(
    const glm::vec3& center,
    const glm::vec3& half_extents
);

// =========================================================================
// Camera
// =========================================================================

void engine_set_active_camera(entt::entity e);

// =========================================================================
// Collision Flash — demo-scene visual feedback (DEMO-SCENE)
// =========================================================================

void engine_on_collision(entt::entity e);
