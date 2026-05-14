# Game Engine — Concept, MVP, Milestones

Seed document for Engine SpecKit. Concise but complete. Supersedes the engine sections of `Milestones_MVPs_Concepts.md` after the feasibility review. Runs *after* the renderer interface spec is frozen.

---

## Concept

A minimal ECS-based game engine scoped to a 3D space shooter in an asteroid field. Built on `entt` (ECS), `cgltf` (glTF loading), `tinyobjloader` (OBJ fallback), `GLM` (math), and consumes the renderer public API. The engine ticks from inside the renderer's per-frame callback. Input flows: `sokol_app` → renderer → engine → game.

Responsibilities: scene management (ECS), asset import, update loop, input, Euler physics, AABB collision, raycasting, simple steering AI (desirable), thin gameplay API.

Hard cuts (from blueprint §5): no animation (bones/skins), no networking, no audio, no post-processing, no particle systems, no editor.

---

## Public API Surface (freeze before Game SpecKit)

Names illustrative; SpecKit confirms final signatures.

- **Lifecycle:** `init(renderer&, config)`, `shutdown()`, `tick(dt)` called by renderer frame callback.
- **Scene:** `create_entity()`, `destroy_entity(e)`, `add_component<T>(e, ...)`, `get_component<T>(e)`, `remove_component<T>(e)`, `view<Components...>()` iteration.
- **Procedural shape helpers:** `spawn_sphere(pos, radius, material)`, `spawn_cube(...)`, `spawn_capsule(...)` — thin wrappers that call renderer's `make_*_mesh` and attach the handle to a new entity with Transform + Mesh + Material components.
- **Asset loading:** `load_gltf(path) → mesh_handle`, `load_obj(path) → mesh_handle`, `spawn_from_asset(path, pos, rot) → entity`. Paths resolved through `ASSET_ROOT`.
- **Time:** `now()`, `delta_time()` — wraps `sokol_time`.
- **Input:** polled state (`key_down(K)`, `mouse_delta()`, `mouse_button(b)`) + event callback registration.
- **Physics queries:** `raycast(origin, dir, max_dist) → optional<hit{entity, point, normal, distance}>`, `overlap_aabb(center, extents) → span<entity>`.
- **Camera:** `set_active_camera(entity)`; engine computes view+projection from the camera entity's Transform + Camera component and pushes to renderer each frame.

---

## Component Schemas (tentative — finalized in Engine SpecKit)

- **Transform:** `position: vec3`, `rotation: quat`, `scale: vec3`.
- **Mesh:** `mesh_handle` (renderer handle).
- **Material:** `material_handle` (renderer handle).
- **RigidBody:** `mass: float`, `inv_mass: float`, `linear_velocity: vec3`, `angular_velocity: vec3`, `inv_inertia: mat3`, `restitution: float`.
- **Collider:** `AABB { half_extents: vec3 }` in local space; optional `layer_mask: u32`. MVP is AABB-only.
- **Light:** `Directional { direction, color, intensity }`. MVP has exactly one. Point lights appear in E-M6 (desirable).
- **Camera:** `fov, near, far`.
- **Tag markers:** `Static` (no physics), `Dynamic` (physics-driven), `Interactable` (raycast-visible).

Game-layer components (player controller, weapon state, shield, etc.) live in game code, not engine.

---

## Milestone Group — MVP

### E-M1 — Bootstrap + ECS + Scene API
*Target: ~1h. **Sync point: game workstream starts against this + mocks for later milestones.***

- `entt` registry owned by engine.
- `tick(dt)` invoked from renderer frame callback; engine calls renderer per-frame enqueue for every entity with Transform+Mesh+Material.
- Directional-light entity and active-camera entity creation.
- Full public scene API (create/destroy, add/get/remove components, view iteration).
- Procedural shape spawners (`spawn_sphere`, `spawn_cube`) that consume renderer's mesh builders. Capsule spawner deferred to Desirable (requires renderer R-M5).
- No asset import yet.
- `engine_app` driver: builds a hardcoded procedural ECS scene (handful of primitives) and renders it through the renderer.

**Expected outcome:** `engine_app` shows an ECS-driven procedural scene of spheres and cubes rendered by the renderer. Game workstream can begin coding against engine API + mocks for M2–M4 pieces.

**Files:** `engine_core.{cpp,h}`, `scene_api.{cpp,h}`, `components.h`, `engine_app/main.cpp`.

### E-M2 — Asset Import
*Target: ~45 min. Depends on E-M1.*

- glTF import via `cgltf` — mesh vertices, indices, attributes; no animation, no skinning.
- OBJ import via `tinyobjloader` as fallback.
- Mesh-upload bridge: asset data → renderer `upload_mesh` → returned mesh handle.
- `/assets/` folder resolved via `ASSET_ROOT`.
- `load_gltf` / `load_obj` return mesh handles; `spawn_from_asset` convenience creates an entity.

**Expected outcome:** `engine_app` loads at least one real glTF file from `/assets/` and renders it alongside procedural primitives.

**Files:** `asset_import.{cpp,h}`, `asset_bridge.{cpp,h}`. Two file-disjoint efforts (cgltf path vs obj path) → parallel group candidate.

