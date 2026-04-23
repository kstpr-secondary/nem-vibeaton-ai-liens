#include "engine.h"
#include "engine_time.h"

#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// Engine-global state
// ---------------------------------------------------------------------------

static entt::registry g_registry;
static EngineConfig   g_config;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void engine_init(const EngineConfig& config) {
    g_config   = config;
    g_registry = entt::registry{};
    time_init();
}

void engine_tick(float dt) {
    if (dt <= 0.f) {
        fprintf(stderr, "[ENGINE] dt <= 0 (%.6f); clamping to 0.0001s\n", static_cast<double>(dt));
        dt = 0.0001f;
    }
    if (dt > g_config.dt_cap) {
        dt = g_config.dt_cap;
    }
    time_set_delta(dt);

    // --- Phase 3: input delta update wired here ---
    // --- Phase 3: physics substep loop wired here ---
    // --- Phase 3: camera update wired here ---
    // --- Phase 3: render enqueue loop wired here ---

    // Deferred entity destruction sweep (must run last)
    std::vector<entt::entity> to_destroy;
    for (auto e : g_registry.view<DestroyPending>()) {
        to_destroy.push_back(e);
    }
    for (auto e : to_destroy) {
        g_registry.destroy(e);
    }
}

void engine_shutdown() {
    g_registry.clear();
}

// ---------------------------------------------------------------------------
// Registry access
// ---------------------------------------------------------------------------

entt::registry& engine_registry() {
    return g_registry;
}

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

entt::entity engine_create_entity() {
    return g_registry.create();
}

void engine_destroy_entity(entt::entity e) {
    if (!g_registry.valid(e)) return;
    g_registry.emplace_or_replace<DestroyPending>(e);
}

// ---------------------------------------------------------------------------
// Stubs for subsystems implemented in later phases
// (engine.h declarations must be satisfied for the lib to link)
// ---------------------------------------------------------------------------

entt::entity engine_spawn_sphere(const glm::vec3&, float, const Material&) { return entt::null; }
entt::entity engine_spawn_cube(const glm::vec3&, float, const Material&)   { return entt::null; }

RendererMeshHandle engine_load_gltf(const char*) { return {}; }
RendererMeshHandle engine_load_obj(const char*)  { return {}; }

entt::entity engine_spawn_from_asset(const char*, const glm::vec3&, const glm::quat&, const Material&) {
    return entt::null;
}

bool      engine_key_down(int)     { return false; }
glm::vec2 engine_mouse_delta()     { return {0.f, 0.f}; }
bool      engine_mouse_button(int) { return false; }
glm::vec2 engine_mouse_position()  { return {0.f, 0.f}; }

void engine_apply_force(entt::entity, const glm::vec3&) {}
void engine_apply_impulse(entt::entity, const glm::vec3&) {}
void engine_apply_impulse_at_point(entt::entity, const glm::vec3&, const glm::vec3&) {}

std::optional<RaycastHit> engine_raycast(const glm::vec3&, const glm::vec3&, float) {
    return std::nullopt;
}

std::vector<entt::entity> engine_overlap_aabb(const glm::vec3&, const glm::vec3&) {
    return {};
}

void engine_set_active_camera(entt::entity e) {
    if (!g_registry.valid(e)) {
        fprintf(stderr, "[ENGINE] set_active_camera: invalid entity\n");
        return;
    }
    if (!g_registry.all_of<Camera>(e)) {
        fprintf(stderr, "[ENGINE] set_active_camera: entity has no Camera component\n");
        return;
    }
    // Remove tag from previous holder, then set on new entity
    for (auto prev : g_registry.view<CameraActive>()) {
        g_registry.remove<CameraActive>(prev);
    }
    g_registry.emplace<CameraActive>(e);
}
