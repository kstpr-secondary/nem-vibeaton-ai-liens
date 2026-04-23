---
name: entt-ecs
description: Use when implementing, designing, or reviewing ECS code in the game-engine workstream. Covers entt v3.x registry lifecycle, entity create/destroy, component emplace/get/remove, view iteration, exclusion filters, and structural-change safety rules. Activated when writing or reviewing any code that calls entt::registry, entt::entity, or entt views. Do NOT use for gameplay-layer component design (game code owns those) or renderer mesh/material handle internals — those belong to renderer-specialist.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: library-reference
  activated-by: entt::registry or entt::entity usage
---

# entt-ecs Skill

Reference for `entt` v3.x used in the game-engine workstream (`src/engine/`).
**The engine owns the single `entt::registry`. The game accesses component data through the engine's re-exported `view<...>()` call — it does not hold its own registry instance.**

Use per-aspect references under `references/` when you need deeper detail. Read only what the task requires.

---

## 1. Core Rules (must know)

1. **One registry.** `entt::registry` lives in the engine, constructed once in `init()`. The game never creates its own.
2. **`emplace<T>` is the add operation.** The engine wraps it in `add_component<T>(e, ...)` for its public API; the underlying call is always `emplace`.
3. **`get<T>` is UB if absent.** Use `try_get<T>` (returns `T*`, nullptr if missing) whenever presence is uncertain.
4. **Structural changes mid-view are UB.** Never emplace or remove a component type that is part of the active view while iterating it. Defer to end-of-tick.
5. **Tag components are empty structs.** `struct Static {};` Presence is the value — no data fields on tags.
6. **Game code uses re-exported views.** The engine exposes `view<Components...>()` forwarding to its registry. Game code may not hold a raw `entt::registry*`.
7. **Entities are `uint32_t` handles.** `entt::null` is the invalid sentinel. Test liveness with `registry.valid(e)`.

---

## 2. Registry and Entity Lifecycle

```cpp
#include <entt/entt.hpp>

entt::registry registry;               // owned by engine, created once in init()

entt::entity e  = registry.create();   // allocate a new entity
bool alive      = registry.valid(e);   // false after destroy, or for entt::null
size_t count    = registry.alive();    // total living entities

registry.destroy(e);                   // frees entity + all its components
```

**Safe batch destroy** — collect first, then destroy, to avoid invalidating iteration:
```cpp
std::vector<entt::entity> dead;
registry.view<DestroyPending>().each([&](auto e){ dead.push_back(e); });
for (auto e : dead) registry.destroy(e);
```

---

## 3. Component Operations

```cpp
// Emplace — construct in-place, returns T& to the new component
auto& t = registry.emplace<Transform>(e, glm::vec3{0.f}, glm::quat{1,0,0,0}, glm::vec3{1.f});

// Emplace or replace (idempotent; safe to call repeatedly)
registry.emplace_or_replace<Transform>(e, ...);

// Get — UB if component absent; use only when guaranteed present
Transform& t = registry.get<Transform>(e);

// Safe get — returns nullptr if absent
Transform* t = registry.try_get<Transform>(e);

// Patch — modify in-place without move/copy
registry.patch<Transform>(e, [](Transform& t){ t.position.y += 1.f; });

// Remove — safe even if component is absent (entt ≥ 3.x, no exception)
registry.remove<Transform>(e);

// Presence check
bool has_all = registry.all_of<Transform, Mesh>(e);
bool has_any = registry.any_of<Transform, RigidBody>(e);

// Tag component (empty struct; presence is the semantic)
registry.emplace<Static>(e);
bool is_static = registry.all_of<Static>(e);

// Count entities with a given component
size_t n = registry.storage<RigidBody>().size();
```

---

## 4. View Iteration

Views are cheap to create (no allocation) — always create fresh per tick, never cache across structural changes.

```cpp
// Multi-component view — iterate all entities that have every listed type
auto view = registry.view<Transform, Mesh, Material>();

// Structured-binding form (preferred, C++17)
for (auto [entity, transform, mesh, material] : view.each()) {
    renderer.enqueue_draw(mesh.handle, make_model_matrix(transform), material.handle);
}

// Lambda form (equivalent)
view.each([&](entt::entity e, Transform& t, Mesh& m, Material& mat) {
    renderer.enqueue_draw(m.handle, make_model_matrix(t), mat.handle);
});

// Exclusion filter — skip entities with Static tag during physics tick
auto dyn = registry.view<Transform, RigidBody>(entt::exclude<Static>);
for (auto [e, t, rb] : dyn.each()) { integrate(t, rb, dt); }

// Fetch a single component from a view (avoids double lookup)
Transform& t = view.get<Transform>(entity);
```

---

## 5. Structural-Change Safety

**Rule: never emplace, remove, or destroy an entity for any component type included in the current view while iterating that view.**

Symptoms if violated: skipped entities, double-processing, silent corruption, crash.

