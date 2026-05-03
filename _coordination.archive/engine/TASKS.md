# TASKS.md — Engine Workstream

> **Live operational task board.** Authoritative on `feature/engine` branch.  
> Translated from `specs/002-ecs-physics-engine/tasks.md`. Source spec: `specs/002-ecs-physics-engine/`.  
> **Before claiming:** read the task row, then the spec section it references.  
> **To claim:** human sets `Owner = <agent>@<machine>`, `Status = CLAIMED`, commits + pushes, then triggers agent.  
> Related docs: `docs/interfaces/engine-interface-spec.md` (draft API), `docs/architecture/engine-architecture.md` (architecture).

---

## Milestone: SETUP — Type Foundation and Infrastructure

**Expected outcome**: `cmake --build build --target engine_app engine_tests` compiles with stub/skeleton bodies. Engine mock compiles under `USE_ENGINE_MOCKS=ON`. Component types and public header exist.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-001 | Define all ECS component structs (Transform, Mesh, EntityMaterial, RigidBody, Collider, Camera, Light) and tag types (Static, Dynamic, Interactable, CameraActive, DestroyPending) in `src/engine/components.h`. Use GLM types, renderer handle types from frozen `renderer.h`. See `specs/002-ecs-physics-engine/data-model.md` for field details | MED | DONE | — | — | SETUP | BOTTLENECK | SPEC-VALIDATE | files: src/engine/components.h |
| E-002 | Create engine public API header with EngineConfig, RaycastHit, all lifecycle/scene/physics/input/camera function declarations, and template component-operation implementations forwarding to entt::registry in `src/engine/engine.h`. Match `docs/interfaces/engine-interface-spec.md` exactly | MED | DONE | — | E-001 | SETUP | PG-SETUP-A | SPEC-VALIDATE | files: src/engine/engine.h. Fixed: added make_box_inv_inertia_body, make_sphere_inv_inertia_body, update_world_inertia declarations per frozen spec v1.2 |
| E-003 | Implement time system: `stm_setup()` init wrapper, `engine_now()` returning seconds since init via `stm_sec(stm_now())`, `engine_delta_time()` returning last frame dt. Store state file-locally | LOW | DONE | — | E-001 | SETUP | PG-SETUP-A | SELF-CHECK | files: src/engine/time.cpp (internal header engine_time.h). Task notes corrected: time.h does not exist, game uses engine_now()/engine_delta_time() via engine.h only |

---

## Milestone: FOUNDATION — Core Lifecycle and Mocks

**Expected outcome**: Engine compiles as a static lib. `engine_init()` / `engine_tick()` / `engine_shutdown()` skeleton works. Game workstream can build against mocks.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-004 | Implement engine core lifecycle: `engine_init(config)` creating `entt::registry` + initializing time + storing physics config, `engine_tick(dt)` skeleton with dt-cap clamp (max 100ms) and positive-minimum clamp, `engine_shutdown()` clearing registry. `engine_registry()` accessor returns registry ref | HIGH | DONE | — | E-002, E-003 | FOUNDATION | SEQUENTIAL | SELF-CHECK | files: src/engine/engine.cpp |
| E-005 | Create engine mock stubs covering all public API functions: no-ops for void, `{1}` for handle-returning, valid sentinel entity for entity-returning, empty results for queries, static mock registry for template operations | MED | DONE | — | E-002 | FOUNDATION | PG-FOUND-A | SELF-CHECK | files: src/engine/mocks/engine_mock.cpp |

**Checkpoint**: Engine compiles as static lib. Game workstream can build with `USE_ENGINE_MOCKS=ON`.

---

## Milestone: E-M1 — Bootstrap + ECS + Scene API (~1h) 🔑 GAME SYNC POINT

