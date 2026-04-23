# Cross-Workstream Architecture

> **Status:** Living document — renderer section frozen; engine and game sections populated when their SpecKit runs complete.  
> For workstream-local detail, see the per-workstream architecture docs below.

---

## Overview

Three C++17 static libraries and executables built in one repo:

| Target | Kind | Owns | Depends On |
|---|---|---|---|
| `renderer` | static lib | Window, GPU context, sokol_app loop, input pump, shaders, pipelines, Dear ImGui | sokol_gfx, sokol_app, sokol_time, GLM, Dear ImGui, stb_image |
| `renderer_app` | executable | Standalone renderer demo; acceptance vehicle for renderer milestones | `renderer` |
| `engine` | static lib | ECS (entt), asset import (cgltf/tinyobjloader), Euler physics, AABB, raycasting | `renderer`, entt, cgltf, tinyobjloader, GLM |
| `engine_app` | executable | Standalone engine demo; acceptance vehicle for engine milestones | `engine`, `renderer` |
| `game` | executable | 3D space shooter gameplay | `engine`, `renderer` |
| `renderer_tests` | executable | Catch2 unit tests for renderer math/builders | `renderer`, Catch2 |
| `engine_tests` | executable | Catch2 unit tests for engine math/physics/ECS | `engine`, Catch2 |

**Integration flow:** Renderer is the foundation. Engine ticks from inside the renderer's frame callback. Game calls only engine + renderer public APIs.

---

## System Integration Diagram

```
sokol_app main loop (owned by renderer)
    │
    └─► renderer frame_callback()
            ├─ simgui_new_frame()
            ├─ [engine tick() if registered]          ← engine calls:
            │       renderer_set_camera()
            │       renderer_set_directional_light()
            │       renderer_set_skybox()
            │       renderer_enqueue_draw() × N
            │       renderer_enqueue_line_quad() × M
            │       ImGui::* widget calls              ← game emits HUD here
            └─ renderer_end_frame()
                    ├─ pass 0: skybox
                    ├─ pass 1: opaque draws (Unlit + Lambertian + BlinnPhong)
                    ├─ pass 2: transparent draws + line quads
                    └─ pass 3: simgui_render() (ImGui overlay)
```

---

## Renderer — Architecture Summary

> Full detail: `docs/architecture/renderer-architecture.md` (FROZEN)

