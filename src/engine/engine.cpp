#include "engine.h"
#include "engine_time.h"
#include "input.h"
#include "scene_api.h"
#include "camera.h"
#include "math_utils.h"
#include "physics.h"

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

    // Phase 6: physics substep loop (fixed-timestep, before rendering)
    physics_system_tick(g_registry, dt, g_config.physics_hz, g_config.dt_cap);

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
// Physics — Force / Impulse API
// ---------------------------------------------------------------------------

void engine_apply_force(entt::entity e, const glm::vec3& force) {
    if (!g_registry.valid(e)) {
        fprintf(stderr, "[ENGINE] apply_force: invalid entity\n");
        return;
    }
    if (!g_registry.all_of<ForceAccum>(e)) {
        fprintf(stderr, "[ENGINE] apply_force: entity has no ForceAccum component\n");
        return;
    }
    auto& fa = g_registry.get<ForceAccum>(e);
    fa.force += force;
}

void engine_apply_impulse(entt::entity e, const glm::vec3& impulse) {
    if (!g_registry.valid(e)) {
        fprintf(stderr, "[ENGINE] apply_impulse: invalid entity\n");
        return;
    }
    if (!g_registry.all_of<RigidBody>(e)) {
        fprintf(stderr, "[ENGINE] apply_impulse: entity has no RigidBody component\n");
        return;
    }
    auto& rb = g_registry.get<RigidBody>(e);
    rb.linear_velocity += impulse * rb.inv_mass;
}

void engine_apply_impulse_at_point(
    entt::entity     e,
    const glm::vec3& impulse,
    const glm::vec3& world_point
) {
    if (!g_registry.valid(e)) {
        fprintf(stderr, "[ENGINE] apply_impulse_at_point: invalid entity\n");
        return;
    }
    if (!g_registry.all_of<Transform, RigidBody>(e)) {
        fprintf(stderr, "[ENGINE] apply_impulse_at_point: entity missing Transform or RigidBody\n");
        return;
    }
    auto& tr = g_registry.get<Transform>(e);
    auto& rb = g_registry.get<RigidBody>(e);

    // Apply linear impulse
    rb.linear_velocity += impulse * rb.inv_mass;

    // Apply angular impulse: torque = r × impulse, where r = world_point - COM
    glm::vec3 r = world_point - tr.position;
    rb.angular_velocity += rb.inv_inertia * glm::cross(r, impulse);
}

// ---------------------------------------------------------------------------
// Collision Flash — demo-scene visual feedback
// ---------------------------------------------------------------------------

void engine_on_collision(entt::entity e) {
    if (!g_registry.valid(e)) return;
    if (!g_registry.all_of<EntityMaterial>(e)) return;

    // Don't overwrite existing flash — only set on first collision per frame
    if (g_registry.all_of<CollisionFlash>(e)) return;

    auto& em = g_registry.get<EntityMaterial>(e);
    auto& cf = g_registry.emplace<CollisionFlash>(e);
    cf.timer = 1.0f;
    cf.base_color = glm::vec3(em.mat.base_color[0], em.mat.base_color[1], em.mat.base_color[2]);
}
