---
name: sokol-api
description: Use when working with sokol_gfx.h, sokol_app.h, sokol_time.h, sokol_glue.h, sokol_log.h, or sokol_imgui.h in any workstream. Covers resource creation (buffers, images, samplers, shaders, pipelines), render passes, draw calls, app lifecycle, time measurement, and the sokol_app↔sokol_gfx glue. Activated when authoring or reviewing renderer code that calls sg_* or sapp_* functions. Do NOT use for sokol-shdc shader compilation workflow — use the sokol-shdc skill instead.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.1"
  project-stage: pre-implementation
  role: library-reference
  activated-by: any sg_* or sapp_* usage
---

# sokol-api Skill

Reference for `sokol_gfx.h`, `sokol_app.h`, `sokol_time.h`, `sokol_glue.h`, `sokol_log.h`, and `sokol_imgui.h`.
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
```

---

## 4. External Library Integration

### Texture Loading (`stb_image`)
```c
int width, height, channels;
unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
sg_image img = sg_make_image(&(sg_image_desc){
    .width = width, .height = height,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .data.mip_levels[0] = SG_RANGE(pixels),
    .label = "texture"
});
stbi_image_free(pixels);

sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
    .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR,
    .label = "sampler"
});
```

### glTF Integration (`cgltf`)
Extracting mesh data from `cgltf` and mapping to Sokol:
```c
// Attributes from cgltf accessors
const cgltf_accessor* acc = find_accessor(prim, "POSITION");
void* ptr = (uint8_t*)acc->buffer_view->buffer->data + acc->offset + acc->buffer_view->offset;

sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .data = { .ptr = ptr, .size = acc->count * acc->stride },
    .label = "gltf-vertices"
});
```

---

## 5. Advanced Rendering Patterns

### Offscreen Rendering
Demonstrates rendering to a texture via `sg_pass` with custom attachments:
```c
// 1. Create color and depth attachments
sg_image color = sg_make_image(&(sg_image_desc){
    .render_target = true, .width = 512, .height = 512
});
sg_image depth = sg_make_image(&(sg_image_desc){
    .render_target = true, .width = 512, .height = 512, .pixel_format = SG_PIXELFORMAT_DEPTH
});

// 2. Setup the pass
sg_pass offscreen_pass = {
    .attachments = { .colors[0].image = color, .depth_stencil.image = depth },
    .action = { .colors[0].load_action = SG_LOADACTION_CLEAR }
};

// 3. Render
sg_begin_pass(&offscreen_pass);
// ... draw calls ...
sg_end_pass();
```

### Instancing
Configuring the pipeline for per-instance data:
```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX,
        .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
        .attrs = {
            [ATTR_vs_position] = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0 },
            [ATTR_vs_instance_pos] = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=1 }
        }
    },
    .shader = shd,
    .index_type = SG_INDEXTYPE_UINT16,
    .label = "instancing-pipeline"
});

// Bindings include the instance buffer
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = mesh_vbuf,
    .vertex_buffers[1] = instance_vbuf,
    .index_buffer = mesh_ibuf
});
sg_draw(0, 36, 1000); // 1000 instances
```

---

## 6. Pipeline Definition (Cheatsheet)

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
        [ATTR_vs_position]  = { .format = SG_VERTEXFORMAT_FLOAT3 },
        [ATTR_vs_texcoord0] = { .format = SG_VERTEXFORMAT_FLOAT2 },
        [ATTR_vs_normal]    = { .format = SG_VERTEXFORMAT_FLOAT3 },
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
    
    // Debug UI (optional)
    simgui_new_frame({ width, height, dt, 1.0f });
    // ... ImGui calls ...
    simgui_render();

    sg_end_pass();
    sg_commit();
}
```

---

## 8. Deeper References

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
