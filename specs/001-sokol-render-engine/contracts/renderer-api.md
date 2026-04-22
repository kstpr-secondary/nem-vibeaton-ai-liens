# Renderer Public API Contract

> **Status**: DRAFT — produced by SpecKit plan phase.  
> **Promotion path**: This file becomes authoritative at `docs/interfaces/renderer-interface-spec.md` after human approval and freeze marker addition.  
> **Freeze condition**: Must be frozen (version marker added, status changed to FROZEN) before the engine workstream begins its SpecKit planning cycle.

---

## Version

`v0.1-draft` — pre-freeze. Breaking changes allowed until freeze.

---

## Overview

The renderer exposes a single C++ header `renderer.h` (under `src/renderer/`) with all public types and free-function declarations. All consuming workstreams (`engine`, `game`, `engine_app`, `game`) include only this header. Internal implementation files are never included directly.

---

## Header Shape (`src/renderer/renderer.h`)

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
    ShadingModel        shading_model = ShadingModel::Unlit;
    float               base_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // RGBA linear
    RendererTextureHandle texture      = {};   // {0} = no texture
    float               shininess      = 32.0f;
    float               alpha          = 1.0f;
};

// ---------------------------------------------------------------------------
// Directional light — one per frame
// ---------------------------------------------------------------------------

struct DirectionalLight {
    float direction[3];   // world-space, normalized, toward source
    float color[3];       // linear RGB
    float intensity;      // scalar multiplier
};

// ---------------------------------------------------------------------------
// Camera — view/projection pair set once per frame
// ---------------------------------------------------------------------------

struct RendererCamera {
    float view[16];        // column-major 4x4 world→camera
    float projection[16];  // column-major 4x4 camera→clip
};

// ---------------------------------------------------------------------------
// Input callback — registered by engine/app to receive raw events
// ---------------------------------------------------------------------------

// Opaque sokol_app event forwarded verbatim. Consumer casts via sapp_event*.
using InputCallback = void(*)(const void* sapp_event, void* user_data);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void renderer_init(const RendererConfig& config);
void renderer_set_input_callback(InputCallback cb, void* user_data);
void renderer_run();       // blocking — owns sokol_app main loop
void renderer_shutdown();  // called from within sokol_app cleanup event

// ---------------------------------------------------------------------------
// Per-frame API (called from within frame callback registered via renderer_run)
// ---------------------------------------------------------------------------

void renderer_begin_frame();
void renderer_end_frame();

// ---------------------------------------------------------------------------
// Scene setup (called between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera& camera);
void renderer_set_directional_light(const DirectionalLight& light);
void renderer_set_skybox(RendererTextureHandle cubemap);

// ---------------------------------------------------------------------------
// Draw submission (called between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_enqueue_draw(
    RendererMeshHandle    mesh,
    const float           world_transform[16],  // column-major 4x4
    const Material&       material
);

// Camera-facing billboard quad between two world-space endpoints.
void renderer_enqueue_line_quad(
    const float p0[3],
    const float p1[3],
    float       width,
    const float color[4]   // RGBA linear
);

// ---------------------------------------------------------------------------
// Mesh builders — procedural geometry (lives in renderer, consumed by all)
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions);
RendererMeshHandle renderer_make_cube_mesh(float half_extent);
// R-M5 (Desirable): RendererMeshHandle renderer_make_capsule_mesh(float radius, float height, int subdivisions);

// Upload external geometry (engine-imported glTF/OBJ data)
RendererMeshHandle renderer_upload_mesh(
    const Vertex*    vertices,
    uint32_t         vertex_count,
    const uint32_t*  indices,
    uint32_t         index_count
);

// ---------------------------------------------------------------------------
// Texture upload
// ---------------------------------------------------------------------------

// (a) Raw pixel buffer — used by engine for cgltf-embedded textures
RendererTextureHandle renderer_upload_texture_2d(
    const void* pixels,
    int         width,
    int         height,
    int         channels    // 3 = RGB, 4 = RGBA
);

// (b) File path convenience — decodes PNG/JPG/BMP via stb_image, calls (a)
RendererTextureHandle renderer_upload_texture_from_file(const char* path);

// Cubemap — six faces in +X/-X/+Y/-Y/+Z/-Z order
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

1. **Lifecycle order**: `renderer_init()` → `renderer_set_input_callback()` → `renderer_run()`.  
   Inside `renderer_run()`, the consumer's frame tick function calls `renderer_begin_frame()` / scene setup / draw submission / `renderer_end_frame()`.
2. **Thread safety**: None. All calls must occur on the main thread (sokol_app requirement).
3. **Frame boundary**: `enqueue_draw` / `enqueue_line_quad` / `set_camera` / `set_directional_light` / `set_skybox` are only valid between `begin_frame()` and `end_frame()`. Calls outside this window are silently ignored.
4. **Handle lifetime**: Valid from creation until `renderer_shutdown()`. No per-handle destroy.

---

## Error Behavior (non-negotiable)

| Situation | Behavior |
|-----------|----------|
| Shader / pipeline creation failure at `init()` | Magenta error pipeline installed; logged; no crash |
| Draw with invalid mesh handle | Call silently skipped |
| Draw with invalid texture handle | Falls back to `base_color` |
| More than 1024 submissions per frame | Overflow silently dropped; one debug log line |
| Zero-length line quad | Silently skipped |
| `set_camera()` not called | Identity matrices used; debug warning logged |
| `set_directional_light()` not called | Previous frame's light retained; debug warning logged |

---

## Mock Surface

When `USE_RENDERER_MOCKS=ON`, `src/renderer/mocks/renderer_mock.cpp` provides no-op or stub implementations of every function above. Return values:
- `RendererMeshHandle` stubs return `{1}` (a non-zero dummy handle) so downstream code that checks `renderer_handle_valid()` does not panic.
- `RendererTextureHandle` stubs return `{1}`.
- All void functions are no-ops.

---

## Freeze Procedure

1. Human supervisor reviews this contract against the spec acceptance scenarios.
2. Adds `**Status**: FROZEN — v1.0` to the top of `docs/interfaces/renderer-interface-spec.md`.
3. Commits with message `freeze: renderer interface v1.0`.
4. Announces to engine workstream: engine SpecKit may now begin.
5. Any subsequent change to a frozen signature requires explicit human approval and a version bump.
