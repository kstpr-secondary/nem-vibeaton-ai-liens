// Engine mock stubs — compiled when USE_ENGINE_MOCKS=ON.
// All void functions are no-ops; returning functions return safe sentinels.
// Template component operations work via the static mock registry.

#include "engine.h"

#include <optional>
#include <vector>

// ---------------------------------------------------------------------------
// Static mock state
// ---------------------------------------------------------------------------

static entt::registry g_mock_registry;

// Pre-created sentinel entity so callers that store returned handles get a
// valid (but otherwise inert) entity.
static entt::entity ensure_sentinel() {
    static entt::entity s = entt::null;
    if (s == entt::null || !g_mock_registry.valid(s)) {
        s = g_mock_registry.create();
    }
    return s;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void engine_init(const EngineConfig&) {}
void engine_tick(float) {}
void engine_shutdown() {}

// ---------------------------------------------------------------------------
// Registry — returns the mock registry so template component ops compile
// ---------------------------------------------------------------------------

entt::registry& engine_registry() { return g_mock_registry; }

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

entt::entity engine_create_entity()           { return g_mock_registry.create(); }
void         engine_destroy_entity(entt::entity) {}

// ---------------------------------------------------------------------------
// Procedural spawners
// ---------------------------------------------------------------------------

entt::entity engine_spawn_sphere(const glm::vec3&, float, const Material&)  { return ensure_sentinel(); }
entt::entity engine_spawn_cube(const glm::vec3&, float, const Material&)    { return ensure_sentinel(); }

// ---------------------------------------------------------------------------
// Asset loading
// ---------------------------------------------------------------------------

RendererMeshHandle engine_load_gltf(const char*, RendererTextureHandle*) { return {1}; }
RendererMeshHandle engine_load_obj(const char*)  { return {1}; }

entt::entity engine_spawn_from_asset(const char*, const glm::vec3&, const glm::quat&, const Material&) {
    return ensure_sentinel();
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

double    engine_now()        { return 0.0; }
float     engine_delta_time() { return 0.f; }

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool      engine_key_down(int)     { return false; }
glm::vec2 engine_mouse_delta()     { return {0.f, 0.f}; }
bool      engine_mouse_button(int) { return false; }
glm::vec2 engine_mouse_position()  { return {0.f, 0.f}; }

// ---------------------------------------------------------------------------
// Physics
// ---------------------------------------------------------------------------

void engine_apply_force(entt::entity, const glm::vec3&) {}
void engine_apply_impulse(entt::entity, const glm::vec3&) {}
void engine_apply_impulse_at_point(entt::entity, const glm::vec3&, const glm::vec3&) {}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

std::optional<RaycastHit> engine_raycast(const glm::vec3&, const glm::vec3&, float) {
    return std::nullopt;
}

std::vector<entt::entity> engine_overlap_aabb(const glm::vec3&, const glm::vec3&) {
    return {};
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

void engine_set_active_camera(entt::entity) {}
