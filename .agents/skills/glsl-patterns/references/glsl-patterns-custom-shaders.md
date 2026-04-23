# GLSL Patterns / Custom Shader Hooks

Open this when implementing the renderer's custom shader hook (R-M5) or wiring game-workstream shaders into the renderer pipeline. Covers the extensible material architecture, the shared vertex contract, and error handling. For the actual game FX shader implementations (plasma, explosion, shield, laser), see `glsl-patterns-game-fx.md`.

---

## 1. Custom Shader Hook Architecture (R-M5)

The renderer supports engine-supplied custom fragment shaders attached to a material handle. The VS is always the renderer's standard template; the game workstream provides a complete `@program` (VS + FS in one `.glsl` file) that follows the standard VS interface.

```
Renderer owns:                      Game workstream owns:
┌────────────────────────┐          ┌──────────────────────────────┐
│  Standard Vertex       │          │  Custom .glsl program         │
│  (mvp, model,          │ ──────►  │  VS: same interface as        │
│   normal_mat uniforms) │  wires   │       renderer standard       │
│                        │          │  FS: custom effect            │
│  Pipeline creation     │          │  @program hook vs custom_fs   │
│  Magenta fallback      │          └──────────────────────────────┘
└────────────────────────┘
```

**Contract:** Every game FX `.glsl` program must use a VS whose uniform block (`vs_params`) and vertex attributes match the renderer's standard layout exactly. This ensures the same vertex buffers work across all pipelines.

### Standard VS contract (must match exactly)

```glsl
// Required VS uniform block — same struct as all renderer pipelines
uniform vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;  // CPU-computed transpose(inverse(mat3(model))), packed as mat4
};

// Required vertex attributes — same layout as renderer vertex buffers
in vec3 position;
in vec3 normal;
in vec2 texcoord0;

// Required varyings — FS may or may not use all of them
out vec3 world_pos;
out vec3 world_normal;
out vec2 uv;
```

### Template: custom fragment shader with shared scene uniforms

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

// ---- Standard vertex shader (renderer contract) ----
@vs vs
uniform vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

out vec3 world_pos;
out vec3 world_normal;
out vec2 uv;

void main() {
    world_pos    = (model * vec4(position, 1.0)).xyz;
    world_normal = normalize(mat3(normal_mat) * normal);
    uv           = texcoord0;
    gl_Position  = mvp * vec4(position, 1.0);
}
@end

// ---- Shared scene uniforms (optional; use @block if shared across FX) ----
@block scene_uniforms
uniform scene_params {
    vec4 light_dir_ws;        // .xyz = FROM light, .w = unused
    vec4 light_color_inten;   // .xyz = color, .w = intensity
};
@end

// ---- Custom fragment shader ----
@fs custom_fs
@include_block scene_uniforms

uniform custom_params {
    vec4 base_color;   // base color + alpha
    vec4 anim_params;  // .x = time, .y = intensity, .zw = custom use
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    frag_color = vec4(base_color.rgb, base_color.a * anim_params.y);
}
@end

@program custom_hook vs custom_fs
```

---

## 2. Design Rules for Custom Shaders

1. **Match the VS contract exactly.** The renderer creates pipelines once; the vertex buffer layout is fixed. A custom VS with different attributes will produce garbage or crash.
2. **All vec3 quantities go in vec4 fields.** `glm::vec3` is 12 bytes; std140 `vec3` is 16 bytes. Mixing them silently corrupts uniforms. Pack everything as `vec4` and swizzle `.xyz` in GLSL.
3. **Let sokol-shdc generate C structs.** Use `@ctype mat4 glm::mat4`, `@ctype vec4 glm::vec4`. Never hand-pad uniform structs — mismatches produce black screens or wrong colors.
4. **Use `@block` / `@include_block` for shared scene uniforms.** Avoids duplicating light/camera data across multiple game FX shaders.
5. **The renderer owns pipeline creation and the magenta fallback.** If a custom shader fails compilation, the renderer substitutes a magenta pipeline — never crashes. The game workstream never sees the failure directly.

---

## 3. Uniform Conventions for Custom FS Shaders

| Field | Type | Purpose |
|-------|------|---------|
| `base_color` | `vec4` | Base color + alpha |
| `anim_params.x` | `float` (in `vec4`) | Animation time |
| `anim_params.y` | `float` (in `vec4`) | Intensity / fade factor |
| `view_pos_w.xyz` | `vec3` (in `vec4`) | Camera world position (view-dependent FX) |

**Slot naming:** sokol-shdc generates `SLOT_<block_name>` constants (no `SG_` prefix). Use these with `sg_apply_uniforms(SLOT_custom_params, &SG_RANGE(cp))`.

---

## 4. Shader Error Handling

When a custom shader fails pipeline creation:

1. Detect failure: `sg_query_shader_state(shader) == SG_RESOURCESTATE_FAILED`.
2. Log via the custom logger registered in `sg_setup()` — there is no `sg_report_error()` function.
3. Bind the magenta fallback pipeline: `sg_make_pipeline(magenta_pip_desc)` with a solid-color FS that outputs `vec4(1.0, 0.0, 1.0, 1.0)`.
4. The object renders magenta — visible, obviously broken, no crash.

---

## 5. Companion Files

- `.agents/skills/glsl-patterns/SKILL.md` — dialect, uniform layout rules, depth/alpha table
- `.agents/skills/glsl-patterns/references/glsl-patterns-lighting.md` — standard VS template, unlit/Lambertian/Phong
- `.agents/skills/glsl-patterns/references/glsl-patterns-special-shaders.md` — skybox, line-quad
- `.agents/skills/glsl-patterns/references/glsl-patterns-game-fx.md` — complete game FX programs (plasma, explosion, shield, laser)
- `.agents/skills/sokol-shdc/SKILL.md` — annotation dialect, CLI flags, CMake integration
- `.agents/skills/sokol-api/` — sokol_gfx resource creation and draw calls
