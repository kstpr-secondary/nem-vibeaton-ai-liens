---
name: sokol-api
description: Use when working with sokol_gfx.h, sokol_app.h, sokol_time.h, sokol_glue.h, or sokol_log.h in any workstream. Covers resource creation (buffers, images, samplers, shaders, pipelines), render passes, draw calls, app lifecycle, time measurement, and the sokol_app↔sokol_gfx glue. Activated when authoring or reviewing renderer code that calls sg_* or sapp_* functions. Do NOT use for sokol-shdc shader compilation workflow — use the sokol-shdc skill instead.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: library-reference
  activated-by: any sg_* or sapp_* usage
---

# sokol-api Skill

Reference for `sokol_gfx.h`, `sokol_app.h`, `sokol_time.h`, `sokol_glue.h`, and `sokol_log.h`.
**Target backend: `SG_BACKEND_GLCORE` (OpenGL 3.3 Core).**

Use per-aspect references under `references/` when you need deeper detail on a specific topic. Read only what the task requires.

---

## 1. Core Rules (must know)

1. **Header-only, STB style.** One `.cpp` file must `#define SOKOL_GFX_IMPL` (and similarly for other headers) before the include. All other files include without the macro.
2. **Never hand-fill `sg_shader_desc`**. Use the generated `xxx_shader_desc(sg_query_backend())` from sokol-shdc.
3. **All resource IDs wrap `uint32_t`.** `id == 0` (`SG_INVALID_ID`) means uninitialized. Check with `sg_query_*_state()` after creation.
4. **Pipeline creation failure → magenta error pipeline.** Never crash on a failed pipeline. Log the error, fall back.
5. **`ASSET_ROOT` macro for all runtime file paths.** Never hardcode relative paths for textures or meshes.
6. **Labels everywhere** (`.label = "my-thing"`). Required for RenderDoc and runtime diagnostics.
7. **`sg_shutdown()` is always last** in `cleanup()`. Destroy individual resources before it if you want ordered teardown.

---

## 2. Application Lifecycle (`sokol_app.h`)

```c
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb    = init,
        .frame_cb   = frame,
        .cleanup_cb = cleanup,
        .event_cb   = event,
        .width      = 1280,
        .height     = 720,
        .window_title = "My App",
        .logger.func  = slog_func,
        .gl = { .major_version = 3, .minor_version = 3 },  // OpenGL 3.3 Core
    };
}
```

| Callback | Purpose |
|----------|---------|
| `init_cb` | `sg_setup()`, create all GPU resources |
| `frame_cb` | Update state, begin pass, draw, end pass, commit |
| `cleanup_cb` | Destroy resources, `sg_shutdown()` |
| `event_cb` | Handle `sapp_event` (keyboard, mouse, resize) |

---

## 3. Graphics Initialization (`sokol_gfx.h`)

```c
static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),  // always use this
        .logger.func = slog_func,
    });
    stm_setup();  // if using sokol_time
}

static void cleanup(void) {
    sg_shutdown();  // last call
}
```

---

## 4. Resource Types (IDs)

```c
sg_buffer    // vertex or index buffer
sg_image     // 2D texture, render target, cubemap
sg_sampler   // filter + wrap settings
sg_shader    // compiled shader (from sokol-shdc generated desc)
sg_pipeline  // shader + vertex layout + render state
sg_view      // image/buffer bound to shader slots
```

All IDs are `uint32_t` handles. Test validity: `sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID`.

---

## 5. Key Enums (quick lookup)

| Category | Common Values |
|----------|--------------|
| Usage | `SG_USAGE_IMMUTABLE` (default), `SG_USAGE_DYNAMIC`, `SG_USAGE_STREAM` |
| Buffer type | `SG_BUFFERTYPE_VERTEXBUFFER` (default), `SG_BUFFERTYPE_INDEXBUFFER` |
| Index type | `SG_INDEXTYPE_NONE`, `SG_INDEXTYPE_UINT16`, `SG_INDEXTYPE_UINT32` |
| Vertex format | `SG_VERTEXFORMAT_FLOAT3` (pos/normal), `SG_VERTEXFORMAT_FLOAT2` (UV), `SG_VERTEXFORMAT_FLOAT4` |
| Vertex step | `SG_VERTEXSTEP_PER_VERTEX` (default), `SG_VERTEXSTEP_PER_INSTANCE` |
| Pixel format | `SG_PIXELFORMAT_RGBA8` (color), `SG_PIXELFORMAT_DEPTH` (depth), `SG_PIXELFORMAT_SRGB8A8` |
| Cull mode | `SG_CULLMODE_NONE` (default), `SG_CULLMODE_BACK` (typical), `SG_CULLMODE_FRONT` |
| Compare | `SG_COMPAREFUNC_LESS_EQUAL` (depth), `SG_COMPAREFUNC_LESS` |
| Load action | `SG_LOADACTION_CLEAR` (default), `SG_LOADACTION_LOAD`, `SG_LOADACTION_DONTCARE` |
| Filter | `SG_FILTER_LINEAR` (default), `SG_FILTER_NEAREST` |
| Wrap | `SG_WRAP_REPEAT` (default), `SG_WRAP_CLAMP_TO_EDGE` |

