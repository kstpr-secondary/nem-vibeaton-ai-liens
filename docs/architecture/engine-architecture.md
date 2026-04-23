# Engine Architecture

> **Status:** Draft — to be populated during Engine SpecKit planning cycle.  
> **Upstream**: Renderer architecture (`renderer-architecture.md`, FROZEN).

---

## Overview

The game engine is a C++17 static library providing ECS-based scene management, asset import, physics, collision, and input routing. It consumes the renderer public API and is consumed by the game executable.

This document will be populated by the Engine SpecKit `plan.md` output with:

- Module decomposition and file map
- ECS component registry design
- Asset import pipeline (cgltf/tinyobjloader → renderer mesh upload)
- Physics integration architecture (Euler, AABB, collision response)
- Input routing flow (sokol_app → renderer → engine → game)
- Frame lifecycle (engine_tick inside renderer frame callback)
- Milestone-to-module mapping

---

## Seed Architecture (from concept doc)

### Integration with Renderer

```
sokol_app main loop (owned by renderer)
    └─► renderer frame_callback()
            ├─ [engine tick(dt) if registered]
            │       ├─ physics step (Euler integration)
            │       ├─ collision detection + response
            │       ├─ game tick callback
            │       ├─ renderer_set_camera()
            │       ├─ renderer_set_directional_light()
            │       ├─ renderer_enqueue_draw() × N
            │       └─ ImGui widget calls (game HUD)
            └─ renderer_end_frame()
```

### Key Dependencies

| Library | Role | Version |
|---|---|---|
| entt | ECS registry, views, entity lifecycle | v3.13.2 |
| cgltf | glTF/GLB asset parsing | v1.15 |
| tinyobjloader | OBJ fallback loading | v2.0.0rc13 |
| GLM | Math (vec3, mat4, quat) | 1.0.1 |
| renderer (static lib) | GPU resource creation, draw submission | Internal |

### Milestones (from concept doc)

| ID | Name | Key Deliverables |
|---|---|---|
| E-M1 | Bootstrap + ECS + Scene API | entt registry, components, entity CRUD, scene tick |
| E-M2 | Asset Import | cgltf glTF loader, tinyobjloader OBJ loader, mesh upload bridge |
| E-M3 | Input + Camera | Input polling/events, camera entity → view/proj, active camera |
| E-M4 | Euler Physics + Collision | RigidBody integration, AABB detection, impulse response |
| E-M5 | Raycasting + Queries | Ray-vs-AABB, overlap queries, hit results |
| E-M6 | Point Lights (Desirable) | Multiple point light support |
| E-M7 | Steering AI (Desirable) | Seek + obstacle avoidance behaviors |
