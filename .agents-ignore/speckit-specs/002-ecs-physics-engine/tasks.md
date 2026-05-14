# Tasks: ECS Physics Engine

**Input**: Design documents from `/specs/002-ecs-physics-engine/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/engine-api.md ✅, quickstart.md ✅

**Tests**: Included — the spec explicitly requires Catch2 tests for math, physics, collision primitives, and ECS lifecycle (SC-008, plan.md §Testing).

**Organization**: Tasks are grouped by user story and aligned with milestone gates (E-M1 → E-M4). US4 (Input + Colliders + Raycasting) is scheduled before US3 (Physics) because the collision pipeline depends on the collider system.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: Which user story this task belongs to (US1–US5)
- Include exact file paths in descriptions

## Path Conventions

All paths relative to repository root. Engine sources in `src/engine/`, tests in `src/engine/tests/`, driver in `src/engine/app/`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Create the engine's type foundation — component definitions, public API header, and time system. These files are imported by every subsequent phase.

- [x] T001 Define all ECS component structs (Transform, Mesh, EntityMaterial, RigidBody, Collider, Camera, Light) and tag types (Static, Dynamic, Interactable, CameraActive, DestroyPending) in src/engine/components.h
- [x] T002 [P] Create engine public API header with EngineConfig, RaycastHit, all lifecycle/scene/physics/input/camera function declarations, and template component-operation implementations forwarding to entt::registry in src/engine/engine.h
- [x] T003 [P] Implement time system: sokol_time init wrapper, engine_now() returning seconds since init, engine_delta_time() returning last frame dt in src/engine/engine_time.h and src/engine/time.cpp

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core lifecycle that every system depends on — the entt::registry owner, the tick skeleton, and mock stubs for the downstream game workstream.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [x] T004 Implement engine core lifecycle: engine_init(config) creating entt::registry + initializing time, engine_tick(dt) skeleton with dt-cap clamp, engine_shutdown() clearing registry in src/engine/engine.cpp
- [x] T005 [P] Create engine mock stubs covering all public API functions (no-ops for void, valid sentinels for handles, empty results for queries, static mock registry for templates) in src/engine/mocks/engine_mock.cpp

**Checkpoint**: Foundation ready — engine compiles as static lib, game workstream can build against mocks, user story implementation can begin.

---

## Phase 3: User Story 1 — Build a Procedural ECS Scene from Code (Priority: P1) 🎯 MVP

**Goal**: Initialize the engine, create entities through the scene API, attach Transform + Mesh + Material components, and see them rendered on screen. Procedural spheres and cubes can be spawned at arbitrary positions.

**Milestone Gate**: E-M1 — `engine_app` displays procedural spheres/cubes via ECS-driven renderer enqueue.

**Independent Test**: Spawn 10+ procedural primitives via the scene API; verify they appear rendered with correct positions and orientations.

### Tests for User Story 1

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T006 [P] [US1] Write ECS entity lifecycle tests: create, destroy (deferred), valid/invalid checks, component emplace/get/remove/has in src/engine/tests/test_ecs.cpp
- [x] T007 [P] [US1] Write transform and matrix composition tests: TRS model matrix, identity quaternion, translate-rotate-scale correctness, view matrix from camera transform in src/engine/tests/test_math.cpp

### Implementation for User Story 1

- [x] T008 [US1] Implement scene API: engine_create_entity(), engine_destroy_entity() with DestroyPending deferral, engine_registry() accessor, entity validity via registry.valid(e), end-of-tick destruction sweep in src/engine/scene_api.h and src/engine/scene_api.cpp
- [x] T009 [US1] Implement procedural shape spawners: engine_spawn_sphere() and engine_spawn_cube() creating entities with Transform + Mesh + EntityMaterial using renderer_make_sphere_mesh/renderer_make_cube_mesh in src/engine/scene_api.cpp
- [x] T010 [US1] Implement camera system: compute view matrix from inverse TRS of CameraActive entity, projection matrix via glm::perspective with aspect from sapp_widthf/sapp_heightf, push to renderer_set_camera() in src/engine/camera.h and src/engine/camera.cpp
- [x] T011 [US1] Wire render enqueue loop into engine_tick(): iterate view<Transform, Mesh, EntityMaterial> calling renderer_enqueue_draw() per entity, call camera update, upload directional light via renderer_set_directional_light() in src/engine/engine.cpp
- [x] T012 [US1] Create engine_app E-M1 driver: renderer_init → engine_init → register frame callback → spawn camera entity + light entity + 10 procedural spheres/cubes at varied positions → renderer_run() in src/engine/app/main.cpp

**Checkpoint**: engine_app displays a procedural ECS scene. Tests in test_ecs.cpp and test_math.cpp pass. E-M1 acceptance criteria met.

---

## Phase 4: User Story 2 — Load and Display glTF/OBJ Assets (Priority: P2)

**Goal**: Load 3D models from glTF/OBJ files on disk and spawn them as fully composed ECS entities alongside procedural primitives.

**Milestone Gate**: E-M2 — `engine_app` loads a glTF from `/assets/` and renders alongside procedurals.

**Independent Test**: Load a known glTF file from the assets directory; verify the mesh renders at the specified position with correct geometry.

### Implementation for User Story 2

- [x] T013 [P] [US2] Implement glTF/GLB mesh loader: cgltf_parse → cgltf_load_buffers → walk mesh primitives extracting positions/normals/UVs/indices, handle GLB vs glTF paths, resolve via ASSET_ROOT macro, log [ENGINE] warning for unsupported features (skeletal animation, morph targets) in src/engine/asset_import.h and src/engine/asset_import.cpp
- [x] T014 [P] [US2] Implement OBJ mesh fallback loader: tinyobjloader parse, extract positions/normals/UVs/indices per shape, resolve via ASSET_ROOT macro in src/engine/obj_import.h and src/engine/obj_import.cpp
- [x] T015 [US2] Implement asset bridge: convert extracted vertex arrays to renderer Vertex layout (zero-fill tangent if absent, Y-up default normal if absent), call renderer_upload_mesh(), return RendererMeshHandle in src/engine/asset_bridge.h and src/engine/asset_bridge.cpp
- [x] T016 [US2] Implement engine_spawn_from_asset(): load mesh via asset_import/obj_import, bridge to renderer handle, create entity with Transform + Mesh + EntityMaterial in src/engine/scene_api.cpp
- [x] T017 [US2] Update engine_app for E-M2: load a glTF model from assets/ directory, spawn it alongside existing procedural primitives, verify mixed rendering in src/engine/app/main.cpp

**Checkpoint**: engine_app renders glTF models alongside procedurals. Asset loading errors return {0} handle without crashing. E-M2 acceptance criteria met.

---

## Phase 5: User Story 4 — Navigate and Query the Scene (Priority: P4) — E-M3

**Goal**: Move a camera through the scene with keyboard/mouse, cast rays to identify entities, and query AABB volumes for nearby entities. Polled input state exposed to game layer.

**Milestone Gate**: E-M3 — `engine_app` navigable with WASD+mouse; raycast returns hits.

**Independent Test**: Move camera with WASD+mouse; verify raycast returns correct hit results against known entity positions.

**⚠️ NOTE**: Scheduled before US3 (Physics) because the collider system and spatial queries are prerequisites for the physics collision pipeline.

### Tests for User Story 4

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T018 [P] [US4] Write collision primitive tests: AABB-vs-AABB overlap true/false cases, world AABB computation from position + half_extents, ray-vs-AABB slab method hit/miss/distance/normal cases in src/engine/tests/test_collision.cpp

### Implementation for User Story 4

- [x] T019 [P] [US4] Implement input bridge: register InputCallback with renderer_set_input_callback(), update InputState singleton (key_states[], mouse position, mouse delta, mouse buttons) from sokol_app events in src/engine/input.h and src/engine/input.cpp
- [x] T020 [US4] Implement collider system: compute world AABB from Transform.position ± Collider.half_extents, brute-force all-pairs AABB-vs-AABB overlap detection returning collision pairs with penetration depth and contact normal (minimum-penetration SAT axis) in src/engine/collider.h and src/engine/collider.cpp
- [x] T021 [US4] Implement raycast and overlap queries: engine_raycast() via ray-vs-AABB slab method returning nearest RaycastHit (entity, point, normal, distance), engine_overlap_aabb() returning all intersecting entities in src/engine/raycast.h and src/engine/raycast.cpp
- [x] T022 [US4] Wire input callback registration into engine_init(), add mouse delta computation to engine_tick() start, expose engine_key_down/engine_mouse_delta/engine_mouse_button/engine_mouse_position in src/engine/engine.cpp
- [x] T023 [US4] Update engine_app for E-M3: WASD+mouse FPS-style camera controller, spawn 50+ collidable entities to validate SC-005 raycast scale, highlight entity under crosshair via raycast, display hit info via fprintf in src/engine/app/main.cpp

**Checkpoint**: engine_app navigable with WASD+mouse. Raycasts return correct hits. Collision tests in test_collision.cpp pass. E-M3 acceptance criteria met.

---

## Phase 6: User Story 3 — Simulate N-Body Zero-Gravity Physics with Elastic Collisions (Priority: P3) — E-M4

**Goal**: Assign RigidBody + Collider components to entities, apply initial velocities, and observe Euler-integrated motion with elastic AABB collisions in zero gravity. Energy is conserved; static obstacles are immovable.

**Milestone Gate**: E-M4 — `engine_app` shows rigid bodies colliding and bouncing elastically.

**Independent Test**: Spawn 20+ rigid-body entities with initial velocities; let simulation run; verify bodies move, collide, bounce elastically, and that static obstacles remain fixed.

### Tests for User Story 3

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T024 [P] [US3] Write physics unit tests: Euler linear integration (p += v*dt), angular integration with quaternion renormalization, force→acceleration, impulse→velocity, elastic two-body collision energy conservation (within 5% over 10s), static body zero-velocity-change, dt-cap clamp behavior in src/engine/tests/test_physics.cpp

### Implementation for User Story 3

- [ ] T025 [US3] Implement Euler integration and force API: linear integration (p += v*dt, v += F/m*dt), angular integration (ω += I⁻¹*τ*dt, q += 0.5*ω*q*dt then renormalize), world-space inertia recomputation (R*I_body_inv*Rᵀ), apply_force/apply_impulse/apply_impulse_at_point writing to per-entity ForceAccum map, fixed-timestep substep loop with 120 Hz accumulator and dt-cap in src/engine/physics.h and src/engine/physics.cpp
- [ ] T026 [US3] Implement collision response: impulse-based elastic response with hardcoded restitution = 1.0 (configurable restitution is post-MVP), handle dynamic-dynamic and dynamic-static (inv_mass=0) pairs, Baumgarte positional correction (k_slop=0.005, k_baumgarte=0.3), clear force accumulators after each substep in src/engine/collision_response.h and src/engine/collision_response.cpp
- [ ] T027 [US3] Wire complete physics pipeline into engine_tick(): fixed-timestep substep loop containing force accumulation → Euler integration → world inertia update → AABB collision detection (from collider.h) → impulse response → Baumgarte correction → force clear, running before camera/render enqueue in src/engine/engine.cpp
- [ ] T028 [US3] Update engine_app for E-M4: spawn 20+ dynamic rigid bodies with random initial velocities + 4 static obstacle walls, demonstrate elastic collisions in zero gravity, display simulation running at ≥30 FPS in src/engine/app/main.cpp

**Checkpoint**: 20+ rigid bodies collide elastically in zero gravity. Energy conservation within 5% tolerance. Static obstacles immovable. Physics tests in test_physics.cpp pass. E-M4 acceptance criteria met.

---

## Phase 7: User Story 5 — Game Builds Interaction on Top of the Engine API (Priority: P5)

**Goal**: Ensure the engine's public API is complete and stable enough for the game layer to build a full space-shooter experience — custom components, direct registry access, lifecycle calls, input, physics, and queries — all without engine source modifications.

**Independent Test**: Build a minimal game loop using only engine.h that spawns entities, applies physics, reads input, and responds to collisions, producing interactive behavior.

### Implementation for User Story 5

- [x] T029 [US5] Finalize engine.h public header: verify all template component operations compile from an external translation unit, ensure engine_registry() exposes direct entt::registry& for game-layer view<> iteration, add any missing declarations in src/engine/engine.h
- [x] T030 [US5] Update engine mock stubs to cover full finalized API surface (all new functions from Phases 3–6) in src/engine/mocks/engine_mock.cpp
- [x] T031 [US5] Promote engine-api.md contract to frozen interface spec: copy to docs/interfaces/engine-interface-spec.md, add FROZEN version marker, update status from DRAFT

**Checkpoint**: Game workstream can include engine.h, link against engine static lib (or mocks), and use all public API functions. Interface spec frozen.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, performance verification, and coordination updates.

- [x] T032 [P] Run all Catch2 engine tests and verify 100% pass: cmake --build build --target engine_tests && ./build/engine_tests
- [x] T033 [P] Verify engine_app runs at ≥30 FPS with 100 active rigid bodies on target hardware via ./build/engine_app
- [x] T034 Run full build verification (all workstreams): cmake --build build returns 0 with no engine-originated warnings
- [x] T035 [P] Update _coordination/engine/PROGRESS.md with milestone completion status

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 (components.h, engine.h exist) — BLOCKS all user stories
- **US1 / E-M1 (Phase 3)**: Depends on Phase 2 — first deliverable milestone
- **US2 / E-M2 (Phase 4)**: Depends on Phase 3 (scene API, render enqueue loop exist)
- **US4 / E-M3 (Phase 5)**: Depends on Phase 3 (scene API exists); independent of Phase 4
- **US3 / E-M4 (Phase 6)**: Depends on Phase 5 (collider system required for collision detection)
- **US5 (Phase 7)**: Depends on all prior phases (API surface finalized)
- **Polish (Phase 8)**: Depends on all user stories being complete

### User Story Dependencies

- **US1 (P1)**: Can start after Foundational (Phase 2) — no dependencies on other stories
- **US2 (P2)**: Depends on US1 (needs scene_api.cpp, render enqueue in engine_tick)
- **US4 (P4)**: Depends on US1 (needs scene_api.cpp, engine_init); independent of US2
- **US3 (P3)**: Depends on US4 (needs collider system from collider.h/collider.cpp for collision detection)
- **US5 (P5)**: Depends on US1 + US2 + US3 + US4 (complete API surface)

### Milestone Execution Order

```
E-M1 (Phase 3) → E-M2 (Phase 4) ─┐
                                    ├→ E-M4 (Phase 6) → US5 (Phase 7) → Polish (Phase 8)