### E-M3 — Input + AABB Colliders + Raycasting
*Target: ~1h. Depends on E-M1; independent of E-M2 on file set.*

- Input: engine registers with renderer's event callback; exposes polled state (`key_down`, `mouse_delta`, `mouse_button`) + event queue.
- Camera controller wired to WASD + mouse in `engine_app` (game owns its own controller; this one is for the driver).
- AABB Collider component.
- Ray-vs-AABB test, AABB-vs-AABB test, brute force over entities with Collider. Spatial partitioning is stretch.
- `raycast(...)` and `overlap_aabb(...)` public queries.

**Expected outcome:** `engine_app` scene is navigable with WASD+mouse; camera collides with AABB obstacles and cannot pass through; `raycast` returns hits against scene entities.

**Files:** `input.{cpp,h}`, `collider.{cpp,h}`, `raycast.{cpp,h}`, `camera_controller.cpp` (in engine_app). `input` and `collider/raycast` are file-disjoint → parallel group.

### E-M4 — Euler Physics + Collision Response
*Target: ~1h. Depends on E-M3.*

- RigidBody integration: `v += F/m * dt`, `p += v * dt`; angular analog with inverse inertia.
- Force / impulse accumulation API (`apply_force`, `apply_impulse_at_point`).
- Impulse-based elastic collision response between AABB rigid bodies (and between rigid + static).
- Fixed-timestep substepping or dt cap to bound simulation error.
- Time API: `now()`, `delta_time()` wraps `sokol_time`.

**Expected outcome:** `engine_app` demo with a cluster of rigid cubes — applied impulses cause realistic collisions and bounces; static obstacles block without being pushed. **Engine MVP complete.**

**Files:** `physics.{cpp,h}`, `collision_response.{cpp,h}`, `time.{cpp,h}`.

---

## Milestone Group — Desirable

### E-M5 — Enemy Steering AI
*Renamed from draft's "Pathfinding" — this is **steering**, not pathfinding.*

- Seek behavior: steer toward a target position.
- Raycast-ahead obstacle avoidance.
- Used by game for enemy ships homing on the player.

### E-M6 — Multiple Point Lights
*Depends on renderer R-M2 and (ideally) R-M5 for scene illumination beyond one directional source.*

- Point Light component (position, color, intensity, range).
- Engine uploads up to N point lights per frame to renderer.

### E-M7 — Capsule Mesh Integration
- Consume renderer's `make_capsule_mesh` in engine's spawn helpers.
- Demo: `engine_app` renders a capsule alongside spheres and cubes.

---

## Milestone Group — Stretch

- Convex-hull colliders, spatial partitioning (BVH / uniform grid), advanced avoidance.
- True pathfinding (A* on navmesh) — **almost certainly cut**; the asteroid field has no useful navmesh.

**Cut entirely from planning:** animation (skeletal / keyframe), audio, networking, post-processing, particle systems, editor.

---

## Shared Ownership Decisions

- **Procedural shape mesh builders:** live in **renderer**. Engine wraps them into entity-spawn helpers. One implementation, zero duplication.
- **`sokol_app` init, main frame callback, input event pump:** renderer owns. Engine ticks from inside the callback and registers for events.
- **ECS component schema:** engine owns. Changes after Engine SpecKit freeze require human approval (blueprint §8.5 rule 3).
- **Physics and collision primitives:** engine owns. Renderer is rendering-only.
- **CMakeLists.txt:** renderer workstream owns globally; engine owns its own subdirectory CMakeLists (blueprint §8.5 rule 11).

---

## Resolved Open Decisions

- **Procedural shapes live in renderer**, consumed by engine.
- **Engine API exposes engine-owned abstractions**, not raw entt components externally in most cases — except `view<...>()` iteration, which the game will want. Concretely: the engine re-exports enough entt surface to iterate components but owns the component definitions.
- **Mouse-pick / hit-testing UX** is a *game-layer* concern; engine only provides the `raycast` primitive.
- **Pathfinding renamed to steering;** demoted from MVP to desirable (E-M5).
- **Capsule mesh:** renderer R-M5 (Desirable), engine E-M7 (Desirable). MVP engine uses sphere + cube only.
- **Component schema design** is SpecKit output, not a coding milestone.

---

## Notes for SpecKit

- Mock plan: engine exposes stub implementations of its full public API at T+0 (empty scene, constant-return physics queries) so the game workstream is never blocked.
- Parallel group hints:
  - E-M2: glTF path vs OBJ path are file-disjoint.
  - E-M3: input plumbing vs collider/raycast math are file-disjoint.
  - E-M4: integration vs collision-response math are file-disjoint.
- Acceptance checklist per milestone: build passes, `engine_app` demonstrates the expected outcome, unit tests pass where applicable (math/physics are Catch2-testable; rendering integration is behavioral check).
- Interface spec output: `engine-interface-spec.md`. Uses frozen renderer interfaces as input; frozen before Game SpecKit.
- `engine_app` lives at `src/engine/app/main.cpp`; updated each milestone to exercise the new feature.
- Integration with renderer milestones: E-M1 needs R-M1 (unlit meshes). Engine can start on mocks before R-M1 lands; swap when renderer milestone merges. After R-M2 (Lambertian) the engine scene becomes visibly lit with zero engine code changes.
