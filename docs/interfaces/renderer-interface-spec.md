# Renderer Interface Spec

> **Status:** `FROZEN — v1.2`
> **Source**: Promoted from `specs/001-sokol-render-engine/contracts/renderer-api.md`

---

## Version

`FROZEN — v1.2`

### Changelog

- **v1.2**: Full Material redesign (Section 3.1 of visual-improvements.md). Removed `ShadingModel` enum. Added `RendererShaderHandle`, `PipelineState`, `BlendMode`, `CullMode`. Replaced flat `Material` fields with pipeline-cache-backed `shader` + `pipeline` + generic `uniforms` blob (256 bytes). Added `renderer_create_shader()`, `renderer_builtin_shader(BuiltinShader)`, `renderer_set_time(float)`. Updated `renderer_enqueue_line_quad` with optional `BlendMode` parameter. Published `UnlitFSParams` / `BlinnPhongFSParams` for game-side uniform casting. Convenience factories (`renderer_make_unlit_material`, etc.) now fill the uniforms blob and set `shader = builtin_shader(...)`. Approved by human supervisor via visual-improvements.md (2026-05-03).
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
#include <cstring>

// GLM for FS param structs (UnlitFSParams, BlinnPhongFSParams)
#include <glm/glm.hpp>

// sokol_gfx.h for sg_shader_desc (renderer_create_shader)
#include "sokol_gfx.h"

// ---------------------------------------------------------------------------
// Handle types — opaque GPU resource references
// ---------------------------------------------------------------------------

struct RendererMeshHandle    { uint32_t id = 0; };
struct RendererTextureHandle { uint32_t id = 0; };
struct RendererShaderHandle  { uint32_t id = 0; };

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
// Pipeline state — travels with the material instance
// ---------------------------------------------------------------------------

enum class BlendMode  : uint8_t { Opaque = 0, Cutout, AlphaBlend, Additive };
enum class CullMode   : uint8_t { Back   = 0, Front,  None };

struct PipelineState {
    BlendMode blend        = BlendMode::Opaque;
    CullMode  cull         = CullMode::Back;
    bool      depth_write  = true;
    uint8_t   render_queue = 0;
    // render_queue: 0=opaque, 1=cutout, 2=transparent, 3=additive
    // Renderer draws passes in this order. Custom shaders choose their queue.
};

// ---------------------------------------------------------------------------
// Material — inline value type, passed per draw call
// ---------------------------------------------------------------------------

static constexpr int k_material_uniform_bytes = 256;
static constexpr int k_material_texture_slots  = 4;

struct Material {
    RendererShaderHandle  shader;
    PipelineState         pipeline;
    uint8_t               uniforms[k_material_uniform_bytes] = {};
    uint8_t               uniforms_size  = 0;
    RendererTextureHandle textures[k_material_texture_slots] = {};
    uint8_t               texture_count  = 0;
};

// Typed helpers — both declared inline in renderer.h
template<typename T>
void material_set_uniforms(Material& m, const T& params) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    memcpy(m.uniforms, &params, sizeof(T));
    m.uniforms_size = static_cast<uint8_t>(sizeof(T));
}

template<typename T>
T* material_uniforms_as(Material& m) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    return reinterpret_cast<T*>(m.uniforms);
}

// ---------------------------------------------------------------------------
// Published FS param structs for built-in shaders
// (Game code may reinterpret_cast into Material::uniforms using these)
// ---------------------------------------------------------------------------

struct UnlitFSParams {
    glm::vec4 color;             // rgba
    glm::vec4 flags;             // .x = use_texture (1.0 or 0.0)
};

struct BlinnPhongFSParams {
    glm::vec4 base_color;
    glm::vec4 light_dir_ws;
    glm::vec4 light_color_inten;
    glm::vec4 view_pos_w;
    glm::vec4 spec_shin;         // .rgb = specular color, .w = shininess
    glm::vec4 flags;             // .x = use_texture
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
// Frame callback — consumer injects per-frame logic
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
// Shader API
// ---------------------------------------------------------------------------

// Register a shader from a sokol-shdc generated descriptor.
// Call once at init after renderer_init(). Returns an opaque handle.
RendererShaderHandle renderer_create_shader(const sg_shader_desc* desc);

// Retrieve handles to the built-in shaders (no GPU work — just returns a stored handle).
enum class BuiltinShader : uint8_t { Unlit = 0, BlinnPhong, Lambertian };
RendererShaderHandle renderer_builtin_shader(BuiltinShader s);

// ---------------------------------------------------------------------------
// Per-frame API
// ---------------------------------------------------------------------------

void renderer_begin_frame();
void renderer_end_frame();

// Query submitted draw count, triangle count, and frustum cull count for ImGui HUD display.
int renderer_get_draw_count();
int renderer_get_triangle_count();
int renderer_get_culled_count();

// Propagate elapsed time to animated materials.
// Called once per tick from game_tick, before enqueue calls.
void renderer_set_time(float seconds_since_start);

// ---------------------------------------------------------------------------
// Scene setup (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera& camera);
void renderer_set_directional_light(const DirectionalLight& light);
void renderer_set_skybox(RendererTextureHandle cubemap);
void renderer_set_culling_enabled(bool enabled);

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
    const float color[4],
    BlendMode   blend = BlendMode::Opaque   // Additive for laser/VFX
);

// ---------------------------------------------------------------------------
// Mesh builders
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions);
RendererMeshHandle renderer_make_cube_mesh(float half_extent);

RendererMeshHandle renderer_upload_mesh(
    const Vertex*   vertices,
    uint32_t        vertex_count,
    const uint32_t* indices,
    uint32_t        index_count,
    float           radius = 0.0f      // 0.0f = auto-compute from vertex positions
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

RendererTextureHandle renderer_upload_texture_from_file(const char* path);

RendererTextureHandle renderer_upload_cubemap(
    const void* faces[6],
    int         face_width,
    int         face_height,
    int         channels
);

// ---------------------------------------------------------------------------
// Material helpers — convenience factories
// (Internally fill the uniforms blob using material_set_uniforms<T>(…)
//  and set m.shader = renderer_builtin_shader(...))
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
