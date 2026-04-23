# Engine Quickstart

> **Phase 1 output** for `002-ecs-physics-engine`. Build, run, and iterate guide for the engine workstream.

---

## Prerequisites

- Ubuntu 24.04 LTS
- Clang, CMake ≥ 3.19, Ninja
- `sokol-shdc` binary on `PATH` (for renderer shader compilation)
- FetchContent cache warmed (dependencies pre-downloaded)

---

## One-Time Configure

From the repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

---

## Build Commands

### Engine workstream iteration (do NOT rebuild unrelated targets):

```bash
cmake --build build --target engine_app engine_tests
```

### Run the engine driver:

```bash
./build/engine_app
```

### Run engine unit tests:

```bash
./build/engine_tests
```

### Full milestone-sync rebuild (only after a milestone merge):

```bash
cmake --build build
```

---

## Mock Modes

### Build game against engine mocks (when real engine not yet ready):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DUSE_ENGINE_MOCKS=ON
cmake --build build --target game
```

### Build engine against renderer mocks (when real renderer not yet ready):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DUSE_RENDERER_MOCKS=ON
cmake --build build --target engine_app engine_tests
```

### Swap to real implementations:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DUSE_ENGINE_MOCKS=OFF -DUSE_RENDERER_MOCKS=OFF
cmake --build build
```

---

## Key File Locations

| File | Purpose |
|------|---------|
| `src/engine/engine.h` | Public API header — game and engine_app include only this |
| `src/engine/components.h` | ECS component definitions (Transform, RigidBody, etc.) |
| `src/engine/app/main.cpp` | `engine_app` driver — procedural demo, updated each milestone |
| `src/engine/mocks/engine_mock.cpp` | Mock stubs for game workstream |
| `src/engine/tests/` | Catch2 test files |
| `docs/interfaces/engine-interface-spec.md` | Frozen interface contract (post-freeze) |
| `_coordination/engine/TASKS.md` | Live task board |

---

## Milestone Acceptance Flow

1. Implement the milestone's tasks per `_coordination/engine/TASKS.md`.
2. Build: `cmake --build build --target engine_app engine_tests` — must return 0.
3. Run tests: `./build/engine_tests` — all must pass.
4. Run driver: `./build/engine_app` — visually confirm the Expected Outcome.
5. Mark tasks `READY_FOR_VALIDATION` / `READY_FOR_REVIEW` in `TASKS.md`.
6. Queue validation/review via `_coordination/queues/`.
7. Human behavioral check on `engine_app`.
8. Human merges feature branch.

---

## Engine Milestones (MVP)

| ID | Name | Expected Outcome |
|----|------|-----------------|
| E-M1 | Bootstrap + ECS + Scene API | `engine_app` displays procedural spheres/cubes via ECS-driven renderer enqueue |
| E-M2 | Asset Import | `engine_app` loads a glTF from `/assets/` and renders alongside procedurals |
| E-M3 | Input + Colliders + Raycasting | `engine_app` navigable with WASD+mouse; raycast returns hits |
| E-M4 | Euler Physics + Collision | `engine_app` shows rigid bodies colliding and bouncing elastically |

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `engine_app` shows nothing | Check `engine_tick` is called between `begin_frame` / `end_frame`; verify entities have Transform + Mesh + MaterialComp |
| Physics bodies tunnel through walls | Verify fixed-timestep substep loop is active (120 Hz); check DT_CAP |
| `[ENGINE] Failed to load` asset | Verify `ASSET_ROOT` macro resolves correctly; check asset exists at path |
| Crash on `get_component` | Use `try_get_component` — UB if component absent without check |
| Camera not working | Ensure exactly one entity has `CameraActive` tag; verify `set_active_camera()` was called |
