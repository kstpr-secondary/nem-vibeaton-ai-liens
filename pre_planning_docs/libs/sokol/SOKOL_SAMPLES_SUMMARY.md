# Sokol Samples & API Summary (OpenGL 3.3 Core + shdc)

This document summarizes the core patterns, methods, and structures extracted from the `sokol-samples` repository, tailored for the **OpenGL 3.3 Core** and **sokol-shdc** mandatory workflow.

---

## 1. Application Lifecycle (`sokol_app.h`)

The entry point and application management use `sokol_app.h`.

### Core Structure: `sokol_main`
```c
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,             // Called once at startup
        .frame_cb = frame,           // Called every frame
        .cleanup_cb = cleanup,       // Called once at shutdown
        .event_cb = input,           // (Optional) Input event handler
        .width = 1280,
        .height = 720,
        .window_title = "Sokol Project",
        .icon.sokol_default = true,
        .logger.func = slog_func,    // Standard logging
    };
}
```

### Lifecycle Callbacks
- **`init`**: Initialize `sokol_gfx`, create buffers, pipelines, and shaders.
- **`frame`**: Update state, begin passes, apply pipelines/bindings/uniforms, draw, end passes, and commit.
- **`cleanup`**: Shutdown `sokol_gfx` and other libraries.
- **`input`**: Handle `sapp_event` (mouse, keyboard, etc.).

---

## 2. Graphics Initialization (`sokol_gfx.h`)

### Setup
Always use `sglue_environment()` for the environment setup to ensure portability.
```c
static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
}
```

### Shutdown
```c
static void cleanup(void) {
    sg_shutdown();
}
```

---

## 3. Mandatory Shader Workflow (`sokol-shdc`)

All shaders must be written in GLSL and precompiled via `sokol-shdc`.

### GLSL Source Patterns (`xxx.glsl`)
Use annotations like `@vs`, `@fs`, `@block`, and `@program`.
```glsl
@ctype mat4 mat4s_t  // Map GLSL mat4 to a C/C++ type (e.g., from GLM)

@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};

in vec4 position;
in vec4 color0;
out vec4 color;

void main() {
    gl_Position = mvp * position;
    color = color0;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program my_shader vs fs
```

---

## 4. Resource Management

### Vertex and Index Buffers
```c
// Vertex Buffer
float vertices[] = { ... };
sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(vertices),
    .label = "vertices"
});

// Index Buffer
uint16_t indices[] = { 0, 1, 2, ... };
sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices),
    .label = "indices"
});
```

### Pipeline State Object (PSO)
The pipeline defines the vertex layout, shader, and render states.
```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .attrs = {
            [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
            [ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4
        }
    },
    .index_type = SG_INDEXTYPE_UINT16,
    .cull_mode = SG_CULLMODE_BACK,
    .depth = {
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
        .write_enabled = true
    },
    .label = "pipeline"
});
```

### Textures and Samplers (`stb_image`)
Loading a texture involves decoding with `stb_image` and creating an `sg_image`.
```c
int width, height, channels;
unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
sg_image img = sg_make_image(&(sg_image_desc){
    .width = width,
    .height = height,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .data.mip_levels[0] = SG_RANGE(pixels),
    .label = "texture"
});
stbi_image_free(pixels);

sg_sampler smp = sg_make_sampler(&(sg_sampler_desc){
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR,
    .label = "sampler"
});
```

---

## 5. Rendering Loop

### Standard Frame Sequence
```c
static void frame(void) {
    sg_pass_action action = {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_begin_pass(&(sg_pass){ .action = action, .swapchain = sglue_swapchain() });
    sg_apply_pipeline(pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf,
        .views[VIEW_tex] = sg_make_view(&(sg_view_desc){ .texture.image = img }),
        .samplers[SMP_smp] = smp
    });
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
    sg_draw(0, num_elements, 1);
    sg_end_pass();
    sg_commit();
}
```

### Offscreen Rendering
To render to a texture, create a pass with custom attachments.
```c
// 1. Create attachment images (color and depth)
sg_image offscreen_color = sg_make_image(&(sg_image_desc){
    .render_target = true, .width = 512, .height = 512
});
sg_image offscreen_depth = sg_make_image(&(sg_image_desc){
    .render_target = true, .width = 512, .height = 512, .pixel_format = SG_PIXELFORMAT_DEPTH
});

// 2. Create the pass
sg_pass offscreen_pass = {
    .attachments = {
        .colors[0].image = offscreen_color,
        .depth_stencil.image = offscreen_depth
    },
    .action = { .colors[0].load_action = SG_LOADACTION_CLEAR }
};

// 3. In frame():
sg_begin_pass(&offscreen_pass);
// ... draw calls ...
sg_end_pass();
```

