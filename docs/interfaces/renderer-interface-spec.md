# Renderer Interface Spec

> **Status:** `FROZEN — v1.1`
> **Source**: Promoted from `specs/001-sokol-render-engine/contracts/renderer-api.md`

---

## Version

`FROZEN — v1.2`

### Changelog

- **v1.2**: Added `renderer_upload_texture_from_memory(pixels, w, h, ch)` — accepts raw RGBA pixel data already in memory, required for engine-side glTF texture extraction (Approach B). Approved by human supervisor via ASSESSMENT-gltf-texture-extraction.md.
- **v1.1**: Added `FrameCallback` type and `renderer_set_frame_callback()` — required for engine/game to inject per-frame tick logic into the renderer-owned sokol_app loop. Approved by human supervisor.

---

## Freeze rules

- This document becomes authoritative only after a human supervisor adds `**Status**: FROZEN — v1.0` at the top.
- Engine planning MUST NOT rely on any signature here until that freeze happens.
- Any post-freeze contract change requires explicit human approval and a version bump.
- Downstream workstreams that compile against this interface MUST NOT modify it to make their code compile; they must file a blocker in `_coordination/overview/BLOCKERS.md`.

---

## Overview

The renderer exposes a single C++ header `renderer.h` (under `src/renderer/`) with all public types and free-function declarations. All consuming workstreams (`engine`, `game`, `engine_app`) include only this header. Internal implementation files are never included directly.

---

## Public API (`src/renderer/renderer.h`)

```cpp
#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// Handle types — opaque GPU resource references
// ---------------------------------------------------------------------------

struct RendererMeshHandle    { uint32_t id = 0; };
struct RendererTextureHandle { uint32_t id = 0; };

inline bool renderer_handle_valid(RendererMeshHandle h)    { return h.id != 0; }
inline bool renderer_handle_valid(RendererTextureHandle h) { return h.id != 0; }

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

struct RendererConfig {
    int         width     = 1280;
    int         height    = 720;
    float       clear_r   = 0.05f;
    float       clear_g   = 0.05f;
    float       clear_b   = 0.10f;
    float       clear_a   = 1.0f;
    const char* title     = "renderer_app";
};

// ---------------------------------------------------------------------------
// Vertex layout — universal for all mesh types
// ---------------------------------------------------------------------------

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[3];
};

// ---------------------------------------------------------------------------
// Shading model
// ---------------------------------------------------------------------------

enum class ShadingModel : uint8_t {
    Unlit      = 0,   // R-M1
    Lambertian = 1,   // R-M2
    BlinnPhong = 2,   // R-M4 (Desirable)
};

// ---------------------------------------------------------------------------
// Material — inline value type, passed per draw call
// ---------------------------------------------------------------------------

struct Material {
    ShadingModel          shading_model = ShadingModel::Unlit;
    float                 base_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    RendererTextureHandle texture        = {};
    float                 shininess      = 32.0f;
    float                 alpha          = 1.0f;
};

// ---------------------------------------------------------------------------
// Directional light — one per frame maximum
// ---------------------------------------------------------------------------

struct DirectionalLight {
    float direction[3];
    float color[3];
    float intensity;
};

// ---------------------------------------------------------------------------
// Camera — view/projection pair, set once per frame
// ---------------------------------------------------------------------------

struct RendererCamera {
    float view[16];        // column-major 4x4 world→camera
    float projection[16];  // column-major 4x4 camera→clip
};

// ---------------------------------------------------------------------------
// Input callback
// ---------------------------------------------------------------------------

using InputCallback = void(*)(const void* sapp_event, void* user_data);

// ---------------------------------------------------------------------------
// Frame callback (v1.1) — consumer injects per-frame logic
// ---------------------------------------------------------------------------

using FrameCallback = void(*)(float dt, void* user_data);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void renderer_init(const RendererConfig& config);
void renderer_set_frame_callback(FrameCallback cb, void* user_data);
void renderer_set_input_callback(InputCallback cb, void* user_data);
void renderer_run();
void renderer_shutdown();

// ---------------------------------------------------------------------------
// Per-frame API
// ---------------------------------------------------------------------------

void renderer_begin_frame();
void renderer_end_frame();

// ---------------------------------------------------------------------------
// Scene setup (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera& camera);
void renderer_set_directional_light(const DirectionalLight& light);
void renderer_set_skybox(RendererTextureHandle cubemap);

// ---------------------------------------------------------------------------
// Draw submission (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_enqueue_draw(
    RendererMeshHandle  mesh,
    const float         world_transform[16],
    const Material&     material
);

void renderer_enqueue_line_quad(
    const float p0[3],
    const float p1[3],
    float       width,
    const float color[4]
);

// ---------------------------------------------------------------------------
// Mesh builders
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions);
RendererMeshHandle renderer_make_cube_mesh(float half_extent);
// Desirable (R-M5): renderer_make_capsule_mesh(float radius, float height, int subdivisions)

RendererMeshHandle renderer_upload_mesh(
    const Vertex*   vertices,
    uint32_t        vertex_count,
    const uint32_t* indices,
    uint32_t        index_count
);

// ---------------------------------------------------------------------------
// Texture upload
// ---------------------------------------------------------------------------

RendererTextureHandle renderer_upload_texture_2d(
    const void* pixels,
    int         width,
    int         height,
    int         channels
);

RendererTextureHandle renderer_upload_texture_from_memory(
    const void* pixels,
    int         width,
    int         height,
    int         channels
);

RendererTextureHandle renderer_upload_texture_from_file(const char* path);

RendererTextureHandle renderer_upload_cubemap(
    const void* faces[6],
    int         face_width,
    int         face_height,
    int         channels
);

// ---------------------------------------------------------------------------
// Material helpers
// ---------------------------------------------------------------------------

Material renderer_make_unlit_material(const float rgba[4]);
Material renderer_make_lambertian_material(const float rgb[3]);
Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                           RendererTextureHandle texture = {});
```

