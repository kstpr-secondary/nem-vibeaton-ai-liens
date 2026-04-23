# Engine Public API Contract

> **Status**: DRAFT — produced by SpecKit plan phase.  
> **Promotion path**: This file becomes authoritative at `docs/interfaces/engine-interface-spec.md` after human approval and freeze marker addition.  
> **Freeze condition**: Must be frozen (version marker added, status changed to FROZEN) before the game workstream begins its SpecKit planning cycle.  
> **Upstream dependency**: Renderer interface spec (`renderer-interface-spec.md`, FROZEN v1.0).

---

## Version

`v0.1-draft` — pre-freeze. Breaking changes allowed until freeze.

---

## Overview

The engine exposes a single C++ header `engine.h` (under `src/engine/`) with all public types, component definitions, and free-function declarations. The game executable and `engine_app` driver include only this header. Internal implementation files (`scene_api.h`, `physics.h`, `input.h`, etc.) are never included by consumers.

The engine consumes the renderer public API (`renderer.h`, FROZEN v1.0) and is consumed by the game executable.

---

## Public Header Shape (`src/engine/engine.h`)

```cpp
#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <renderer.h>
#include <cstdint>
#include <optional>
#include <span>

// =========================================================================
// Components (engine-owned; schema changes after freeze need human approval)
// =========================================================================

struct Transform {
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::vec3 scale    = {1.f, 1.f, 1.f};
};

struct Mesh {
    RendererMeshHandle handle = {};
};

struct EntityMaterial {
    Material mat = {};
};

struct RigidBody {
    float     mass             = 1.0f;
    float     inv_mass         = 1.0f;
    glm::vec3 linear_velocity  = {0.f, 0.f, 0.f};
    glm::vec3 angular_velocity = {0.f, 0.f, 0.f};
    glm::mat3 inv_inertia_body = glm::mat3(1.f);
    glm::mat3 inv_inertia      = glm::mat3(1.f);
    float     restitution      = 1.0f;
};

struct Collider {
    glm::vec3 half_extents = {0.5f, 0.5f, 0.5f};
};

struct Camera {
    float fov        = 60.0f;
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;
};

struct Light {
    glm::vec3 direction = {0.f, -1.f, 0.f};
    glm::vec3 color     = {1.f, 1.f, 1.f};
    float     intensity = 1.0f;
};

// Tag components (empty structs — presence is semantic)
struct Static {};
struct Dynamic {};
struct Interactable {};
struct CameraActive {};
struct DestroyPending {};

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
    float physics_hz   = 120.0f;
    float dt_cap       = 0.1f;
    uint32_t max_entities = 1024;
};

// =========================================================================
// Lifecycle
// =========================================================================

void engine_init(const EngineConfig& config);
void engine_tick(float dt);
void engine_shutdown();

// =========================================================================
// Scene API — Entity Lifecycle
// =========================================================================

entt::entity engine_create_entity();
void         engine_destroy_entity(entt::entity e);  // deferred to end-of-tick

// =========================================================================
// Scene API — Component Operations
// =========================================================================

// Template functions — game includes engine.h and calls directly:
//   engine_add_component<T>(e, args...)
//   engine_get_component<T>(e)
//   engine_remove_component<T>(e)
//   engine_has_component<T>(e)
//
// Implementation detail: these forward to the engine's entt::registry.

template<typename T, typename... Args>
T& engine_add_component(entt::entity e, Args&&... args);

template<typename T>
T& engine_get_component(entt::entity e);

template<typename T>
T* engine_try_get_component(entt::entity e);  // nullptr if absent

template<typename T>
void engine_remove_component(entt::entity e);

template<typename T>
bool engine_has_component(entt::entity e);

// =========================================================================
// Scene API — ECS View Access (game iterates directly)
// =========================================================================

entt::registry& engine_registry();  // direct access for view<>() iteration

// =========================================================================
// Procedural Shape Spawners
// =========================================================================

entt::entity engine_spawn_sphere(
    const glm::vec3& position,
    float radius,
    const Material& material
);

entt::entity engine_spawn_cube(
    const glm::vec3& position,
    float half_extent,
    const Material& material
);

// =========================================================================
// Asset Loading
// =========================================================================

RendererMeshHandle engine_load_gltf(const char* relative_path);
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

double engine_now();         // seconds since init, from sokol_time
float  engine_delta_time();  // last frame dt in seconds

// =========================================================================
// Input (polled state)
// =========================================================================

bool      engine_key_down(int keycode);       // sapp_keycode values
glm::vec2 engine_mouse_delta();
bool      engine_mouse_button(int button);    // 0=left, 1=right, 2=middle
glm::vec2 engine_mouse_position();

// =========================================================================
// Physics — Force/Impulse API
// =========================================================================

void engine_apply_force(entt::entity e, const glm::vec3& force);
void engine_apply_impulse(entt::entity e, const glm::vec3& impulse);
void engine_apply_impulse_at_point(
    entt::entity e,
    const glm::vec3& impulse,
    const glm::vec3& world_point
);

// =========================================================================
// Physics Queries
// =========================================================================

std::optional<RaycastHit> engine_raycast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float max_distance
);

std::vector<entt::entity> engine_overlap_aabb(
    const glm::vec3& center,
    const glm::vec3& half_extents
);

// =========================================================================
// Camera
// =========================================================================

void engine_set_active_camera(entt::entity e);
```

