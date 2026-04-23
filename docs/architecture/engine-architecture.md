# Engine Architecture

> **Status:** Populated from Engine SpecKit (`specs/002-ecs-physics-engine/plan.md`).  
> **Upstream**: Renderer architecture (`renderer-architecture.md`, FROZEN). Renderer interface (`renderer-interface-spec.md`, FROZEN v1.1).

---

## Overview

The game engine is a C++17 static library providing ECS-based scene management, asset import, 6-DOF Euler physics with angular integration, AABB collision with elastic impulse response, raycasting, input routing, and a thin gameplay API. It consumes the renderer public API and is consumed by the game executable.

---

## Integration with Renderer

```
sokol_app main loop (owned by renderer)
    └─► renderer internal frame_callback()
            ├─ simgui_new_frame()
            ├─ call registered FrameCallback(dt, user_data)
            │       ├─ renderer_begin_frame()
            │       ├─ engine_tick(dt)
            │       │       ├─ input state update
            │       │       ├─ physics substep loop (120 Hz):
            │       │       │       ├─ force accumulation
            │       │       │       ├─ linear + angular Euler integration
            │       │       │       ├─ world inertia recomputation
            │       │       │       ├─ AABB collision detection (brute-force)
            │       │       │       ├─ impulse-based elastic response
            │       │       │       ├─ Baumgarte positional correction
            │       │       │       └─ force accumulators cleared
            │       │       ├─ camera view/proj → renderer_set_camera()
            │       │       ├─ light → renderer_set_directional_light()
            │       │       ├─ renderer_enqueue_draw() × N
            │       │       └─ DestroyPending sweep
            │       ├─ ImGui widget calls (game HUD)
            │       └─ renderer_end_frame()
            └─ (renderer post-frame: skybox, opaque, transparent, simgui_render)
```

### Key Dependencies

| Library | Role | Version |
|---|---|---|
| entt | ECS registry, views, entity lifecycle | v3.13.2 |
| cgltf | glTF/GLB asset parsing | v1.15 |
| tinyobjloader | OBJ fallback loading | v2.0.0rc13 |
| GLM | Math (vec3, mat4, quat) | 1.0.1 |
| renderer (static lib) | GPU resource creation, draw submission | FROZEN v1.1 |

---

## Module Map

| File | Responsibility | Milestone |
|------|---------------|-----------|
| `engine.h` | Public API header — all types + declarations | E-M1 |
| `components.h` | ECS component struct definitions | E-M1 |
| `engine.cpp` | Core lifecycle: init / shutdown / tick | E-M1 |
| `scene_api.h/cpp` | Entity CRUD, procedural spawners, spawn_from_asset | E-M1 |
| `camera.h/cpp` | Camera entity → view/projection computation | E-M1 |
| `time.h/cpp` | sokol_time wrappers: now(), delta_time() | E-M1 |
| `asset_import.h/cpp` | cgltf glTF/GLB loader | E-M2 |
| `obj_import.h/cpp` | tinyobjloader OBJ fallback | E-M2 |
| `asset_bridge.h/cpp` | Vertex layout conversion → renderer upload_mesh | E-M2 |
| `input.h/cpp` | Input polling state + event callback bridge | E-M3 |
| `collider.h/cpp` | AABB-vs-AABB overlap, world AABB computation | E-M3 |
| `raycast.h/cpp` | Ray-vs-AABB slab method, overlap_aabb query | E-M3 |
| `physics.h/cpp` | Euler integration (linear + angular), force/impulse API | E-M4 |
| `collision_response.h/cpp` | Impulse-based elastic collision response | E-M4 |
| `app/main.cpp` | engine_app driver — updated each milestone | E-M1+ |
| `mocks/engine_mock.cpp` | Mock stubs (USE_ENGINE_MOCKS=ON) | E-M1 |
| `tests/test_ecs.cpp` | Entity lifecycle, component attach/detach | E-M1 |
| `tests/test_math.cpp` | Transform, matrix composition | E-M1 |
| `tests/test_collision.cpp` | AABB overlap, ray-vs-AABB slab | E-M3 |
| `tests/test_physics.cpp` | Euler integration, energy conservation | E-M4 |

---

## Milestones

### MVP

| ID | Name | Key Deliverables | Expected Outcome |
|---|---|---|---|
| E-M1 | Bootstrap + ECS + Scene API | entt registry, components, entity CRUD, procedural spawners, camera, scene tick, render enqueue | `engine_app` displays procedural spheres/cubes via ECS-driven renderer enqueue |
| E-M2 | Asset Import | cgltf glTF/GLB loader, tinyobjloader OBJ loader, vertex bridge, mesh upload | `engine_app` loads a glTF from `/assets/` and renders alongside procedurals |
| E-M3 | Input + AABB Colliders + Raycasting | Input polling/events, AABB collision detection, ray-vs-AABB queries, overlap queries | `engine_app` navigable with WASD+mouse; raycast returns hits |
| E-M4 | Euler Physics + Collision Response | 6-DOF Euler integration (linear+angular), impulse-based elastic response, fixed 120Hz substep, Baumgarte correction | `engine_app` shows rigid bodies colliding and bouncing elastically |

### Desirable

| ID | Name | Key Deliverables |
|---|---|---|
| E-M5 | Enemy Steering AI | Seek behavior, raycast-ahead obstacle avoidance |
| E-M6 | Multiple Point Lights | Point Light component, upload up to N lights per frame |
| E-M7 | Capsule Mesh Integration | Consume renderer `make_capsule_mesh` in spawn helpers |

### Milestone Execution Order

```
E-M1 → E-M2 ─────────────┐
                           ├→ E-M4 → US5 (API freeze) → Polish
E-M1 → E-M3 ─────────────┘
```

E-M2 and E-M3 are independent after E-M1; both must complete before E-M4.

---

## Physics Architecture

- **Integration**: Semi-implicit Euler, 6-DOF (linear + angular)
- **Substep**: Fixed 120 Hz accumulator with dt-cap (100ms max)
- **Collision geometry**: AABB-only (MVP); rotation/scale ignored for collision
- **Detection**: Brute-force all-pairs O(N²), acceptable for ≤200 entities
- **Response**: Impulse-based elastic (restitution=1.0), min-penetration SAT normal
- **Correction**: Baumgarte stabilization (k_slop=0.005, k_baumgarte=0.3)
- **Inertia**: Uniform-density box approximation from AABB half-extents
- **Forces**: Per-entity ForceAccum map (transient, cleared each substep)
- **Gravity**: None (zero-g space environment; forces applied explicitly by game)