E-M1 (Phase 3) → E-M3 (Phase 5) ─┘
```

### Within Each User Story

- Tests MUST be written and FAIL before implementation (where tests exist)
- Header declarations before implementations
- Core logic before wiring into engine_tick()
- engine_app update is the last task per phase (milestone acceptance vehicle)

### Parallel Opportunities

**Phase 1 (Setup)**:
- T002 (engine.h) and T003 (time system) can run in parallel — different files

**Phase 2 (Foundational)**:
- T005 (mocks) can run in parallel with T004 (engine.cpp) — different files

**Phase 3 (US1)**:
- T006 (test_ecs.cpp) and T007 (test_math.cpp) can run in parallel — different files
- After tests: T008 (scene_api), T010 (camera) touch different files but T009 depends on T008

**Phase 4 (US2)**:
- T013 (glTF loader) and T014 (OBJ loader) can run in parallel — different files
- T015 (asset bridge) depends on T013/T014

**Phase 5 (US4)**:
- T018 (test_collision.cpp) and T019 (input bridge) can run in parallel — different files
- T020 (collider) and T021 (raycast) touch different files but raycast depends on collider types

**Phase 6 (US3)**:
- T024 (test_physics.cpp) can run in parallel with early T025 setup

**Phase 8 (Polish)**:
- T032 (tests), T033 (perf), T035 (progress) can all run in parallel

---

## Parallel Example: User Story 1 (Phase 3)

```bash
# Launch tests in parallel (write-fail-first):
Task T006: "Write ECS lifecycle tests in src/engine/tests/test_ecs.cpp"
Task T007: "Write transform/matrix math tests in src/engine/tests/test_math.cpp"