---

## Calling Convention

1. **Initialization**: `renderer_init(config)` → `engine_init(engine_config)` → renderer `set_input_callback` for engine's input handler → `renderer_run()` (blocking).
2. **Per-frame tick** (inside renderer frame callback): `renderer_begin_frame()` → `engine_tick(dt)` (which internally calls `renderer_set_camera`, `renderer_set_directional_light`, `renderer_enqueue_draw` × N) → `renderer_end_frame()`.
3. All engine API calls must be on the main thread (inherited from sokol_app requirement).
4. `engine_tick(dt)` must be called between `renderer_begin_frame()` and `renderer_end_frame()`.
5. Template functions (`engine_add_component`, etc.) are header-only; the game includes `engine.h` and the linker resolves through the engine static lib.

---

## Error Behavior

| Situation | Behavior |
|-----------|----------|
| `destroy_entity(entt::null)` | Silently ignored |
| `get_component<T>` on entity without T | UB — use `try_get_component` when uncertain |
| `engine_load_gltf` with invalid path | Returns `{0}` (invalid handle); logs `[ENGINE] Failed to load: <path>` |
| `engine_load_obj` with invalid path | Returns `{0}` (invalid handle); logs `[ENGINE] Failed to load: <path>` |
| `apply_force` on entity without RigidBody | Silently ignored |
| `raycast` hits nothing | Returns `std::nullopt` |
| `overlap_aabb` finds nothing | Returns empty vector |
| `set_active_camera` on entity without Camera | Logs warning; camera unchanged |
| Multiple `CameraActive` tags | `set_active_camera` removes old tag first; always exactly one |
| dt is zero or negative | Clamped to small positive minimum; warning logged |

---

## Mock Surface

`src/engine/mocks/engine_mock.cpp` — activated by `USE_ENGINE_MOCKS=ON`. All void functions are no-ops. Entity-returning functions return a valid sentinel entity. Handle-returning functions return `{1}`. Query functions return empty results. Template functions compile against a static mock registry.

---

## Intended Freeze Inputs

- `pre_planning_docs/Hackathon Master Blueprint.md`
- `pre_planning_docs/Game Engine Concept and Milestones.md`
- `docs/interfaces/renderer-interface-spec.md` (FROZEN v1.0)
- `docs/architecture/engine-architecture.md`
- `specs/002-ecs-physics-engine/plan.md`
- `specs/002-ecs-physics-engine/data-model.md`
- `.agents/skills/engine-specialist/SKILL.md`
- `.agents/skills/entt-ecs/SKILL.md`
- `.agents/skills/cgltf-loading/SKILL.md`
- `.agents/skills/physics-euler/SKILL.md`
