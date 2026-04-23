# sokol-shdc Tool Summary

> Distilled reference for the [Vibe Coding Hackathon](../nem-vibeaton-ai-liens/pre_planning_docs/Hackathon%20Master%20Blueprint.md) project.
> Graphics target: **OpenGL 3.3 Core** (`glsl410`). Vulkan/HLSL/Metal are out of scope.

---

## What sokol-shdc Does

`sokol-shdc` is a shader cross-compiler and C-header generator for `sokol_gfx`. It takes annotated Vulkan-style GLSL 450 source files and produces C headers containing:

- Embedded shader source strings (or bytecode) for every target backend
- C structs mirroring each uniform block
- Constants for vertex attribute slots, uniform binding indices, texture/sampler indices
- A `sg_shader_desc` getter function per `@program` — the only thing you pass to `sg_make_shader()`

**Pipeline:** `.glsl` → glslang → SPIR-V → SPIRV-Cross → `glsl410` source → `.glsl.h`

---

## Essential CLI Flags

```bash
sokol-shdc \
  --input  shaders/renderer/lambertian.glsl \   # annotated GLSL source (required)
  --output generated/shaders/lambertian.glsl.h \ # output header path (required)
  --slang  glsl410 \                             # target language(s), comma-separated
  --format sokol                                 # output format (default: sokol)
```

| Flag | Short | Description |
|------|-------|-------------|
| `--input=<file>` | `-i` | Annotated Vulkan-GLSL source file |
| `--output=<path>` | `-o` | Output header path |
| `--slang=<langs>` | `-l` | Comma-separated output backends (see below) |
| `--format=<fmt>` | `-f` | Code format: `sokol`, `sokol_impl`, `bare`, `bare_yaml` |
| `--ifdef` | | Wrap backend code in `#ifdef SOKOL_GLCORE` / `#ifdef SOKOL_GLES3` etc. |
| `--reflection` | `-r` | Generate runtime reflection inspection functions |
| `--bytecode` | `-b` | Compile HLSL/Metal to bytecode (not needed for GL) |
| `--errfmt=gcc\|msvc` | `-e` | Error format for IDE integration (default: `gcc`) |
| `--defines=A:B:C` | | Preprocessor defines for conditional shader variants |
| `--module=<name>` | `-m` | Override the `@module` name |
| `--dependency-file=<path>` | | Emit `.d` depfile for build system incremental support |
| `--no-log-cmdline` | | Omit command-line comment from output (cleaner git diffs) |
| `--dump` | `-d` | Verbose debug dump to stderr |

### Target Backend (`--slang`) Values

| Value | Backend | Notes |
|-------|---------|-------|
| **`glsl410`** | **OpenGL 4.1 / GL 3.3 Core** | **Use this — no compute, no SSBOs** |
| `glsl430` | OpenGL 4.3+ | SSBOs and compute shaders |
| `glsl300es` | GLES3 / WebGL2 | No compute |
| `glsl310es` | GLES3.1 | Compute support |
| `hlsl5` | Direct3D 11 | Out of scope |
| `metal_macos` | Metal macOS | Out of scope |
| `wgsl` | WebGPU | Out of scope |
| `spirv_vk` | Vulkan SPIR-V | Out of scope |

---

## Shader File Annotations

Input files are Vulkan-style GLSL 450 with special `@`-tags (or `#pragma sokol @tag`):

### Shader Stage Blocks

```glsl
@vs vs_name
// vertex shader GLSL code
@end

@fs fs_name
// fragment shader GLSL code
@end

@cs cs_name
// compute shader (glsl430/glsl310es only — not used for GL 3.3 Core target)
@end
```

### Code Reuse

```glsl
@block block_name
// shared code: structs, functions, uniform blocks
@end

// Inside a @vs or @fs:
@include_block block_name

// Include from filesystem:
@include path/to/file.glsl
```

### Program Linking

```glsl
@program prog_name vs_name fs_name   // vertex + fragment
@program comp_prog  cs_name          // compute only
```

Generated C function: `shd_prog_name_shader_desc(sg_query_backend())`

### Module Namespace

```glsl
@module mymod
```