**Expected outcome**: `engine_app` displays an ECS-driven procedural scene of spheres and cubes rendered through the renderer. Game workstream can begin coding against engine API + mocks for E-M2–E-M4.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-006 | Write ECS entity lifecycle tests (Catch2): create entity, destroy (deferred via DestroyPending), valid/invalid checks, component emplace/get/remove/has, view iteration. **Write tests FIRST, ensure they FAIL** | LOW | DONE | — | E-004 | E-M1 | PG-EM1-A | SELF-CHECK | files: src/engine/tests/test_ecs.cpp |
| E-007 | Write transform and matrix composition tests (Catch2): TRS model matrix construction, identity quaternion, translate-rotate-scale correctness, view matrix from camera transform inverse. **Write tests FIRST, ensure they FAIL** | LOW | DONE | — | E-004 | E-M1 | PG-EM1-A | SELF-CHECK | files: src/engine/tests/test_math.cpp |
| E-008 | Implement scene API: `engine_create_entity()`, `engine_destroy_entity(e)` with DestroyPending deferral, entity validity check, end-of-tick destruction sweep iterating `view<DestroyPending>` and calling `registry.destroy()` | MED | DONE | — | E-004 | E-M1 | PG-EM1-B | SELF-CHECK | files: src/engine/scene_api.h, src/engine/scene_api.cpp |
| E-009 | Implement procedural shape spawners: `engine_spawn_sphere()` and `engine_spawn_cube()` creating entities with Transform + Mesh + EntityMaterial using `renderer_make_sphere_mesh()` / `renderer_make_cube_mesh()` from frozen renderer API | MED | DONE | — | E-008 | E-M1 | SEQUENTIAL | SELF-CHECK | files: src/engine/scene_api.cpp |
| E-010 | Implement camera system: compute view matrix from inverse TRS of CameraActive entity, projection matrix via `glm::perspective` with aspect from `sapp_widthf()/sapp_heightf()`, push to `renderer_set_camera()` each tick | MED | DONE | — | E-004 | E-M1 | PG-EM1-B | SELF-CHECK | files: src/engine/camera.h, src/engine/camera.cpp |
| E-011 | Wire render enqueue loop into `engine_tick()`: iterate `view<Transform, Mesh, EntityMaterial>` calling `renderer_enqueue_draw()` per entity with TRS model matrix. Call camera update. Upload directional light via `renderer_set_directional_light()` from Light entity | MED | DONE | — | E-008, E-010 | E-M1 | SEQUENTIAL | SELF-CHECK | files: src/engine/engine.cpp |
| E-012 | Create `engine_app` E-M1 driver: `renderer_init()` → `engine_init()` → register frame callback (calls `renderer_begin_frame()` / `engine_tick(dt)` / `renderer_end_frame()`) → spawn camera entity + light entity + 10 procedural spheres/cubes at varied positions → `renderer_run()` | LOW | DONE | — | E-011, E-009 | E-M1 | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp |

**Checkpoint**: `engine_app` displays procedural ECS scene. `test_ecs.cpp` and `test_math.cpp` pass. E-M1 acceptance: SC-001, SC-006.

---

## Milestone: E-M2 — Asset Import (~45 min)

**Expected outcome**: `engine_app` loads at least one real glTF file from `/assets/` and renders it alongside procedural primitives.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-013 | Implement glTF/GLB mesh loader: `cgltf_parse()` → `cgltf_load_buffers()` → walk mesh primitives extracting positions/normals/UVs/indices. Handle GLB vs glTF paths. Resolve via `ASSET_ROOT` macro. Log `[ENGINE]` warning for unsupported features (skeletal animation, morph targets). Return extracted vertex/index arrays | HIGH | DONE | — | E-012 | E-M2 | PG-EM2-A | SELF-CHECK | files: src/engine/asset_import.h, src/engine/asset_import.cpp |
| E-014 | Implement OBJ mesh fallback loader: `tinyobjloader::LoadObj()`, extract positions/normals/UVs/indices per shape. Resolve via `ASSET_ROOT` macro. Return extracted vertex/index arrays | MED | DONE | — | E-012 | E-M2 | PG-EM2-B | SELF-CHECK | files: src/engine/obj_import.h, src/engine/obj_import.cpp |
| E-015 | Implement asset bridge: convert extracted vertex arrays to renderer `Vertex` layout (zero-fill tangent if absent, Y-up default normal if absent), call `renderer_upload_mesh()`, return `RendererMeshHandle`. Bridge serves both glTF and OBJ paths | MED | DONE | — | E-013, E-014 | E-M2 | SEQUENTIAL | SELF-CHECK | files: src/engine/asset_bridge.h, src/engine/asset_bridge.cpp |
| E-016 | Implement `engine_spawn_from_asset()`: load mesh via asset_import or obj_import (detect by extension), bridge to renderer handle, create entity with Transform + Mesh + EntityMaterial. `engine_load_gltf()` and `engine_load_obj()` return handles directly | LOW | DONE | — | E-015 | E-M2 | SEQUENTIAL | SELF-CHECK | files: src/engine/scene_api.cpp |
| E-017 | Update `engine_app` for E-M2: load a glTF model from `assets/` directory, spawn it alongside existing procedural primitives, verify mixed rendering works | LOW | DONE | — | E-016 | E-M2 | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp |

**Checkpoint**: `engine_app` renders glTF models alongside procedurals. SC-002 met. Asset loading errors return `{0}` without crashing.

---

## Milestone: E-M3 — Input + AABB Colliders + Raycasting (~1h)

**Expected outcome**: `engine_app` scene is navigable with WASD+mouse; camera collides with AABB obstacles; `raycast()` and `overlap_aabb()` return hits against scene entities.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-018 | Write collision primitive tests (Catch2): AABB-vs-AABB overlap true/false cases, world AABB computation from position + half_extents, ray-vs-AABB slab method hit/miss/distance/normal cases. **Write tests FIRST, ensure they FAIL** | LOW | DONE | — | E-012 | E-M3 | PG-EM3-A | SELF-CHECK | files: src/engine/tests/test_collision.cpp |
| E-019 | Implement input bridge: register `InputCallback` with `renderer_set_input_callback()`, update `InputState` singleton (key_states[], mouse position, mouse delta, mouse buttons) from sokol_app events. Expose `engine_key_down()`, `engine_mouse_delta()`, `engine_mouse_button()`, `engine_mouse_position()` | MED | DONE | — | E-004 | E-M3 | PG-EM3-B | SELF-CHECK | files: src/engine/input.h, src/engine/input.cpp |
| E-020 | Implement collider system: compute world AABB from `Transform.position ± Collider.half_extents`, brute-force all-pairs AABB-vs-AABB overlap detection returning collision pairs with penetration depth and contact normal (minimum-penetration SAT axis) | HIGH | DONE | — | E-008 | E-M3 | PG-EM3-A | SELF-CHECK | files: src/engine/collider.h, src/engine/collider.cpp |
| E-021 | Implement raycast and overlap queries: `engine_raycast()` via ray-vs-AABB slab method returning nearest `RaycastHit` (entity, point, normal, distance); `engine_overlap_aabb()` returning all intersecting entities | MED | DONE | — | E-020 | E-M3 | SEQUENTIAL | SELF-CHECK | files: src/engine/raycast.h, src/engine/raycast.cpp |
| E-022 | Wire input callback registration into `engine_init()`, add mouse delta computation to `engine_tick()` start. Ensure input state is updated before physics/gameplay each frame | MED | DONE | — | E-019 | E-M3 | SEQUENTIAL | SELF-CHECK | files: src/engine/engine.cpp |
| E-023 | Update `engine_app` for E-M3: WASD+mouse FPS-style camera controller, spawn 50+ collidable entities (validates SC-005 raycast scale), highlight entity under crosshair via raycast, display hit info via `fprintf` | LOW | DONE | — | E-021, E-022 | E-M3 | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp. Scene: 40 cubes + 20 asteroids (glTF fallback to cubes) + 4 static walls + central asteroid = 60+ collidable entities with Lambertian materials, directional light, crosshair raycast every frame, hit info printed once/second. Post-E-M3 fix: reversed glTF index winding in asset_import.cpp to resolve inside-out asteroid rendering (CW→CCW conversion). |

**Checkpoint**: `engine_app` navigable with WASD+mouse. Raycasts return correct hits. `test_collision.cpp` passes. SC-004, SC-005 met.

---

## Milestone: E-M4 — Euler Physics + Collision Response (~1h) 🔑 ENGINE MVP COMPLETE