---

## 6. Resource Creation Cheatsheet

**Immutable vertex buffer:**
```c
sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(vertices),
    .label = "mesh-vbuf",
});
```

**Index buffer:**
```c
sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices),
    .label = "mesh-ibuf",
});
```

**Dynamic buffer (update once/frame):**
```c
sg_buffer dyn = sg_make_buffer(&(sg_buffer_desc){
    .size = MAX * sizeof(vertex_t),
    .usage = { .vertex_buffer = true, .dynamic_update = true },
    .label = "dynamic-vbuf",
});
// Each frame:
sg_update_buffer(dyn, &SG_RANGE(data));
```

**2D texture:**
```c
sg_image img = sg_make_image(&(sg_image_desc){
    .width = w, .height = h,
    .data.mip_levels[0] = SG_RANGE(pixels),
    .label = "texture",
});
```

**Sampler:**
```c
sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
    .wrap_u = SG_WRAP_REPEAT,
    .wrap_v = SG_WRAP_REPEAT,
    .label = "default-smp",
});
```

**Shader (always via sokol-shdc):**
```c
#include "shaders/my_shader.glsl.h"
sg_shader shd = sg_make_shader(my_shader_shader_desc(sg_query_backend()));
```

**Pipeline:**
```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout.attrs = {
        [SG_ATTR_vs_position]  = { .format = SG_VERTEXFORMAT_FLOAT3 },
        [SG_ATTR_vs_texcoord0] = { .format = SG_VERTEXFORMAT_FLOAT2 },
        [SG_ATTR_vs_normal]    = { .format = SG_VERTEXFORMAT_FLOAT3 },
    },
    .index_type = SG_INDEXTYPE_UINT16,
    .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true },
    .cull_mode = SG_CULLMODE_BACK,
    .label = "main-pip",
});
```

---

## 7. Render Loop Pattern

```c
static void frame(void) {
    uint64_t dt_ticks = stm_laptime(&state.last_time);
    float dt = (float)stm_sec(dt_ticks);

    sg_begin_pass(&(sg_pass){
        .action = state.pass_action,
        .swapchain = sglue_swapchain(),
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    vs_params_t vs_p = { .mvp = compute_mvp() };
    sg_apply_uniforms(SG_SLOT_vs_params, &SG_RANGE(vs_p));
    sg_draw(0, state.num_elements, 1);
    sg_end_pass();
    sg_commit();
}
```

Draw call sequence within a pass:
```
sg_begin_pass → sg_apply_pipeline → sg_apply_bindings → sg_apply_uniforms → sg_draw → sg_end_pass → sg_commit
```

---

## 8. Cleanup Pattern

```c
static void cleanup(void) {
    sg_destroy_pipeline(state.pip);
    sg_destroy_shader(state.shd);
    sg_destroy_buffer(state.vbuf);
    sg_destroy_image(state.img);
    sg_destroy_sampler(state.smp);
    sg_shutdown();  // always last
}
```

---

## 9. Time (`sokol_time.h`)

```c
stm_setup();                          // once in init()
uint64_t lap = stm_laptime(&last_t);  // frame delta ticks
float dt = (float)stm_sec(lap);       // to seconds
```

---

## 10. Deeper References

For deeper API detail, open only the reference you need:

| Aspect | File | When to open |
|--------|------|-------------|
| Descriptor structs (full) | `references/sokol-api-work-with-descriptors.md` | Creating complex resources with non-default fields |
| Buffer management | `references/sokol-api-work-with-buffers.md` | Dynamic/stream buffers, append, overflow check |
| Render passes | `references/sokol-api-render-passes.md` | Offscreen rendering, multi-pass |
| Draw calls & state | `references/sokol-api-draw-calls.md` | Instancing, viewport, scissor, extended draw |
| App, time & queries | `references/sokol-api-app-time.md` | Input events, key codes, query functions |
| Shadow mapping *(stretch)* | `references/sokol-api-shadow-mapping.md` | Offscreen depth pass, light-space transforms |
| Compute dispatch *(stretch)* | `references/sokol-api-compute-dispatch.md` | Compute pipelines, SSBOs, dispatch (GL 4.3+) |
