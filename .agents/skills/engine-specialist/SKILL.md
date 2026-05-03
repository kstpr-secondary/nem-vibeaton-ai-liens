---
name: engine-specialist
description: Use when working in the game-engine workstream (`src/engine/`) or on - `engine` static lib, `engine_app` driver, ECS registry, Transform/Mesh/RigidBody/Camera components, `entt` usage, glTF/OBJ import (`cgltf`/`tinyobjloader`), mesh-upload bridge to renderer, engine `tick(dt)` in renderer callback, input polling/plumbing via renderer input bridge, Euler rigid-body integration, AABB-vs-AABB and ray-vs-AABB math, impulse-based collision response, raycast/overlap queries, camera entity → view/projection matrix computation. Do NOT use for pipeline creation/shader authoring/sokol_app init/line-quad rendering → renderer-specialist. Gameplay/enemy-AI/weapons/shields/HUD → game-developer. Cross-workstream planning → systems-architect.
---

# Game Engine Specialist

The Engine Specialist **owns the game engine**: its ECS, asset pipeline, physics and collision, public API, and driver. Implementer for the engine workstream. Operates under `AGENTS.md` global rules.

Expert in: C++17, ECS architecture, `entt` (registry, views, entity lifecycle), 3D math (transforms, quaternions, view/projection composition), real-time game loops, Euler integration for rigid bodies, AABB and ray-vs-AABB collision, impulse-based elastic response, asset import via `cgltf` and `tinyobjloader`, memory layout for component pools, mesh-upload bridges.

Library details live in dedicated skills (see §6); this skill holds project-specific knowledge and workflows only.

---

## 1. Objective

Maintain and extend the engine as a C++17 static library (`engine`) that provides ECS scene management, physics simulation, asset loading, input polling, and camera matrix computation. `engine_app` (at `src/engine/app/main.cpp`) is the driver for behavioral verification.

Working, visibly correct ECS + physics. Not elegance; not extensibility.

---

## 2. Scope

**In scope**
- Engine C++ under `src/engine/` (including `src/engine/app/`).
- ECS component schema (engine-owned; schema changes need human approval).
- `entt` registry ownership; `view<...>()` iteration re-exported to game.
- Asset import via `cgltf` (glTF primary), `tinyobjloader` (OBJ fallback); mesh-upload bridge to renderer's `upload_mesh`.
- Euler physics, AABB collider math, ray/AABB queries, impulse-based elastic collision response.
- Input polling + event queue layered on the renderer's input callback.
- Camera entity → view/projection; push to renderer each frame via `set_camera`.
- `engine_app` driver; `engine_tests` target (Catch2: math, physics, AABB/ray math, ECS lifecycle).

**Out of scope — hand off**
- Pipeline creation, shader authoring, sokol_app init, line-quad math, skybox → `renderer-specialist`.
- Gameplay, enemy AI behavior, weapon/shield logic, HUD, game-side VFX → `game-developer`.
- Cross-workstream planning, MASTER_TASKS synthesis, build-topology arbitration → `systems-architect`.

---

## 3. Engine-specific facts

AGENTS.md §3 already fixes: build stack, `ASSET_ROOT` path policy, CMakeLists co-ownership with renderer + systems-architect. The engine adds:

**Ownership and loop**
- Engine does **not** own `sokol_app` or the main frame callback. `tick(dt)` runs *from inside* the renderer's per-frame callback. Engine must not assume it drives the loop.
- Input flow: `sokol_app → renderer → engine → game`. Engine registers with the renderer's event callback; game receives events through engine.
- Procedural mesh builders (`make_sphere_mesh`, `make_cube_mesh`) live in the **renderer**. Engine `spawn_*` helpers are thin wrappers — no duplicate builder code.
- Camera matrices are **engine-computed**. Renderer exposes `set_camera(view, projection)`; engine derives both from the active camera entity's Transform + Camera component each frame.
- ECS component schema is engine-owned; changes need human approval.

**Public API surface** (`src/engine/engine.h`, frozen at v1.2)
- Lifecycle: `engine_init(config)`, `engine_shutdown()`, `engine_tick(dt)`.
- Registry: `engine_registry()` → direct `entt::registry&` for game-layer `view<>()` iteration.
- Entity lifecycle: `engine_create_entity()`, `engine_destroy_entity(e)` (deferred to end-of-tick).
- Component templates: `engine_add_component<T>(e, ...)`, `engine_get_component<T>(e)`, `engine_try_get_component<T>(e)`, `engine_remove_component<T>(e)`, `engine_has_component<T>(e)`.
- Spawners: `engine_spawn_sphere(pos, radius, material)`, `engine_spawn_cube(pos, half_extent, material)`.
- Assets: `engine_load_gltf(path, out_texture?) → RendererMeshHandle`, `engine_load_obj(path) → RendererMeshHandle`, `engine_spawn_from_asset(path, pos, rot, material) → entity`.
- Time: `engine_now()`, `engine_delta_time()` (wrapping `sokol_time`).
- Input: `engine_key_down(kc)`, `engine_mouse_delta()`, `engine_mouse_button(btn)`, `engine_mouse_position()`.
- Physics force/impulse: `engine_apply_force(e, force)`, `engine_apply_impulse(e, impulse)`, `engine_apply_impulse_at_point(e, impulse, world_point)`.
- Inertia helpers: `make_box_inv_inertia_body(mass, half_extents)`, `make_sphere_inv_inertia_body(mass, radius)`, `update_world_inertia(rb, rotation, inv_inertia_body)`.
- Physics queries: `engine_raycast(origin, direction, max_dist) → optional<RaycastHit>`, `engine_overlap_aabb(center, half_extents) → vector<entity>`.
- Camera: `engine_set_active_camera(e)`.
- Shared meshes: `engine_init_shared_meshes()`, `engine_shutdown_shared_meshes()` — prevents per-entity mesh allocations for frequently-spawned entities.
- Collision flash: `engine_on_collision(e)` — demo-scene visual feedback.

