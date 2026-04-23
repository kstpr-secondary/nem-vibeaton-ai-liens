# Renderer Data Model

> **Phase 1 output** for `001-sokol-render-engine`. Entities, fields, relationships, state transitions, and validation rules.

---

## Handle Types

All GPU resources are referenced by opaque handles — thin structs wrapping a `uint32_t` id. Index `0` is reserved as the invalid/null handle sentinel.

```cpp
struct RendererMeshHandle    { uint32_t id = 0; };
struct RendererTextureHandle { uint32_t id = 0; };

inline bool renderer_handle_valid(RendererMeshHandle h)    { return h.id != 0; }
inline bool renderer_handle_valid(RendererTextureHandle h) { return h.id != 0; }
```

**Validation rule**: Any draw call or operation that receives an invalid handle (`id == 0`) is silently skipped. No assertion, no crash.

---

## RendererConfig

Passed to `renderer_init()`. Controls window and clear color.

```cpp
struct RendererConfig {
    int   width       = 1280;
    int   height      = 720;
    float clear_r     = 0.05f;
    float clear_g     = 0.05f;
    float clear_b     = 0.10f;
    float clear_a     = 1.0f;
    const char* title = "renderer_app";
};
```

**Validation rules**:
- `width` and `height` must be > 0 (assert in debug builds).
- `title` must not be null; falls back to `"renderer"` if null.

---

## Vertex (Canonical Layout)

Used for all meshes — procedural and uploaded. One universal layout avoids pipeline fragmentation across shading tiers.

```cpp
struct Vertex {
    float position[3];  // xyz world-space position
    float normal[3];    // xyz world-space normal (normalized)
    float uv[2];        // st texture coordinates [0,1]
    float tangent[3];   // xyz tangent vector (normalized), for normal maps
};
```

**Validation rules**:
- Normals and tangents must be unit-length at upload time (not validated at runtime — caller responsibility).
- Index buffer uses `uint16_t` for ≤65535 vertices, `uint32_t` for larger meshes (procedural builders always use `uint16_t`; `upload_mesh` accepts `uint32_t`).

---

## ShadingModel

Enum controlling which pipeline is selected at draw time.

```cpp
enum class ShadingModel : uint8_t {
    Unlit       = 0,  // flat base_color, no lighting (R-M1)
    Lambertian  = 1,  // diffuse only from directional light (R-M2)
    BlinnPhong  = 2,  // diffuse + specular + shininess (R-M4, Desirable)
};
```

---

## Material

Inline value type; not reference-counted. Passed per draw command.

```cpp
struct Material {
    ShadingModel       shading_model = ShadingModel::Unlit;
    float              base_color[4] = {1, 1, 1, 1};  // RGBA linear
    RendererTextureHandle texture    = {};              // {0} = use base_color only
    float              shininess     = 32.0f;           // BlinnPhong only
    float              alpha         = 1.0f;            // 1 = fully opaque
};
```

**Validation rules**:
- `alpha < 1.0` routes the draw to the transparent pass automatically.
- `shininess` is clamped to `[1, 512]` in the shader uniform upload.
- If `texture.id != 0` and the handle is invalid (e.g., freed or never uploaded), the renderer falls back to `base_color`.

---

## DrawCommand

Immutable per-frame submission. Consumed during `end_frame()`. Not retained across frames.

```cpp
struct DrawCommand {
    RendererMeshHandle mesh;
    float              transform[16];  // column-major 4x4 world matrix
    Material           material;
};
```

**Validation rules**:
- Maximum 1024 `DrawCommand` entries per frame. Submissions beyond this limit are silently dropped; one debug log line is emitted per overflow.
- Submission before `begin_frame()` or after `end_frame()` is silently ignored.
- Invalid `mesh.id` → draw skipped silently.

---

## LineQuadCommand

Camera-facing billboard quad between two world-space endpoints. Consumed during `end_frame()`.

```cpp
struct LineQuadCommand {
    float p0[3];          // world-space start point
    float p1[3];          // world-space end point
    float width;          // billboard half-width in world units
    float color[4];       // RGBA linear; alpha controls blending
};
```

**Validation rules**:
- Zero-length quads (`p0 == p1`) are silently skipped.
- `width <= 0` → silently skipped.
- Routed through the transparent pass (same as alpha < 1 draw commands).
- Uses a **separate** fixed queue of **256 slots** (`line_quad_queue[256]`); does **not** share the 1024-slot `draw_queue`. Submissions beyond 256 are silently dropped and logged. The 1024-slot `draw_queue` budget (FR-004) applies only to `enqueue_draw()` calls.

