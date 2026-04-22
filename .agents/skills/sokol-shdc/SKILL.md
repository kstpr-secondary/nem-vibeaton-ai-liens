---
name: sokol-shdc
description: Use when authoring, compiling, or integrating sokol-shdc shaders. Covers the .glsl annotation dialect (@vs, @fs, @block, @include_block, @program, @ctype), the sokol-shdc CLI, the generated .glsl.h header API, and the CMake custom command that drives compilation. Activated whenever writing shaders/renderer/*.glsl or shaders/game/*.glsl files, or wiring up sokol-shdc in CMakeLists.txt.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: library-reference
  activated-by: shader authoring or CMake shader compilation
---

# sokol-shdc Skill

Reference for the **mandatory** sokol shader cross-compiler used in this project.
All shaders must be precompiled via sokol-shdc. No runtime GLSL loading. No hot-reload.

---

## 1. Core Rules (must know)

1. **Every `.glsl` file** in `shaders/{renderer,game}/` must be processed by sokol-shdc before the build.
2. **Output headers** land in `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. Include them with `#include "shaders/<name>.glsl.h"` (path configured via CMake include dirs).
3. **Never call `sg_make_shader` with a hand-crafted `sg_shader_desc`.** Always use the generated `<program>_shader_desc(sg_query_backend())`.
4. **Never load GLSL at runtime.** The shader is a compile-time dependency.
5. **Shader creation failure → log + fallback to magenta error pipeline.** Never crash.
6. **Backend flag for OpenGL 3.3 Core:** use `--slang glsl430` (works for GL 3.3+ Core because sokol-shdc emits compatible GLSL; `glsl330` is also acceptable).

---

## 2. GLSL Annotation Dialect

### 2.1 Basic structure

```glsl
@vs vs_name
// vertex shader GLSL code
@end

@fs fs_name
// fragment shader GLSL code
@end

@program program_name vs_name fs_name
```

`@program` creates the named shader pair. The generated C function will be `program_name_shader_desc(sg_backend)`.

### 2.2 Uniform blocks

Declare uniforms in a `uniform` block inside the shader stage:

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;
    mat4 model;
};

in vec3 position;
in vec2 texcoord0;
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    uv = texcoord0;
}
@end
```

The generated header provides a C struct `vs_params_t` and a bind-slot constant `SG_SLOT_vs_params`.

### 2.3 Textures and samplers

```glsl
@fs fs
uniform texture2D tex;
uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end
```

Generated constants: `SG_SLOT_tex` (view slot), `SG_SLOT_smp` (sampler slot).

### 2.4 @block — shared uniform definitions

Use `@block` to define uniforms shared across vertex and fragment stages:

```glsl
@block shared_params
uniform scene_params {
    mat4 view_proj;
    vec3 light_dir;
    float ambient;
};
@end

@vs vs
@include_block shared_params

uniform vs_local {
    mat4 model;
};
in vec3 position;
in vec3 normal;
out vec3 world_normal;

void main() {
    gl_Position = view_proj * model * vec4(position, 1.0);
    world_normal = mat3(model) * normal;
}
@end

@fs fs
@include_block shared_params

in vec3 world_normal;
out vec4 frag_color;

void main() {
    float NdotL = max(dot(normalize(world_normal), light_dir), 0.0);
    frag_color = vec4(vec3(ambient + NdotL), 1.0);
}
@end

@program lit_shader vs fs
```

### 2.5 @ctype — map GLSL types to C/C++ types

Use `@ctype` at file top to tell sokol-shdc what C type to use in generated structs:

```glsl
@ctype mat4 glm::mat4   // use GLM mat4 in the generated C struct
@ctype mat4 mat4s_t     // alternative: map to a custom math-lib type
@ctype vec3 glm::vec3
@ctype vec2 glm::vec2
```

This means `vs_params_t.mvp` will be `glm::mat4` instead of a raw float array.

### 2.6 Instancing — per-instance vertex attributes

```glsl
@vs vs
in vec3 position;      // per-vertex  (buffer slot 0)
in vec2 texcoord0;     // per-vertex  (buffer slot 0)
in vec3 inst_pos;      // per-instance (buffer slot 1)
in float inst_scale;   // per-instance (buffer slot 1)

uniform vs_params {
    mat4 view_proj;
};

out vec2 uv;

void main() {
    vec3 world_pos = position * inst_scale + inst_pos;
    gl_Position = view_proj * vec4(world_pos, 1.0);
    uv = texcoord0;
}
@end
```

---

## 3. sokol-shdc CLI

```bash
sokol-shdc \
    --input  shaders/renderer/my_shader.glsl \
    --output ${BUILD_DIR}/generated/shaders/my_shader.glsl.h \
    --slang  glsl430
```

| Flag | Purpose |
|------|---------|
| `--input` | Source `.glsl` file with annotations |
| `--output` | Generated `.glsl.h` header path |
| `--slang` | Target shading language(s). Use `glsl430` for OpenGL 3.3 Core |
| `--module` | Optional prefix for generated symbol names |

For multi-backend (not used in this project): `--slang glsl430:hlsl5:metal_macos`.

---

## 4. Generated Header — What You Get

After compilation, the header provides:

```c
// 1. Shader descriptor function (pass to sg_make_shader):
const sg_shader_desc* my_shader_shader_desc(sg_backend backend);

// 2. Uniform block C structs (if @ctype used, fields are GLM types):
typedef struct vs_params_t { glm::mat4 mvp; glm::mat4 model; } vs_params_t;
typedef struct fs_params_t { glm::vec3 light_dir; float ambient; } fs_params_t;

// 3. Uniform block bind slot constants:
#define UB_vs_params  0
#define UB_fs_params  1

// 4. Texture/sampler slot constants:
#define VIEW_tex        0
#define SMP_smp        0

// 5. Vertex attribute slot constants:
#define ATTR_vs_position   0
#define ATTR_vs_texcoord0  1
#define ATTR_vs_normal     2
```

---

## 5. Using the Generated Header in C++

### 5.1 Create shader and pipeline

```cpp
#include "shaders/my_shader.glsl.h"   // generated by sokol-shdc

// Shader
sg_shader shd = sg_make_shader(my_shader_shader_desc(sg_query_backend()));
if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
    // log error, create magenta fallback pipeline
}

// Pipeline
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
    .label = "my-pipeline",
});
```

### 5.2 Upload uniforms

```cpp
// Per-frame, before draw:
vs_params_t vs_p = {
    .mvp   = proj * view * model,
    .model = model,
};
sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_p));

fs_params_t fs_p = {
    .light_dir = glm::vec3(0.577f, 0.577f, 0.577f),
    .ambient   = 0.2f,
};
sg_apply_uniforms(UB_fs_params, &SG_RANGE(fs_p));
```

### 5.3 Bind textures and samplers

```cpp
sg_bindings bind = {
    .vertex_buffers[0] = vbuf,
    .index_buffer = ibuf,
    .views[VIEW_tex]    = sg_make_view(&(sg_view_desc){ .texture.image = img }),
    .samplers[SMP_smp] = smp,
};
sg_apply_bindings(&bind);
```

---

## 6. CMake Integration

```cmake
# 1. Find the sokol-shdc binary (provided via sokol-tools-bin)
find_program(SOKOL_SHDC sokol-shdc
    PATHS ${SOKOL_TOOLS_BIN_DIR}
    REQUIRED
)

# 2. Create output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated/shaders)

# 3. Custom command per shader file
add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/generated/shaders/my_shader.glsl.h
    COMMAND ${SOKOL_SHDC}
            --input  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/renderer/my_shader.glsl
            --output ${CMAKE_BINARY_DIR}/generated/shaders/my_shader.glsl.h
            --slang  glsl430
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/renderer/my_shader.glsl
    COMMENT "sokol-shdc: compiling my_shader.glsl"
    VERBATIM
)

# 4. Collect all generated headers into a target
add_custom_target(renderer_shaders
    DEPENDS
        ${CMAKE_BINARY_DIR}/generated/shaders/my_shader.glsl.h
        # add more shaders here
)

# 5. Make your library depend on the shaders target
add_library(renderer STATIC ...)
add_dependencies(renderer renderer_shaders)

# 6. Add generated include dir so #include "shaders/xxx.glsl.h" resolves
target_include_directories(renderer PRIVATE
    ${CMAKE_BINARY_DIR}/generated
)
```

**CMakeLists.txt is owned by the Renderer / Systems Architect.** Cross-workstream changes require a 2-minute notice.

---

## 7. Common Pitfalls

| Problem | Cause | Fix |
|---------|-------|-----|
| `SG_RESOURCESTATE_FAILED` on shader | Wrong `--slang` flag or GLSL syntax error | Check sokol-shdc stdout; use `glsl430` |
| `ATTR_vs_*` undefined | Wrong generated header included, or `@program` name mismatch | Verify `#include` path and `@program` name |
| Uniform struct misaligned | `std140` layout requires 16-byte alignment | Add explicit padding or use `SG_UNIFORMLAYOUT_STD140` |
| Black screen after pipeline creation | Pipeline valid but bindings wrong | Check `UB_*`, `VIEW_*`, or `SMP_*` constants match the bindings |
| Generated header not found at build | CMake include dir not set | Add `target_include_directories(... ${CMAKE_BINARY_DIR}/generated)` |

---

## 8. Magenta Error Pipeline Recipe

```cpp
// Call when sg_make_pipeline or sg_make_shader returns FAILED state
sg_pipeline make_error_pipeline() {
    // Inline minimal passthrough shaders (no sokol-shdc needed for fallback)
    sg_shader err_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source =
            "#version 330\n"
            "in vec4 position;\n"
            "void main() { gl_Position = position; }\n",
        .fragment_func.source =
            "#version 330\n"
            "out vec4 frag_color;\n"
            "void main() { frag_color = vec4(1,0,1,1); }\n",
        .label = "error-shader",
    });
    return sg_make_pipeline(&(sg_pipeline_desc){
        .shader = err_shd,
        .layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3,
        .label = "error-pipeline",
    });
}
```