---

## Calling Convention

1. `renderer_init()` → `renderer_set_frame_callback()` → `renderer_set_input_callback()` → `renderer_run()` (blocking).
2. Each frame, the renderer calls the registered `FrameCallback` with the frame delta time. Inside that callback the consumer calls: `begin_frame()` → scene setup + enqueue → `end_frame()`.
3. If no frame callback is registered, `renderer_run()` renders empty frames (clear color only).
4. All calls on the main thread only (sokol_app requirement).
5. All draw/scene functions outside `begin_frame`/`end_frame` are silently ignored.
6. All resource handles are valid from creation until `renderer_shutdown()`.

---

## Error Behavior

| Situation | Behavior |
|-----------|----------|
| Shader/pipeline creation failure | Magenta error pipeline; logged; no crash |
| Draw with invalid mesh handle | Silently skipped |
| Draw with invalid texture handle | Falls back to `base_color` |
| > 1024 submissions/frame | Overflow dropped; one debug log |
| Zero-length line quad | Silently skipped |
| `set_camera()` not called | Identity matrices; debug warning |
| `set_directional_light()` not called | Previous frame's light retained; debug warning |
| No frame callback registered | Empty frames (clear color); debug warning once |

---

## Mock Surface

`src/renderer/mocks/renderer_mock.cpp` — activated by `USE_RENDERER_MOCKS=ON`. All void functions are no-ops; handle-returning functions return `{1}` (valid sentinel).

---

## Implementation Verification (R-027 / R-055)

**Verified:** 2026-04-23 by `claude@laptopA`  
**Result:** ZERO signature drift. Every declaration in `renderer.h` is present in the frozen spec; no spec entry is absent from the header.  
**Build:** `cmake --build build --target renderer_app` → exit 0, no errors.  
**Demo scene:** 12 draw calls (6 spheres + 6 cubes) configured in `src/renderer/app/main.cpp`.  
**Pending:** Human behavioral check SC-002 (visual render confirmation) and SC-007 (interface freeze announcement to engine workstream / laptopB).

---

## Intended freeze inputs

- `pre_planning_docs/Hackathon Master Blueprint.md`
- `pre_planning_docs/Renderer Concept and Milestones.md`
- `docs/architecture/renderer-architecture.md`
- `specs/001-sokol-render-engine/plan.md`
- `specs/001-sokol-render-engine/contracts/renderer-api.md`