Prefixes all generated identifiers: `mymod_prog_name_shader_desc(...)`, `MYMOD_ATTR_vs_position`, etc.

### C Type Mapping

```glsl
@ctype mat4 HMM_Mat4       // map GLSL mat4 → C type in uniform struct
@ctype vec3 HMM_Vec3
@ctype float float         // explicit passthrough
```

Valid GLSL types: `float`, `vec2`, `vec3`, `vec4`, `int`, `ivec2`, `ivec3`, `ivec4`, `mat4`

### Target-Language Header Injection

```glsl
@header #include "mymath.h"   // injected into the generated C header
```

### Per-Backend SPIRV-Cross Options

```glsl
// Inside @vs block:
@glsl_options fixup_clipspace    // adjust clip-space for GL (usually not needed)
@glsl_options flip_vert_y        // flip Y in vertex output
@msl_options fixup_clipspace     // same, for Metal output
```

### Texture / Sampler Type Hints

```glsl
@image_sample_type tex_name   float              // default
@image_sample_type depth_tex  depth
@image_sample_type int_tex    sint
@sampler_type smp_name        filtering           // default
@sampler_type smp_name        nonfiltering
@sampler_type smp_name        comparison
```

---

## Resource Binding Rules

All resources use `layout(binding=N)`. Limits:

| Resource | Binding Range | Notes |
|----------|---------------|-------|
| Uniform blocks | 0–7 | `layout(binding=N) uniform BlockName { ... }` |
| Textures / images | 0–31 | `layout(binding=N) uniform texture2D name;` |
| Samplers | 0–11 | `layout(binding=N) uniform sampler name;` |
| Storage buffers | 0–7 | `layout(binding=N) readonly buffer ...` (glsl430+) |

Keep textures and samplers **separate** (Vulkan-style). SPIRV-Cross combines them for GLSL output.

---

## Generated Output — What You Consume in C++

Given `@module rnd` and `@program lambertian vs_main fs_main`:

```c
// 1. Include the generated header
#include "generated/shaders/lambertian.glsl.h"

// 2. Vertex attribute slot constants
//    rnd_ATTR_vs_main_position, rnd_ATTR_vs_main_normal, ...

// 3. Uniform binding index constants
//    rnd_UB_vs_params, rnd_UB_fs_params, ...

// 4. Texture/sampler index constants
//    rnd_VIEW_diffuse_tex, rnd_SMP_diffuse_smp, ...

// 5. C struct mirroring each uniform block
typedef struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} rnd_vs_params_t;

// 6. sg_shader_desc getter — pass directly to sg_make_shader()
sg_shader sh = sg_make_shader(rnd_lambertian_shader_desc(sg_query_backend()));
```

### Applying Uniforms at Runtime

```cpp
rnd_vs_params_t vs_params = { .mvp = mvp, .model = model };
sg_apply_uniforms(rnd_UB_vs_params, SG_RANGE(vs_params));
```

---

## CMake Integration

The `sokol-tools` repo ships CMake helper macros (via fips/fibs integration):

```cmake
# Basic — compile shader, generate header
sokol_shader(shaders/renderer/lambertian.glsl glsl410)

# With runtime reflection functions
sokol_shader_with_reflection(shaders/renderer/lambertian.glsl glsl410)

# Conditional variant (e.g., skinned vs. unskinned)
sokol_shader_variant(shaders/renderer/lambertian.glsl glsl410 rnd SKINNING)

# Debuggable — no binary blobs embedded
sokol_shader_debuggable(shaders/renderer/lambertian.glsl glsl410)
```

For the hackathon project using CMake + FetchContent, the equivalent custom command:

```cmake
add_custom_command(
    OUTPUT  ${CMAKE_BINARY_DIR}/generated/shaders/lambertian.glsl.h
    COMMAND sokol-shdc
            --input  ${CMAKE_SOURCE_DIR}/shaders/renderer/lambertian.glsl
            --output ${CMAKE_BINARY_DIR}/generated/shaders/lambertian.glsl.h
            --slang  glsl410
            --format sokol
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/renderer/lambertian.glsl
    COMMENT "Compiling lambertian.glsl"
)
```

