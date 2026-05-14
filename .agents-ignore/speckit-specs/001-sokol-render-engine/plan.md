# Implementation Plan: Sokol Render Engine

**Branch**: `001-sokol-render-engine` | **Date**: 2026-04-23 | **Spec**: [specs/001-sokol-render-engine/spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-sokol-render-engine/spec.md`

## Summary

Build a forward-rendered C++17 graphics library (static lib `renderer` + standalone driver `renderer_app`) on top of `sokol_gfx`/`sokol_app`/`sokol_time`, with OpenGL 3.3 Core as the sole backend on Ubuntu 24.04 LTS. The renderer owns the application main loop, window, input forwarding, Dear ImGui wiring, and the public draw-submission API (enqueue_draw / enqueue_line_quad / set_camera / set_directional_light / set_skybox). Shaders are precompiled at build time via `sokol-shdc`. The frozen public header feeds the engine workstream through a mock-swap path. MVP milestones R-M0 through R-M3 must land within 3 hours; desirable milestones R-M4 through R-M6 fill remaining hackathon time.

## Technical Context

**Language/Version**: C++17 — `-Wall -Wextra -Wpedantic`, `-Werror` off  
**Primary Dependencies**: sokol_gfx, sokol_app, sokol_time (single-header FetchContent), sokol-shdc (build-time shader compiler), GLM 1.x, Dear ImGui (renderer-owned via sokol util), stb_image (FetchContent, PUBLIC include path), Catch2 v3  
**Storage**: N/A — all GPU resources (meshes, textures, pipelines) held until `shutdown()`; no per-handle destroy API  
**Testing**: Catch2 v3 for pure math/logic units (mesh builder vertex counts, matrix ops); human behavioral check for all rendering correctness  
**Target Platform**: Ubuntu 24.04 LTS, desktop Linux, OpenGL 3.3 Core Profile  
**Project Type**: Static library (`renderer`) + standalone driver executable (`renderer_app`)  
**Performance Goals**: Subjective "feels smooth" at 60 fps; supports up to **1024 draw calls per frame** (fixed pre-allocated queue); overflow silently dropped + logged  
**Constraints**: 3-hour MVP window; R-M0 bootstrap on all machines within 45 min; no audio/networking/skeletal animation/PBR/deferred rendering  
**Scale/Scope**: ~10 unique meshes, ~5 textures, 1024 draw commands/frame max; single directional light; one active camera per frame

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate | Status | Evidence |
|------|--------|---------|
| **I. Behavioral Correctness** | ✅ PASS | Every FR maps to acceptance scenarios; each milestone gated by human behavioral check on `renderer_app` |
| **II. Milestone-Driven Integration** | ✅ PASS | R-M0→R-M3 are discrete sync points; engine workstream starts from frozen interface + mocks only after R-M1 freeze |
| **III. Speed Over Maintainability** | ✅ PASS | Fixed-size draw queue, no scene graph, no RAII resource pools, no extensibility hooks beyond the public API |
| **IV. AI-Generated Under Human Supervision** | ✅ PASS | All C++ AI-generated; human approves frozen interface, runs behavioral checks, signs off on each milestone |
| **V. Minimal Fixed Stack** | ✅ PASS | All deps via CMake FetchContent with pinned versions; zero new libraries; Vulkan removed; runtime GLSL loading cut |

*Post-Phase-1 re-check: identical verdicts — no design decisions violate the constitution.*

## Project Structure

### Documentation (this feature)

```text
specs/001-sokol-render-engine/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output (already exists in docs/planning/speckit/renderer/)
├── contracts/           # Phase 1 output → promoted to docs/interfaces/renderer-interface-spec.md
└── tasks.md             # Phase 2 output (speckit.tasks command)
```

### Source Code (repository root)

```text
src/renderer/
├── renderer.h               # Frozen public API — types, handles, free functions
├── renderer.cpp             # Core: init / shutdown / run / frame lifecycle
├── pipeline_unlit.cpp       # Unlit forward pipeline (R-M1)
├── pipeline_unlit.h
├── pipeline_lambertian.cpp  # Lambertian diffuse pipeline (R-M2)
├── pipeline_lambertian.h
├── pipeline_blinnphong.cpp  # Blinn-Phong + textures (R-M4, Desirable)
├── pipeline_blinnphong.h
├── mesh_builders.cpp        # Procedural sphere / cube (+ capsule R-M5)
├── mesh_builders.h
├── skybox.cpp               # Cubemap loading + skybox pass (R-M3)
├── skybox.h
├── debug_draw.cpp           # Line-quad billboard renderer (R-M3)
├── debug_draw.h
├── texture.cpp              # upload_texture_2d / upload_texture_from_file (R-M4)
├── texture.h
└── app/
    └── main.cpp             # renderer_app driver — procedural demo, updated each milestone

src/renderer/mocks/          # Mock stubs consumed by engine when USE_RENDERER_MOCKS=ON

shaders/renderer/
├── unlit.glsl               # R-M1
├── lambertian.glsl          # R-M2
├── skybox.glsl              # R-M3
├── line_quad.glsl           # R-M3
└── blinnphong.glsl          # R-M4 (Desirable)

docs/interfaces/
└── renderer-interface-spec.md   # Frozen after R-M1; authoritative for engine SpecKit

docs/architecture/
└── renderer-architecture.md     # Populated by this plan

_coordination/renderer/
├── TASKS.md                 # Renderer task board (populated by speckit.tasks)
├── PROGRESS.md
├── VALIDATION/
└── REVIEWS/
```

**Structure Decision**: Single C++ static-lib workstream. `renderer_app` is the standalone driver and milestone-acceptance vehicle. Shaders live under `shaders/renderer/` and are compiled at build time into `${CMAKE_BINARY_DIR}/generated/shaders/`. No game or engine source is touched by this workstream.

## Complexity Tracking

> No constitution violations — table omitted.
