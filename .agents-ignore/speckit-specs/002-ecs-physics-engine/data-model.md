# Engine Data Model

> **Phase 1 output** for `002-ecs-physics-engine`. ECS component schema, entity relationships, state transitions, and validation rules.

---

## Entity

A lightweight `entt::entity` identifier (opaque `uint32_t`). No behavior on its own — behavior is determined by attached components. `entt::null` is the invalid sentinel. Entity validity checked via `registry.valid(e)`.

**Validation rules**:
- Entity handles may be reused after `destroy()`. Never cache handles across ticks without re-validating with `registry.valid(e)`.
- `destroy_entity()` defers destruction to end-of-tick via `DestroyPending` tag.

---

## Transform

Spatial placement of every scene entity. Required on all renderable, physical, and camera entities.

```cpp
struct Transform {
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f); // identity
    glm::vec3 scale    = {1.f, 1.f, 1.f};
};
```

**Derived value**: Model matrix = `T * R * S` (translate × rotate × scale), column-major per GLM convention.

**Validation rules**:
- Rotation quaternion must remain normalized. Physics integration renormalizes after each substep.
- Scale is not used by the collision system at MVP (AABB half-extents are local-space, unscaled).

---

## Mesh

References renderer-owned geometry. Opaque handle — engine never inspects internals.

```cpp
struct Mesh {
    RendererMeshHandle handle = {};  // {0} = invalid/no mesh
};
```

**Validation rules**:
- `handle.id == 0` means the entity has no renderable geometry. Draw enqueue skips it silently.
- Handle lifetime: valid from `renderer_upload_mesh()` / `renderer_make_*_mesh()` until `renderer_shutdown()`.

---

## EntityMaterial

Engine-side material wrapper storing the renderer's `Material` value type.

```cpp
struct EntityMaterial {
    Material mat = {};  // renderer Material struct — ShadingModel, base_color, texture, shininess, alpha
};
```

**Naming note**: Named `EntityMaterial` to avoid collision with the renderer's `Material` type which is stored directly inside this component.

**Validation rules**:
- `mat.alpha < 1.0` routes draw to transparent pass automatically (renderer behavior).
- Invalid texture handle falls back to `base_color` (renderer behavior).

---

## RigidBody

Physical properties for dynamic entities under physics simulation.

```cpp
struct RigidBody {
    float     mass             = 1.0f;
    float     inv_mass         = 1.0f;   // 1/mass; precomputed at spawn
    glm::vec3 linear_velocity  = {0.f, 0.f, 0.f};
    glm::vec3 angular_velocity = {0.f, 0.f, 0.f};  // world-space ω, radians/s
    glm::mat3 inv_inertia_body = glm::mat3(1.f);    // body-space I⁻¹; set at spawn, never changes
    glm::mat3 inv_inertia      = glm::mat3(1.f);    // world-space I⁻¹; recomputed each substep
    float     restitution      = 1.0f;               // 1.0 = perfectly elastic
};
```

**State transitions**:
- Created at entity spawn with `mass`, `inv_mass = 1/mass`, `inv_inertia_body` computed from AABB half-extents.
- `linear_velocity` and `angular_velocity` updated each substep by integration + impulse response.
- `inv_inertia` (world-space) recomputed from `inv_inertia_body` and current rotation each substep.

**Validation rules**:
- `mass` must be > 0 for dynamic bodies. Zero mass is illegal — use `Static` tag instead.
- `inv_mass = 0` on Static-tagged entities (impulse formula automatically produces zero velocity change).
- `restitution` should be in `[0, 1]`; 1.0 = perfectly elastic (energy-conserving), 0.0 = perfectly inelastic.

---

## Collider

Axis-aligned bounding box in local space for collision detection and spatial queries.

```cpp
struct Collider {
    glm::vec3 half_extents = {0.5f, 0.5f, 0.5f};  // local-space AABB half-sizes
};
```

**Derived value**: World AABB = `Transform.position ± half_extents`. Rotation and scale are **ignored** at MVP.

**Validation rules**:
- All three half-extents must be > 0.
- World AABB recomputed each physics substep from the entity's current position.

---

## Camera

Viewing parameters for projection matrix construction. Combined with Transform for the view matrix.

```cpp
struct Camera {
    float fov  = 60.0f;   // vertical field of view in degrees
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;
};
```

**Derived values**:
- View matrix = `glm::inverse(make_model_matrix(camera_transform))`.
- Projection = `glm::perspective(radians(fov), aspect_ratio, near_plane, far_plane)`.
- Aspect ratio from `sapp_widthf() / sapp_heightf()`.

**Validation rules**:
- `near_plane` must be > 0 and < `far_plane`.
- `fov` must be in `(0, 180)` exclusive.
- Exactly one entity with `CameraActive` tag per scene.

---

## Light

Illumination source. MVP: exactly one directional light. Point lights = E-M6 (Desirable).