# Then launch independent implementation files:
Task T008: "Implement scene API in src/engine/scene_api.h + scene_api.cpp"
Task T010: "Implement camera system in src/engine/camera.h + camera.cpp"
# (T009 spawners depend on T008 scene_api; T011 depends on T008+T010; T012 depends on all)
```

## Parallel Example: User Story 2 (Phase 4)

```bash
# Launch independent loaders in parallel:
Task T013: "Implement glTF/GLB loader in src/engine/asset_import.h + asset_import.cpp"
Task T014: "Implement OBJ loader in src/engine/obj_import.h + obj_import.cpp"
# (T015 asset bridge depends on both; T016 depends on T015; T017 depends on T016)
```

---

## Implementation Strategy

### MVP First (E-M1 — User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL — blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Run engine_app, verify 10+ procedural primitives rendered via ECS
5. Run engine_tests — all math and ECS tests pass
6. Human behavioral check on engine_app → E-M1 merge

### Incremental Delivery (Milestone Per Phase)

1. Setup + Foundational → Engine compiles, mocks available for game workstream
2. Add US1 → E-M1 merge (procedural ECS scene)
3. Add US2 → E-M2 merge (asset import)
4. Add US4 → E-M3 merge (input + colliders + raycasting)
5. Add US3 → E-M4 merge (Euler physics + elastic collisions)
6. Finalize US5 → Interface freeze, game workstream unblocked
7. Each milestone is independently demonstrable and merge-safe

### Key Build Commands

```bash
# Iterative build (engine workstream only):
cmake --build build --target engine_app engine_tests

# Run tests:
./build/engine_tests

# Run driver:
./build/engine_app

# Full milestone-sync rebuild:
cmake --build build

# Build with renderer mocks (before real renderer is ready):
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DUSE_RENDERER_MOCKS=ON
cmake --build build --target engine_app engine_tests
```

---

## Notes

- [P] tasks = different files, no dependencies on incomplete tasks in the same phase
- [Story] label maps task to specific user story for traceability
- Each milestone (E-M1 through E-M4) should be independently completable and demonstrable
- US4 (P4) is scheduled before US3 (P3) due to technical dependency (colliders needed for physics)
- All asset paths resolved via ASSET_ROOT macro from cmake/paths.h.in — never hard-code relative paths
- Shaders are precompiled by sokol-shdc — the engine does not compile or load shaders at runtime
- The engine does NOT own the main loop — it ticks from inside renderer's frame callback
- Engine includes renderer.h (FROZEN v1.0) for handle types, Material, Vertex, and all renderer_* functions
- Logging: fprintf(stderr, "[ENGINE] ...") for all warnings and errors
- Entity destruction is always deferred via DestroyPending tag to avoid mid-iteration UB