- **Owns**: `sokol_app` init, main frame callback, Dear ImGui, all GPU pipeline creation, public `renderer.h` API.
- **Does not own**: scene graph, entity lifecycle, physics, asset parsing.
- **Shaders**: annotated `.glsl` under `shaders/renderer/`, precompiled by `sokol-shdc` at build time into `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. No runtime GLSL loading.
- **Error handling**: any shader/pipeline creation failure → magenta error pipeline (logged, no crash).
- **Asset paths**: runtime-loaded assets compose paths from `ASSET_ROOT` (generated `paths.h`). Shaders need no runtime path.
- **Dear ImGui**: renderer owns setup/shutdown/forwarding; game code only emits widget calls.

### Renderer Milestones

| Milestone | Scope | Tier |
|---|---|---|
| R-M0 | Bootstrap: window, clear color, input pump | MVP gate |
| R-M1 | Unlit forward rendering + procedural shapes | **Engine sync point; interface freeze** |
| R-M2 | Directional light + Lambertian shading | MVP |
| R-M3 | Skybox + line-quads + alpha blending | **MVP complete** |
| R-M4 | Blinn-Phong + diffuse textures | Desirable |
| R-M5 | Sorted transparency + capsule mesh | Desirable |
| R-M6 | Frustum culling + front-to-back sort | Desirable (conditional on perf need) |

---

## Game Engine — Architecture Summary

> Full detail: `docs/architecture/engine-architecture.md` (populated after Engine SpecKit)

- **Owns**: ECS registry (`entt`), asset import bridge, game loop tick, input callbacks, Euler physics, AABB collision, raycasting.
- **Does not own**: window/context, shaders, GPU resource creation (all delegated to renderer public API).
- **Integration**: engine tick runs inside renderer's frame callback. Mesh uploads go through `renderer_upload_mesh()`.
- **Starts from**: frozen `renderer.h` + `USE_RENDERER_MOCKS=ON` stubs; swaps to real renderer at R-M1 merge.

### Engine Milestones

| Milestone | Scope | Tier |
|---|---|---|
| E-M1 | Bootstrap + ECS + Scene API | **Game sync point** |
| E-M2 | Asset import (glTF/OBJ → mesh upload bridge) | MVP |
| E-M3 | Input + AABB colliders + raycasting | MVP |
| E-M4 | Euler physics + collision response | **MVP complete** |
| E-M5 | Enemy steering AI | Desirable |
| E-M6 | Multiple point lights | Desirable |
| E-M7 | Capsule mesh integration | Desirable (depends R-M5) |

---

## Game — Architecture Summary

> Full detail: `docs/architecture/game-architecture.md` (populated after Game SpecKit)

- **Owns**: player flight controller, third-person camera rig, asteroid field + containment, weapon systems (laser raycast + plasma projectile), HP/shields/boost, enemy AI, Dear ImGui HUD, restart/win/quit flow.
- **Does not own**: renderer internals, engine ECS internals.
- **Starts from**: frozen `renderer.h` + `engine.h` interfaces + mocks; swaps real implementations at milestone merges.

### Game Milestones

| Milestone | Scope | Tier |
|---|---|---|
| G-M1 | Flight controller + procedural scene + camera | Starts T+0 from mocks |
| G-M2 | Physics + containment + asteroid dynamics | Swaps engine physics mock at E-M4 |
| G-M3 | Weapons + enemies + HP/Shield | Requires R-M3 + E-M3 + E-M4 |
| G-M4 | HUD + game flow + restart | **MVP complete** |
| G-M5 | AI upgrade + scaling | Desirable |
| G-M6 | Visual polish | Desirable |

---

## Cross-Workstream Dependencies

| Downstream milestone | Requires renderer at | Requires engine at |
|---|---|---|
| E-M1 | R-M1 (interface frozen) | — |
| E-M2 | R-M1 | E-M1 |
| E-M3 | R-M1 | E-M1 |
| E-M4 | R-M1 | E-M3 |
| G-M1 | R-M1 | E-M1 |
| G-M2 | R-M1 | E-M4 (or mocks) |
| G-M3 | R-M1 + R-M3 | E-M3 + E-M4 |
| G-M4 | R-M1 + renderer ImGui | E-M1 |

---

## Shared Technology Decisions

- **Graphics backend**: OpenGL 3.3 Core Profile only. Vulkan removed.
- **Build**: CMake + Ninja + Clang; FetchContent only; no Conan/vcpkg.
- **Shaders**: `sokol-shdc` annotated GLSL; precompiled; no runtime GLSL.
- **ECS**: `entt` v3.x.
- **Asset formats**: glTF/GLB primary; OBJ fallback. Max two formats.
- **Testing**: Catch2 v3 for math/parsers/ECS logic; rendering correctness via human behavioral check.
- **Vertex layout** (universal, all tiers): `float3 position`, `float3 normal`, `float2 uv`, `float3 tangent`.
- **Draw queue**: fixed pre-allocated 1024 slots; overflow dropped + logged.
- **Resource lifetime**: all GPU resources held until `renderer_shutdown()`; no per-handle destroy API.

---

## Per-Workstream Architecture Documents

| Workstream | File | Status |
|---|---|---|
| Renderer | `docs/architecture/renderer-architecture.md` | **FROZEN** |
| Engine | `docs/architecture/engine-architecture.md` | Pending Engine SpecKit |
| Game | `docs/architecture/game-architecture.md` | Pending Game SpecKit |
