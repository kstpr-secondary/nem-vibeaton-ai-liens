# Implementation Plan: ECS Physics Engine

**Branch**: `002-ecs-physics-engine` | **Date**: 2026-04-23 | **Spec**: [specs/002-ecs-physics-engine/spec.md](./spec.md)
**Input**: Feature specification from `/specs/002-ecs-physics-engine/spec.md`

## Summary

Build a C++17 game engine static library (`engine`) providing ECS-based scene management (entt), asset import (cgltf/tinyobjloader), Euler rigid-body physics with AABB collision and elastic impulse response, spatial queries (ray/AABB overlap), input routing, and camera computation. The engine ticks from inside the renderer's frame callback, consumes the frozen renderer public API for all GPU operations, and exposes a thin public API for the game layer. The engine owns the single `entt::registry` and all component definitions; the game layer accesses components through re-exported entt views. MVP milestones E-M1 through E-M4 deliver the full simulation stack; desirable milestones E-M5 through E-M7 add steering AI, point lights, and capsule mesh integration.

## Technical Context

**Language/Version**: C++17 — `-Wall -Wextra -Wpedantic`, `-Werror` off  
**Primary Dependencies**: entt v3.x (ECS), cgltf v1.15 (glTF/GLB import), tinyobjloader v2.0.0rc13 (OBJ fallback), GLM 1.x (vec3, mat4, quat math), renderer static lib (frozen v1.0), sokol_time (timer), Catch2 v3 (unit tests)  
**Storage**: N/A — in-memory ECS component pools via entt; GPU resources via opaque renderer handles; no disk persistence  
**Testing**: Catch2 v3 for engine math (transforms, matrix composition), physics (Euler integration, impulse formula, energy conservation), collision primitives (AABB overlap, ray-vs-AABB slab method), ECS lifecycle (entity create/destroy, component attach/detach). Rendering correctness via human behavioral check on `engine_app`  
**Target Platform**: Ubuntu 24.04 LTS, desktop Linux; OpenGL 3.3 Core via renderer  
**Project Type**: Static library (`engine`) + standalone driver executable (`engine_app`) + Catch2 test executable (`engine_tests`)  
**Performance Goals**: ≥30 FPS with 100 active rigid bodies on target hardware; physics substep at 120 Hz; raycast queries resolve within one frame for 50+ collidable entities  
**Constraints**: ~3h MVP window (E-M1→E-M4); brute-force O(N²) collision for ≤200 entities; AABB-only colliders; single directional light; no animation/networking/audio  
**Scale/Scope**: ≤200 entities, ~10 unique meshes, 20+ collidable rigid bodies, 1 camera, 1 directional light (MVP)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate | Status | Evidence |
|------|--------|---------|
| **I. Behavioral Correctness** | ✅ PASS | Every FR maps to acceptance scenarios (SC-001→SC-008); each milestone gated by human behavioral check on `engine_app`; physics correctness verified by energy-conservation Catch2 tests |
| **II. Milestone-Driven Integration** | ✅ PASS | E-M1→E-M4 are discrete sync points; engine starts against renderer mocks (USE_RENDERER_MOCKS=ON) and frozen renderer-interface-spec.md v1.0; game workstream starts against engine mocks (USE_ENGINE_MOCKS=ON) |
| **III. Speed Over Maintainability** | ✅ PASS | Brute-force O(N²) collision, no spatial partitioning, no resource pools, no RAII wrappers over entt, inline force accumulators, AABB-only geometry — all biased toward fast delivery |
| **IV. AI-Generated Under Human Supervision** | ✅ PASS | All C++ AI-generated; human approves interface freeze, runs behavioral checks, signs off on each milestone merge |
| **V. Minimal Fixed Stack** | ✅ PASS | All deps via CMake FetchContent with pinned versions; zero new libraries; only entt, cgltf, tinyobjloader, GLM, renderer as dependencies |

*Post-Phase-1 re-check: identical verdicts — no design decisions violate the constitution.*

## Project Structure

### Documentation (this feature)

```text
specs/002-ecs-physics-engine/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output → promoted to docs/interfaces/engine-interface-spec.md
└── tasks.md             # Phase 2 output (speckit.tasks command)
```

### Source Code (repository root)

```text
src/engine/
├── engine.h                  # Public API header — types, components, free functions
├── engine.cpp                # Core: init / shutdown / tick lifecycle
├── components.h              # ECS component struct definitions (Transform, RigidBody, etc.)
├── scene_api.cpp             # Entity CRUD, procedural spawners, spawn_from_asset
├── scene_api.h
├── asset_import.cpp          # cgltf glTF/GLB loader (CGLTF_IMPLEMENTATION here)
├── asset_import.h
├── obj_import.cpp            # tinyobjloader OBJ fallback
├── obj_import.h
├── asset_bridge.cpp          # Vertex layout conversion → renderer upload_mesh
├── asset_bridge.h
├── input.cpp                 # Input polling state + event callback bridge
├── input.h
├── collider.cpp              # AABB-vs-AABB overlap, world AABB computation
├── collider.h
├── raycast.cpp               # Ray-vs-AABB slab method, overlap_aabb query
├── raycast.h
├── physics.cpp               # Euler integration (linear + angular), force/impulse API
├── physics.h
├── collision_response.cpp    # Impulse-based elastic collision response
├── collision_response.h
├── time.cpp                  # sokol_time wrappers: now(), delta_time()
├── time.h
├── camera.cpp                # Camera entity → view/projection matrix computation
├── camera.h
├── app/
│   └── main.cpp              # engine_app driver — procedural ECS demo, updated each milestone
├── mocks/
│   └── engine_mock.cpp       # Mock stubs for game workstream (USE_ENGINE_MOCKS=ON)
└── tests/
    ├── test_math.cpp          # Transform, matrix composition tests
    ├── test_physics.cpp       # Euler integration, energy conservation
    ├── test_collision.cpp     # AABB overlap, ray-vs-AABB slab
    └── test_ecs.cpp           # Entity lifecycle, component attach/detach
```

**Structure Decision**: Single C++ static-lib workstream following the existing repo layout. `engine_app` is the standalone driver and milestone-acceptance vehicle. Tests live under `src/engine/tests/` (per repo convention). No game or renderer source is touched by this workstream. The engine depends on the renderer via its frozen public header only.

## Complexity Tracking

> No constitution violations — table omitted.