---

## DirectionalLight

One active directional light maximum per frame. Set via `renderer_set_directional_light()`.

```cpp
struct DirectionalLight {
    float direction[3];   // world-space direction vector (normalized; toward light source)
    float color[3];       // linear RGB [0,1]
    float intensity;      // scalar multiplier, typically [0, 5]
};
```

**Validation rules**:
- Direction is re-normalized in the uniform upload; zero vector falls back to `{0, -1, 0}` (straight down).
- If `set_directional_light()` is not called before `end_frame()`, the previous frame's light is retained (not zeroed). Debug builds log a warning.

---

## Camera

View-projection pair. Set once per frame via `renderer_set_camera()`.

```cpp
struct RendererCamera {
    float view[16];        // column-major 4x4 view matrix (world→camera)
    float projection[16];  // column-major 4x4 projection matrix (camera→clip)
};
```

**Validation rules**:
- If `set_camera()` is not called before `end_frame()`, the identity matrix is used for both view and projection; a warning is logged in debug builds.
- The combined view-projection is computed on CPU (`projection × view`) and uploaded as a single `mat4` uniform to reduce shader uniform slots.

---

## Skybox

Set via `renderer_set_skybox(handle)`. One cubemap texture per frame.

```cpp
// handle is a RendererTextureHandle created via upload_cubemap()
// Cleared to {0} at shutdown; setting {0} disables the skybox.
```

**Validation rules**:
- If the skybox handle is invalid, the skybox pass is skipped (only clear color visible).
- Skybox is drawn first in the frame with depth write disabled.

---

## Internal Frame State (not exposed in public API)

These are implementation-internal tracking types documented here for agent clarity:

| Field | Type | Notes |
|-------|------|-------|
| `draw_queue[1024]` | `DrawCommand[]` | Pre-allocated ring; `draw_count` tracks fill level |
| `line_quad_queue[1024]` | `LineQuadCommand[]` | Shares the 1024 budget with draw_queue (combined limit) |
| `camera` | `RendererCamera` | Overwritten each frame by `set_camera()` |
| `light` | `DirectionalLight` | Retained frame-to-frame; overwritten by `set_directional_light()` |
| `skybox_handle` | `RendererTextureHandle` | Set by `set_skybox()`; persistent |
| `mesh_table[]` | `sg_buffer[2][]` | Parallel vertex/index buffer arrays indexed by handle id |
| `texture_table[]` | `sg_image[]` | Indexed by handle id |
| `pipeline_unlit` | `sg_pipeline` | Created at `init()` |
| `pipeline_lambertian` | `sg_pipeline` | Created at `init()` |
| `pipeline_blinnphong` | `sg_pipeline` | Created at `init()` (R-M4) |
| `pipeline_transparent` | `sg_pipeline` | Separate blend state for transparent pass |
| `pipeline_skybox` | `sg_pipeline` | Depth write off, front-face cull reversed |
| `pipeline_line_quad` | `sg_pipeline` | Billboard quad pipeline |
| `pipeline_magenta` | `sg_pipeline` | Fallback on creation failure |

---

## State Transitions

```
[init()]
  → window opens
  → sokol_gfx initialized
  → all pipelines created (failure → magenta fallback stored)
  → ImGui backend initialized
  → frame_active = false

[run()]
  → enter sokol_app main loop
  → per-frame: frame_callback() called by sokol_app

[frame_callback()]
  → begin_frame()          frame_active = true; queues cleared; simgui_new_frame()
  → consumer enqueues      draw_count incremented (capped at 1024)
  → end_frame()            sort + submit opaque pass → transparent pass → skybox → line quads → ImGui; frame_active = false

[shutdown()]
  → simgui_shutdown()
  → sg_shutdown()
  → sapp_quit()
  → all handles invalidated
```

---

## Relationships

```
RendererConfig ──(1)──► renderer_init()
                              │
                              ▼
                      sokol_app + sokol_gfx
                              │
                    ┌─────────┼──────────────┐
                    ▼         ▼              ▼
               Pipelines  MeshTable    TextureTable
                              │              │
                    DrawCommand.mesh    Material.texture
                    DrawCommand.material.shading_model → selects Pipeline
```