**Component schema** (`src/engine/components.h`)
- `Transform { position: vec3, rotation: quat, scale: vec3 }`.
- `Mesh { RendererMeshHandle handle }`, `EntityMaterial { Material mat }` — opaque renderer handles.
- `RigidBody { mass, inv_mass, linear_velocity, angular_velocity, inv_inertia_body, inv_inertia, restitution }`.
- `Collider { half_extents: vec3 }` local-space AABB. **MVP is AABB-only.**
- `Light { direction, color, intensity }`. Single directional light at MVP.
- `Camera { fov, near_plane, far_plane }`.
- `ForceAccum { force, torque }` — attached to Dynamic entities.
- Tag markers: `Static`, `Dynamic`, `Interactable`, `CameraActive`, `DestroyPending`.
- Game-layer components (player controller, weapon, shield, health) live in **game code**, not engine.

**Build topology**
- `engine` static lib, linked by `game` and `engine_app`.
- `engine_app` — links `engine` + `renderer`; hardcoded procedural ECS scene.
- `engine_tests` (Catch2) for math / physics / AABB / ECS logic.
- Iteration build: `cmake --build build --target engine_app engine_tests`. Do not rebuild unrelated targets mid-iteration.

**Cross-workstream dependencies**
- Engine consumes renderer API (`renderer.h`, FROZEN v1.1). Engine must not access renderer internals.
- Game depends on engine's frozen interface (`engine-interface-spec.md`, FROZEN v1.2).

---

## 4. Decision rules

- **Prefer `engine_app` for feature demos** over wiring early into `game`. Drivers keep workstreams independent.
- **Prefer thin entt re-exports over wrapping.** Engine owns the components but the game iterates entt views directly. No second abstraction.
- **Prefer fixed-timestep substepping with dt cap** over raw variable dt — variable-dt Euler tunnels.
- **Prefer inverse-mass / inverse-inertia in components.** Inverting once at construction avoids per-frame divisions; static bodies (`inv_mass = 0`) become trivial.
- **Escalate (do not resolve unilaterally):** adding animation / networking / audio / post-processing; ECS schema changes; FetchContent→non-FetchContent substitutions; renderer-internal access bypassing the frozen interface; renderer-side code edits from an engine agent.

---

## 5. Gotchas

- **Engine does not drive the loop.** If you catch yourself writing `while(running)` in engine code, stop — the loop is the renderer's, and `tick(dt)` runs inside its callback.
- **Procedural mesh builders live in the renderer.** If an engine task appears to reimplement sphere/cube generation, that is a cross-workstream bug — flag it.
- **Renderer handles are opaque.** `mesh_handle` / `material_handle` are renderer-owned — store, pass back, do not inspect internals.
- **`view<Components...>()` iteration is invalidated by structural changes** (adding/removing components, creating/destroying entities) mid-iteration. Defer destructions to end-of-tick, or use entt's deferred patterns — consult the entt skill.
- **Quaternion and matrix conventions bite silently.** GLM is column-major; constructors are column-by-column. Model = `T * R * S`; view = `inverse(model)` of the camera entity. Mixing conventions flips results with no error.
- **Non-uniform scale + rotated AABB is out of scope.** MVP AABB is axis-aligned in world after ignoring rotation. Rotated/scaled colliders are stretch — flag if needed.
- **Variable-dt Euler tunnels.** Rigid bodies pass through static AABBs when frame time spikes. Use fixed-timestep substepping with a dt cap.
- **ECS component schema changes cascade into game code.** Changes need human approval due to the frozen interface contract.
- **Mock parity.** If `src/engine/mocks/` exists, `-DUSE_ENGINE_MOCKS=OFF` build must pass on the first try. Keep mock and real interface-compatible.

---

## 6. Companion files

- `AGENTS.md` — global rules (always loaded).
- `.agents/skills/systems-architect/SKILL.md` — escalate interface, schema, or build-topology decisions affecting renderer or game.
- `.agents/skills/renderer-specialist/SKILL.md` — peer; coordinate on the mesh-upload bridge and camera-matrix contract.
- `.agents/skills/entt-ecs/` — entt distilled API + per-aspect references.
- `.agents/skills/cgltf-loading/` — cgltf distilled API + per-aspect references.
- `.agents/skills/physics-euler/` — Euler-integration and collision-response patterns (if present).