**Expected outcome**: `engine_app` demo with 20+ rigid bodies — applied impulses cause realistic elastic collisions and bounces; static obstacles block without being pushed. Physics substeps at 120 Hz. Energy conserved within 5% over 10s.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-024 | Write physics unit tests (Catch2): Euler linear integration (p += v*dt), angular integration with quaternion renormalization, force→acceleration, impulse→velocity, elastic two-body collision energy conservation (within 5% over 10s), static body zero-velocity-change, dt-cap clamp behavior. **Write tests FIRST, ensure they FAIL** | LOW | DONE | — | E-020 | E-M4 | PG-EM4-A | SELF-CHECK | files: src/engine/tests/test_physics.cpp |
| E-025 | Implement Euler integration and force API: linear integration (p += v*dt, v += F/m*dt), angular integration (ω += I⁻¹*τ*dt, q += 0.5*ω*q*dt then renormalize), world-space inertia recomputation (R*I_body_inv*Rᵀ), `apply_force`/`apply_impulse`/`apply_impulse_at_point` writing to per-entity `ForceAccum` map, fixed-timestep substep loop with 120 Hz accumulator and dt-cap | HIGH | DONE | — | E-020 | E-M4 | PG-EM4-B | REVIEW | files: src/engine/physics.h, src/engine/physics.cpp |
| E-026 | Implement collision response: impulse-based elastic response with hardcoded `restitution = 1.0`, handle dynamic-dynamic and dynamic-static (`inv_mass=0`) pairs, combined restitution via `min(e_A, e_B)`, Baumgarte positional correction (`k_slop=0.005`, `k_baumgarte=0.3`), clear force accumulators after each substep | HIGH | DONE | — | E-025 | E-M4 | SEQUENTIAL | REVIEW | files: src/engine/collision_response.h, src/engine/collision_response.cpp |
| E-027 | Wire complete physics pipeline into `engine_tick()`: fixed-timestep substep loop containing force accumulation → Euler integration → world inertia update → AABB collision detection (from `collider.h`) → impulse response → Baumgarte correction → force clear. Run before camera/render enqueue | MED | DONE | — | E-025, E-026 | E-M4 | SEQUENTIAL | SELF-CHECK | files: src/engine/engine.cpp |
| E-028 | Update `engine_app` for E-M4: spawn 20+ dynamic rigid bodies with random initial velocities + 4 static obstacle walls, demonstrate elastic collisions in zero gravity, verify ≥30 FPS with 100 rigid bodies (SC-003, SC-007) | LOW | DONE | — | E-027 | E-M4 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: src/engine/app/main.cpp |

**Checkpoint**: 20+ rigid bodies collide elastically in zero gravity. Energy conservation within 5%. Static obstacles immovable. `test_physics.cpp` passes. **Engine MVP complete.** SC-003, SC-007, SC-008 met.

---

## Milestone: POST-MVP — API Freeze + Polish

**Expected outcome**: Engine public API surface finalized and frozen. Game workstream fully unblocked. All tests pass, full build clean.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-029 | Finalize `engine.h` public header: verify all template component operations compile from an external translation unit, ensure `engine_registry()` exposes direct `entt::registry&`, add any missing declarations discovered during E-M1–E-M4 | LOW | TODO | — | E-028 | POST-MVP | PG-POST-A | SPEC-VALIDATE | files: src/engine/engine.h |
| E-030 | Update engine mock stubs to cover full finalized API surface (all new functions from E-M1–E-M4) | LOW | TODO | — | E-029 | POST-MVP | PG-POST-A | SELF-CHECK | files: src/engine/mocks/engine_mock.cpp |
| E-031 | Promote engine-api.md contract to frozen interface spec: update `docs/interfaces/engine-interface-spec.md` status to `FROZEN — v1.0`. **Requires human supervisor approval** | MED | TODO | — | E-029 | POST-MVP | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: docs/interfaces/engine-interface-spec.md |
| E-032 | Run all Catch2 engine tests and verify 100% pass: `cmake --build build --target engine_tests && ./build/engine_tests` | LOW | TODO | — | E-028 | POST-MVP | PG-POST-B | SELF-CHECK | files: (verification only) |
| E-033 | Verify `engine_app` runs at ≥30 FPS with 100 active rigid bodies on target hardware via `./build/engine_app` (SC-007) | LOW | TODO | — | E-028 | POST-MVP | PG-POST-B | SELF-CHECK | files: (verification only) |
| E-034 | Run full build verification (all workstreams): `cmake --build build` returns 0 with no engine-originated warnings | LOW | TODO | — | E-032 | POST-MVP | SEQUENTIAL | SELF-CHECK | files: (verification only) |
| E-035 | Update `_coordination/engine/PROGRESS.md` with milestone completion status | LOW | TODO | — | E-028 | POST-MVP | PG-POST-B | NONE | files: _coordination/engine/PROGRESS.md |

---

## Milestone: DEMO-SCENE — Stress Test + Collision Visualization

**Expected outcome**: `engine_app` shows 10 static objects (different asteroid meshes, cyan/blue base color), dynamic objects glow red for 0.5s on collision then fade back to base color over another 0.5s, ImGui stats HUD displays FPS + draw calls + triangle count, and a spawner that increases total object count over time for stress testing.