```cpp
struct Light {
    glm::vec3 direction = {0.f, -1.f, 0.f};  // world-space, toward light source
    glm::vec3 color     = {1.f, 1.f, 1.f};   // linear RGB
    float     intensity = 1.0f;
};
```

**Validation rules**:
- Direction is re-normalized before upload to renderer.
- Zero vector falls back to `{0, -1, 0}` (straight down).

---

## Tag Components (Empty Structs)

Tags carry no data — presence on an entity is the semantic.

```cpp
struct Static {};         // Excluded from physics integration; still collidable
struct Dynamic {};        // Under physics simulation (has RigidBody + Collider)
struct Interactable {};   // Visible to raycast() and overlap_aabb() queries
struct CameraActive {};   // Marks the one active camera entity per scene
struct DestroyPending {}; // Queued for end-of-tick registry.destroy() sweep
```

**Validation rules**:
- `Static` entities have `inv_mass = 0` and `inv_inertia = mat3(0)` on their RigidBody (if they have one for collision participation).
- Exactly one `CameraActive` entity at any time. `set_active_camera(e)` removes the tag from the previous holder and emplaces on the new entity.
- `DestroyPending` is engine-internal; game code calls `destroy_entity()` which emplaces this tag.

---

## ForceAccumulator (Runtime-Only)

Not an ECS component — a transient per-step data structure.

```cpp
struct ForceAccum {
    glm::vec3 force  = {0.f, 0.f, 0.f};
    glm::vec3 torque = {0.f, 0.f, 0.f};
};

// Stored in: std::unordered_map<entt::entity, ForceAccum> force_accumulators;
// Cleared after each physics substep.
```

---

## InputState (Runtime-Only)

Not an ECS component — a singleton managed by the input system.

```cpp
struct InputState {
    bool  key_states[512]   = {};  // indexed by sapp_keycode
    float mouse_x           = 0.f;
    float mouse_y           = 0.f;
    float mouse_dx          = 0.f; // delta since last frame
    float mouse_dy          = 0.f;
    bool  mouse_buttons[3]  = {};  // left, right, middle
};
```

---

## RaycastHit

Return type for `raycast()` query.

```cpp
struct RaycastHit {
    entt::entity entity;
    glm::vec3    point;    // world-space hit point
    glm::vec3    normal;   // surface normal at hit point
    float        distance; // distance from ray origin to hit point
};
```

---

## EngineConfig

Passed to `engine_init()`.

```cpp
struct EngineConfig {
    float physics_hz         = 120.0f;  // substep frequency
    float dt_cap             = 0.1f;    // max accumulated dt per frame
    uint32_t max_entities    = 1024;    // advisory; no hard enforcement
};
```

---

## Entity Composition Patterns

| Pattern | Components | Tag(s) | Description |
|---------|-----------|--------|-------------|
| Static obstacle | Transform, Collider, Mesh, EntityMaterial | Static | Visible, collidable, unmovable |
| Dynamic rigid body | Transform, Collider, RigidBody, Mesh, EntityMaterial | Dynamic, Interactable | Physics-driven, collidable, queryable |
| Camera entity | Transform, Camera | CameraActive | View/projection source |
| Light entity | Transform, Light | — | Directional illumination source |
| Procedural primitive | Transform, Mesh, EntityMaterial | varies | Spawned via `spawn_sphere`/`spawn_cube` |
| Loaded asset | Transform, Mesh, EntityMaterial | varies | Spawned via `spawn_from_asset` |

---

## Relationships

```
EngineConfig ──(1)──► engine_init()
                            │
                            ▼
                    entt::registry (owned by engine)
                            │
            ┌───────────────┼───────────────────┐
            ▼               ▼                   ▼
        Entities        Components          Systems
            │               │                   │
    create/destroy    emplace/get/remove    per-tick logic
            │               │                   │
            ▼               ▼                   ▼
    DestroyPending    Transform, Mesh,    physics_step()
    end-of-tick       RigidBody, ...      collision_detect()
    sweep                                 collision_respond()
                                          camera_update()
                                          render_enqueue()
```

---

## State Transitions

```
[engine_init(config)]
  → entt::registry created
  → input callback registered with renderer
  → time initialized via sokol_time
  → physics accumulator = 0

[engine_tick(dt)]
  → input state updated (mouse delta computed)
  → physics substep loop:
      → force accumulation
      → linear + angular integration
      → world-space inertia update
      → AABB collision detection (brute-force pairs)
      → impulse-based collision response
      → Baumgarte positional correction
      → force accumulators cleared
  → camera view/projection computed → renderer_set_camera()
  → light entity → renderer_set_directional_light()
  → iterate view<Transform, Mesh, EntityMaterial> → renderer_enqueue_draw() per entity
  → DestroyPending sweep → registry.destroy()

[engine_shutdown()]
  → registry.clear()  (all entities + components released)
  → input state zeroed
```