**Safe pattern — defer all structural changes to end-of-tick:**
```cpp
// Phase 1: collect intents during iteration
std::vector<std::pair<entt::entity, glm::vec3>> spawn_requests;
registry.view<Weapon, Transform>().each([&](auto e, Weapon& w, Transform& t) {
    if (w.fire_requested) {
        spawn_requests.push_back({e, t.position + t.forward()});
        w.fire_requested = false;
    }
});

// Phase 2: apply structural changes after all views are done
for (auto [src, pos] : spawn_requests) {
    auto bullet = registry.create();
    registry.emplace<Transform>(bullet, pos, glm::quat{1,0,0,0}, glm::vec3{0.1f});
    registry.emplace<RigidBody>(bullet, ...);
    registry.emplace<Collider>(bullet, glm::vec3{0.05f});
    registry.emplace<Interactable>(bullet);
}
```

---

## 6. Project Component Schema

Defined in `src/engine/components.h`. Engine-owned; changes after the interface freeze (`engine-interface-spec.md@v1.0`) require human approval and cascade to game code.

| Component | Key fields | Notes |
|-----------|-----------|-------|
| `Transform` | `position: vec3`, `rotation: quat`, `scale: vec3` | All world-space entities |
| `Mesh` | `mesh_handle` (renderer opaque) | Do not inspect renderer internals |
| `Material` | `material_handle` (renderer opaque) | Do not inspect renderer internals |
| `RigidBody` | `mass, inv_mass: float`; `linear_velocity, angular_velocity: vec3`; `inv_inertia: mat3`; `restitution: float` | `inv_mass = 0` keeps entity in physics broadphase but unmovable; use `Static` to exclude entirely |
| `Collider` | `half_extents: vec3` (AABB, local space) | MVP: AABB only; world AABB = translate by position, **ignore rotation** |
| `Light` | `direction: vec3`, `color: vec3`, `intensity: float` | MVP: exactly one directional; point lights = E-M6 (Desirable) |
| `Camera` | `fov: float`, `near: float`, `far: float` | Engine pushes view+proj to renderer each tick |

**Tag markers (empty structs — presence is semantic):**

| Tag | Meaning |
|-----|---------|
| `Static` | Excluded from physics integration; still collidable |
| `Dynamic` | Under physics simulation |
| `Interactable` | Visible to `raycast` and `overlap_aabb` queries |
| `CameraActive` | Marks the one active camera entity per scene |
| `DestroyPending` | Queued for end-of-tick `registry.destroy()` sweep |

**Game-layer components** — player controller, weapon state, shield, health, enemy AI — live in `src/game/`. Never define gameplay state in `src/engine/components.h`.

---

## 7. Engine Public API (entt surface re-exported to game)

The engine wraps registry calls to decouple game code from raw entt. These map 1:1 to entt internals:

```cpp
// Signatures from engine-interface-spec (freeze before Game SpecKit)
entt::entity create_entity();
void          destroy_entity(entt::entity e);

template<typename T, typename... Args>
T&   add_component(entt::entity e, Args&&... args);    // → registry.emplace<T>

template<typename T>
T&   get_component(entt::entity e);                    // → registry.get<T>

template<typename T>
void remove_component(entt::entity e);                 // → registry.remove<T>

template<typename... Components>
auto view();   // → registry.view<Components...>() — game iterates directly
```

The game receives raw entt views and iterates with structured bindings. The engine does **not** add a second abstraction layer over view iteration.

---

## 8. Model Matrix Construction

Engine computes the model matrix from `Transform` before each draw enqueue:

```cpp
glm::mat4 make_model_matrix(const Transform& t) {
    return glm::translate(glm::mat4{1.f}, t.position)
         * glm::mat4_cast(t.rotation)
         * glm::scale(glm::mat4{1.f}, t.scale);  // TRS — column-major, GLM default
}
```

Camera view matrix: `glm::inverse(make_model_matrix(camera_transform))`.  
Camera projection: `glm::perspective(glm::radians(cam.fov), aspect, cam.near, cam.far)`.

---

## 9. Gotchas

- **`get<T>` with no assert in release.** UB is silent. Use `try_get` in any uncertain code path.
- **Views are not snapshots.** The view reflects current registry state on each call. Cached views become stale after structural changes.
- **Destroying inside `view.each()` is UB.** Use `DestroyPending` tag + end-of-tick sweep (§5).
- **Handle reuse after `destroy`.** The same `entt::entity` value may be reassigned to a new entity. Never store handles across ticks without re-validating with `registry.valid(e)`.
- **Non-uniform scale + AABB.** MVP AABB ignores `Transform.scale` — world half-extents equal local half-extents translated by position only. This is a known limitation; note it in the task.
- **Exactly one `CameraActive` per scene.** Multiple tags yield undefined camera selection. Engine must enforce single-active invariant on `set_active_camera(e)`.
- **Angular integration is optional at MVP.** E-M4 marks angular velocity as cuttable if time runs short. Do not gate collision response or the physics milestone on it.
- **Schema freeze cascade.** Any component field added after `engine-interface-spec.md@v1.0` can silently break game-side iteration code. Treat the schema as a contract, not internal state.

---

## 10. Deeper References

Open only the reference relevant to your current task:

| Aspect | File | When to open |
|--------|------|-------------|
| Registry basics | `references/entt-registry-basics.md` | Entity pools, version bits, registry copy/move, storage inspection |
| Component views (advanced) | `references/entt-component-views.md` | Groups, multi-view cache optimization, view vs group trade-offs |
| Entity lifecycle edge cases | `references/entt-entity-lifecycle.md` | Handle reuse, version fields, deferred destruction context, serialization |
