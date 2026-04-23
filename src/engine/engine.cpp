#include "engine.h"
#include "engine_time.h"
#include "input.h"
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
    input_init();  // registers the input callback with the renderer (T022)
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

    input_begin_frame();  // reset per-frame mouse delta (T022)

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
// Stubs for Phase 6 physics force API
// (engine_key_down, mouse_delta, mouse_button, mouse_position → input.cpp)
// (engine_raycast, engine_overlap_aabb → raycast.cpp)
// ---------------------------------------------------------------------------

void engine_apply_force(entt::entity, const glm::vec3&) {}
void engine_apply_impulse(entt::entity, const glm::vec3&) {}
void engine_apply_impulse_at_point(entt::entity, const glm::vec3&, const glm::vec3&) {}