> **Context**: Engine MVP is complete (E-M1–E-M4 DONE, POST-MVP TODO). This is a post-MVP demo-scene extension — no new engine APIs are needed beyond what already exists. All work is in `engine_app` (the driver) plus one renderer API extension (`renderer_get_triangle_count`). The existing Lambertian/unlit materials, ECS entity lifecycle, physics tick loop, and ImGui infrastructure (already linked via sokol_imgui in the renderer) are reused directly.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| E-036 | Extend renderer API: add `renderer_get_triangle_count()` to `renderer.h`, implement in `renderer.cpp` by summing `mesh_index_count_get(cmd.mesh.id) / 3` for each entry in `draw_queue[0..draw_count-1]` during `end_frame()`, store as `triangle_count` in `RendererState`, reset to 0 at `begin_frame()` | MED | DONE | claude@laptopA | E-035 | DEMO-SCENE | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.h, src/renderer/renderer.cpp. Exposes total index_count/3 accumulated per frame from draw_queue. |
| E-037 | Add `CollisionFlash` component to `src/engine/components.h`: fields `float timer = 0.f`, `glm::vec3 flash_color = {1.f, 0.15f, 0.15f}` (red), `glm::vec3 base_color[3]` (saved from EntityMaterial at collision time). Add `engine_on_collision(entt::entity e)` API in `engine.h`/`engine.cpp` that creates CollisionFlash on an entity with current Material base_color as the saved base. | MED | DONE | claude@laptopA | E-036 | DEMO-SCENE | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/components.h, src/engine/engine.h, src/engine/engine.cpp. Thin wrapper: reads EntityMaterial, stores base_color, resets timer=1.0f. Note: spec says base_color[3], simplified to single glm::vec3 base_color (only one value needed per E-039 blending). |
| E-038 | Hook collision event tracking into physics loop in `collision_response.cpp`: after `resolve_collision()` fires (non-zero impulse), call `engine_on_collision(pair.a)` and `engine_on_collision(pair.b)` for any entity that has both `Dynamic` tag and `EntityMaterial`. Uses entt `reg.has<EntityMaterial>()` check. No new engine API needed — just call through to `engine_on_collision`. | MED | DONE | claude@laptopA | E-037 | DEMO-SCENE | SEQUENTIAL | SELF-CHECK | files: src/engine/collision_response.cpp. Guard: only Dynamic entities with EntityMaterial get flash. Static walls and central asteroid are tagged Static → no flash. Checks v_rel_n < 0 before resolve_collision to detect non-zero impulse. |
| E-039 | Update render enqueue loop in `engine_app/main.cpp`: for each entity drawn, check `CollisionFlash` component; if `timer > 0.f`, blend between flash_color and saved base_color based on `timer / 1.0f` (linear interpolation: `lerp(flash, base, 1.0f - timer/1.0f)` with clamp at timer=0.5s transition point); modify EntityMaterial.mat.base_color per draw call via a temporary Material copy. | MED | DONE | claude@laptopA | E-037, E-038 | DEMO-SCENE | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp. Blend curve: timer 1.0→0.5 = flash to mid-blend; timer 0.5→0.0 = mid-blend to base_color. Timer decay happens in render loop (dt-based), CollisionFlash removed when timer <= 0 via engine_remove_component. Uses engine_try_get_component since CollisionFlash is optional/not in view. Manual lerp instead of glm::mix (no vector_operations.hpp available). |
| E-040 | Update `setup_scene()` in `engine_app/main.cpp`: replace the single static arena wall + central asteroid with 10 static objects at varied positions using 4 different glTF asteroid models (`Asteroid_1a.glb`, `Asteroid_1e.glb`, `Asteroid_2a.glb`, `Asteroid_2b.glb`), each with cyan/blue Lambertian base color (`{0.2f, 0.7f, 0.9f}` or `{0.15f, 0.6f, 0.85f}`). Vary scales: small (1.0–1.5), medium (2.0–2.5), large (3.0–4.0). Keep collider half_extents proportional to scale. | MED | DONE | claude@laptopA | E-039 | DEMO-SCENE | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp. Scene layout: 10 static asteroids spread across arena at different positions, varied models/scales/colors. |
| E-041 | Add ImGui stats HUD to `engine_app/main.cpp`: include `<imgui.h>`, call `simgui_new_frame()` in frame callback (renderer already links sokol_imgui), render a small window in top-left showing: `FPS: %.1f`, `Draws: %d`, `Triangles: %d`, `Entities: %d` (dynamic count from view). Use `ImGuiCond_Once | ImGuiWindowFlags_NoDecoration`. Reuse the renderer app's HUD pattern (lines 311-321 of renderer/app/main.cpp) as reference. | LOW | DONE | claude@laptopA | E-040 | DEMO-SCENE | SEQUENTIAL | SELF-CHECK | files: src/engine/app/main.cpp. ImGui linked via vibeaton::imgui_backend in CMake. |
| E-042 | Add stress-test spawner to `engine_app/main.cpp`: static state tracking `spawn_timer`, `spawn_interval` (starts at 2.0s, decreases to 0.3s), `max_entities` (capped at 200). Each interval: pick random position within arena bounds, random velocity, pick random asteroid model or cube, assign random mass tier (small=1.0, medium=3.0, large=5.0), spawn with Dynamic tag + RigidBody + Collider. At `max_entities`, stop spawning until count drops below 150 (destroy oldest N entities tagged SpawnTarget with DestroyPending). | MED | DONE | claude@laptopA | E-041 | DEMO-SCENE | SEQUENTIAL | SPEC-VALIDATE | files: src/engine/app/main.cpp. Spawner loop runs in frame_cb, not in engine_tick. SpawnTarget tag component added to components.h. |

