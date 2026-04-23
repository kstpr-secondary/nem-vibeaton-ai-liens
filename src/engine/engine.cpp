#include "engine.h"
#include "engine_time.h"
#include "scene_api.h"
#include "camera.h"
#include "math_utils.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

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

    // --- Phase 5: input delta update goes here ---
    // --- Phase 6: physics substep loop goes here ---

    // Camera: view + projection → renderer
    camera_update();

    // Directional light upload (first Light entity wins)
    for (auto e : g_registry.view<Light>()) {
        auto& l = g_registry.get<Light>(e);
        glm::vec3 dir = (glm::length(l.direction) > 0.f)
                        ? glm::normalize(l.direction)
                        : glm::vec3(0.f, -1.f, 0.f);
        DirectionalLight dl{};
        dl.direction[0] = dir.x; dl.direction[1] = dir.y; dl.direction[2] = dir.z;
        dl.color[0]     = l.color.x; dl.color[1] = l.color.y; dl.color[2] = l.color.z;
        dl.intensity    = l.intensity;
        renderer_set_directional_light(dl);
        break;
    }

    // Render enqueue: every entity with Transform + Mesh + EntityMaterial
    for (auto&& [e, t, mesh, mat] :
         g_registry.view<Transform, Mesh, EntityMaterial>().each()) {
        (void)e;
        if (!renderer_handle_valid(mesh.handle)) continue;
        glm::mat4 model = make_model_matrix(t);
        renderer_enqueue_draw(mesh.handle, glm::value_ptr(model), mat.mat);
    }

    // Deferred entity destruction sweep
    scene_flush_pending_destroys();
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
// Camera control
// ---------------------------------------------------------------------------

void engine_set_active_camera(entt::entity e) {
    if (!g_registry.valid(e)) {
        fprintf(stderr, "[ENGINE] set_active_camera: invalid entity\n");
        return;
    }
    if (!g_registry.all_of<Camera>(e)) {
        fprintf(stderr, "[ENGINE] set_active_camera: entity has no Camera component\n");
        return;
    }
    for (auto prev : g_registry.view<CameraActive>())
        g_registry.remove<CameraActive>(prev);
    g_registry.emplace<CameraActive>(e);
}

// ---------------------------------------------------------------------------
// Stubs for subsystems implemented in later phases
// ---------------------------------------------------------------------------

// Phase 4 — asset import
RendererMeshHandle engine_load_gltf(const char*)  { return {}; }
RendererMeshHandle engine_load_obj(const char*)   { return {}; }
entt::entity engine_spawn_from_asset(const char*, const glm::vec3&, const glm::quat&, const Material&) {
    return entt::null;
}

// Phase 5 — input
bool      engine_key_down(int)     { return false; }
glm::vec2 engine_mouse_delta()     { return {0.f, 0.f}; }
bool      engine_mouse_button(int) { return false; }
glm::vec2 engine_mouse_position()  { return {0.f, 0.f}; }

// Phase 6 — physics force API
void engine_apply_force(entt::entity, const glm::vec3&) {}
void engine_apply_impulse(entt::entity, const glm::vec3&) {}
void engine_apply_impulse_at_point(entt::entity, const glm::vec3&, const glm::vec3&) {}

// Phase 5 — spatial queries
std::optional<RaycastHit> engine_raycast(const glm::vec3&, const glm::vec3&, float) {
    return std::nullopt;
}
std::vector<entt::entity> engine_overlap_aabb(const glm::vec3&, const glm::vec3&) {
    return {};
}