---

## 6. glTF Loading (`cgltf`)

### Parsing and Data Extraction
```c
cgltf_data* data = NULL;
cgltf_result res = cgltf_parse_file(&options, path, &data);
cgltf_load_buffers(&options, data, path);

// Attribute Mapping (Example: Position)
const cgltf_accessor* acc = find_accessor(prim, "POSITION");
void* ptr = (uint8_t*)acc->buffer_view->buffer->data + acc->offset + acc->buffer_view->offset;
```

### Full Mesh Creation Pattern
Extracting data from `cgltf` and mapping it to Sokol buffers:
```c
// 1. Map cgltf attributes to sokol vertex layout
sg_vertex_layout_state layout = { 0 };
for (int i = 0; i < prim->attributes_count; i++) {
    const cgltf_attribute* attr = &prim->attributes[i];
    if (attr->type == cgltf_attribute_type_position) {
        layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
    } else if (attr->type == cgltf_attribute_type_normal) {
        layout.attrs[ATTR_vs_normal].format = SG_VERTEXFORMAT_FLOAT3;
    }
}

// 2. Create buffers from accessor data
sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .data = { .ptr = ptr, .size = acc->count * acc->stride },
    .label = "gltf-vertices"
});
```

---

## 7. Instancing

Instancing requires a separate vertex buffer for per-instance data and a specific step function in the pipeline.

### GLSL for Instancing
```glsl
@vs vs
in vec4 position;      // per-vertex
in vec4 instance_pos;  // per-instance

void main() {
    gl_Position = position + instance_pos;
}
@end
```

### Pipeline Configuration
```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        // Buffer 0: static mesh data (vertices)
        .buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX,
        // Buffer 1: dynamic instance data (positions, matrices)
        .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
        .attrs = {
            [ATTR_vs_position] = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0 },
            [ATTR_vs_instance_pos] = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=1 }
        }
    },
    .shader = sg_make_shader(instancing_shader_desc(sg_query_backend())),
    .index_type = SG_INDEXTYPE_UINT16,
    .label = "instancing-pipeline"
});
```

### Update and Draw
```c
// Update instance data every frame
sg_update_buffer(instance_vbuf, &SG_RANGE(instance_data_array));

// Draw: 36 indices per instance, 1000 instances
sg_bindings bind = {
    .vertex_buffers[0] = mesh_vbuf,
    .vertex_buffers[1] = instance_vbuf,
    .index_buffer = mesh_ibuf
};
sg_apply_bindings(&bind);
sg_draw(0, 36, 1000);
```


---

## 8. Debug UI (`sokol_imgui.h`)

```c
simgui_new_frame({ width, height, dt, 1.0f });
ImGui::Begin("Settings");
ImGui::SliderFloat("Speed", &speed, 0, 10);
ImGui::End();

sg_begin_pass(...);
simgui_render();
sg_end_pass();
```

---

## 9. Best Practices

1. **Alignment**: Uniform structs must be 16-byte aligned for compatibility.
2. **Labels**: Always use `.label` for RenderDoc support.
3. **Usage**: Use `SG_USAGE_IMMUTABLE` for static data; `SG_USAGE_STREAM` for per-frame data.
4. **Shaders**: Prefer `@ctype mat4 glm::mat4` to use GLM types directly in generated C headers.
5. **Memory**: Clear `cgltf_data` only after all GPU buffers are created.

---

## 10. API Reference Quick Look

| Function | Key Parameters |
| :--- | :--- |
| `sg_make_buffer` | `.data`, `.usage`, `.type` |
| `sg_make_image` | `.render_target`, `.width`, `.height`, `.data` |
| `sg_make_shader` | `xxx_shader_desc(sg_query_backend())` |
| `sg_make_pipeline` | `.layout`, `.shader`, `.index_type`, `.depth` |
| `sg_begin_pass` | `.attachments`, `.action`, `.swapchain` |
| `sg_apply_uniforms`| `UB_slot`, `SG_RANGE(data)` |
| `sg_draw` | `base`, `count`, `num_instances` |
| `sg_commit` | No parameters. Call at end of frame. |