Add the generated include dir as a target include path:
```cmake
target_include_directories(renderer PRIVATE ${CMAKE_BINARY_DIR}/generated/shaders)
```

---

## Minimal Complete Shader Example

```glsl
// shaders/renderer/lambertian.glsl
#pragma sokol @module rnd

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@block vs_uniforms
layout(binding=0) uniform vs_params {
    mat4 mvp;
    mat4 model;
};
@end

@block fs_uniforms
layout(binding=1) uniform fs_params {
    vec3 light_dir;
    vec3 light_color;
    vec3 base_color;
};
@end

@vs vs_main
@include_block vs_uniforms

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;

out vec3 frag_normal;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    frag_normal = normalize(mat3(model) * normal);
}
@end

@fs fs_main
@include_block fs_uniforms

in  vec3 frag_normal;
out vec4 frag_color;

void main() {
    float diffuse = max(dot(normalize(frag_normal), normalize(-light_dir)), 0.0);
    frag_color = vec4(base_color * light_color * diffuse, 1.0);
}
@end

@program lambertian vs_main fs_main
```

---

## Conditional Compilation Inside Shaders

Use preprocessor defines passed via `--defines=SKINNING` or the `sokol_shader_variant` macro:

```glsl
#ifdef SKINNING
    // skinning-specific code
#else
    gl_Position = mvp * vec4(position, 1.0);
#endif
```

Backend detection macros (available inside sokol-shdc input):
- `#if SOKOL_GLSL` — any GLSL output
- `#if SOKOL_HLSL`
- `#if SOKOL_MSL`
- `#if SOKOL_WGSL`

---

## Quick Reference: Annotation Summary

| Annotation | Scope | Purpose |
|-----------|-------|---------|
| `@module name` | File | Namespace prefix for all generated identifiers |
| `@vs name` / `@end` | File | Vertex shader block |
| `@fs name` / `@end` | File | Fragment shader block |
| `@cs name` / `@end` | File | Compute shader block (glsl430+) |
| `@block name` / `@end` | File | Reusable code block |
| `@include_block name` | Inside stage | Insert a `@block` by name |
| `@include path` | Inside stage | Include file from filesystem |
| `@program name vs fs` | File | Link vertex+fragment into program |
| `@ctype glsl_type c_type` | File | Map GLSL type to C type in uniform structs |
| `@header statement` | File | Inject statement into generated C header |
| `@glsl_options opt` | Inside `@vs` | SPIRV-Cross GLSL-specific option |
| `@msl_options opt` | Inside `@vs` | SPIRV-Cross MSL-specific option |
| `@image_sample_type tex type` | File | Texture sample type hint |
| `@sampler_type smp type` | File | Sampler type hint |

---

## Error Handling Pattern (Project Rule)

Per §3.3 of the Master Blueprint, `sg_make_shader` / `sg_make_pipeline` failures must **never crash**:

```cpp
sg_shader sh = sg_make_shader(rnd_lambertian_shader_desc(sg_query_backend()));
if (sh.id == SG_INVALID_ID) {
    // log error, fall back to magenta error pipeline
    return make_error_pipeline();
}
```

This is an acceptance criterion for every shading milestone.

---

## Source Location in This Repo

| Path | Contents |
|------|----------|
| `src/shdc/main.cc` | Entry point — orchestrates the pipeline |
| `src/shdc/args.cc` | CLI argument parsing |
| `src/shdc/input.cc` | `@`-tag parser — reads and splits annotated GLSL |
| `src/shdc/spirv.cc` | GLSL → SPIR-V via glslang |
| `src/shdc/spirvcross.cc` | SPIR-V → target dialect via SPIRV-Cross |
| `src/shdc/reflection.cc` | Extracts uniform/attribute reflection data |
| `src/shdc/generators/sokolc.cc` | C header generator (sokol format) |
| `src/shdc/types/slang.h` | `slang_t` enum — all supported backend values |
| `src/shdc/types/snippet.h` | `@block` / `@vs` / `@fs` AST node types |
| `src/shdc/types/program.h` | `@program` linking model |
| `test/` | Reference `.glsl` input files with real annotation usage |