---

## Parallel Group Summary

| Group | Milestone | Tasks | File Sets (disjoint) |
|---|---|---|---|
| PG-SETUP-A | SETUP | E-002, E-003 | `engine.h` / `time.h, time.cpp` |
| PG-FOUND-A | FOUNDATION | E-005 (parallel with E-004) | `mocks/engine_mock.cpp` / `engine.cpp` |
| PG-EM1-A | E-M1 | E-006, E-007 | `tests/test_ecs.cpp` / `tests/test_math.cpp` |
| PG-EM1-B | E-M1 | E-008, E-010 | `scene_api.h, scene_api.cpp` / `camera.h, camera.cpp` |
| PG-EM2-A | E-M2 | E-013 | `asset_import.h, asset_import.cpp` |
| PG-EM2-B | E-M2 | E-014 | `obj_import.h, obj_import.cpp` |
| PG-EM3-A | E-M3 | E-018, E-020 | `tests/test_collision.cpp` / `collider.h, collider.cpp` |
| PG-EM3-B | E-M3 | E-019 | `input.h, input.cpp` |
| PG-EM4-A | E-M4 | E-024 | `tests/test_physics.cpp` |
| PG-EM4-B | E-M4 | E-025 | `physics.h, physics.cpp` |
| PG-POST-A | POST-MVP | E-029, E-030 | `engine.h` / `mocks/engine_mock.cpp` |
| PG-POST-B | POST-MVP | E-032, E-033, E-035 | (verification / PROGRESS.md) |
| PG-DEMO-A | DEMO-SCENE | E-036 | `renderer.h, renderer.cpp` |
| PG-DEMO-B | DEMO-SCENE | E-037 | `components.h, engine.h, engine.cpp` |
| PG-DEMO-C | DEMO-SCENE | E-040, E-041 | (both modify `main.cpp` — SEQUENTIAL, not parallel) |

---

## Milestone Dependency Graph

```
SETUP (E-001..E-003) → FOUNDATION (E-004..E-005)
    → E-M1 (E-006..E-012)
        → E-M2 (E-013..E-017)  ─────────────┐
        → E-M3 (E-018..E-023)  ─────────────┤
                                                ├→ E-M4 (E-024..E-028) → POST-MVP (E-029..E-035)
                                                                     → DEMO-SCENE (E-036..E-042)
```

E-M2 and E-M3 are **independent** after E-M1; both must complete before E-M4.  
DEMO-SCENE is entirely sequential: E-036 (renderer extension) → E-037 (engine component) → E-038..E-042 (app driver work).

---

## Cross-Workstream Dependencies

| Engine Milestone | Requires Renderer | Unlocks |
|---|---|---|
| E-M1 | R-M1 (or `USE_RENDERER_MOCKS=ON`) | Game G-M1 can start |
| E-M2 | R-M1 (for `renderer_upload_mesh()`) | Game asset loading |
| E-M3 | R-M1 (for input bridge) | Game G-M3 (raycast + colliders) |
| E-M4 | R-M1 | Game G-M2 (physics + collision response) |
| POST-MVP | R-M1 | Game workstream fully unblocked (frozen engine API) |
| DEMO-SCENE | E-036 touches frozen `renderer.h` — **requires human approval** for interface extension | Extended `engine_app` for stress testing / collision viz demo |
