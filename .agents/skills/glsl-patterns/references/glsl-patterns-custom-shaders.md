# GLSL Patterns / Custom Shader Hooks & Game FX

Open this when authoring game-workstream fragment shaders (VFX, projectiles, effects) or implementing the renderer's custom shader hook (R-M5). Covers the extensible material pipeline, game FX shader templates, and uniform conventions for animated effects. For lighting pipelines, see `glsl-patterns-lighting.md`. For skybox/line-quads, see `glsl-patterns-special-shaders.md`.

---

## 1. Custom Shader Hook Pattern (R-M5)

The renderer supports engine-supplied custom fragment shaders attached to a material handle. The vertex shader is always the standard one; the fragment shader is swapped per material.

### Architecture

```
Renderer owns:                      Game/Engine owns:
┌─────────────────────┐             ┌──────────────────────────┐
│  Standard Vertex     │             │  Custom Fragment Shader   │
│  (shared template)   │  ──────►   │  (shaders/game/*.glsl)    │
│                      │   wires    │  + custom uniforms        │
│  Pipeline creation   │             │                          │
│  Error fallback      │             │  @program hook vs custom_fs
│  magenta fallback    │             └──────────────────────────┘
└─────────────────────┘
```

### Shader template — shared vertex + custom fragment

```glsl
// ---- Vertex (shared, renderer-owned) ----
@vs vs
uniform vs_params {
    mat4 mvp;
    mat4 model;
};

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

out vec3 world_pos;
out vec3 world_normal;
out vec2 uv;

void main() {
    world_pos = (model * vec4(position, 1.0)).xyz;
    mat3 normal_mat = mat3(transpose(inverse(model)));
    world_normal = normalize(normal_mat * normal);
    uv = texcoord0;
    gl_Position = mvp * vec4(position, 1.0);
}
@end

// ---- Custom Fragment (game-workstream) ----
@block common_fs
uniform scene_params {
    vec3 light_dir;
    vec3 light_color;
    float light_intensity;
};
@end

@fs custom_fs
@include_block common_fs

uniform custom_params {
    vec4 base_color;
    float time;       // animated parameter from engine
    float intensity;  // fade factor / alpha multiplier
};

// Optional textures
uniform texture2D noise_tex;
uniform sampler smp;

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    // Custom effect logic here
    frag_color = vec4(base_color.rgb, intensity);
}
@end

@program custom_hook vs custom_fs
```

### Design rules for custom shaders

1. **Always inherit the standard vertex shader structure.** The renderer wires the VS once; game code only provides the FS.
2. **Use `time` and `intensity` uniforms** for animation. The engine passes per-frame increments.
3. **Let `sokol-shdc` generate the C struct** via `@ctype`. Never hand-pad C uniform structs.
4. **The renderer owns pipeline creation.** If a custom shader fails to compile, the renderer returns the magenta fallback pipeline. Never crash.
5. **Fragment-only entry points.** Use `@block` / `@include_block` to share common uniforms across multiple game FX shaders.

---

## 2. Game FX Shader Templates

### 2.1 Plasma Beam (Projectile VFX)

Animated plasma effect for projectile rendering. Uses time-based noise animation on a quad.

```glsl
@fs plasma_beam_fs
uniform fs_params {
    vec4 base_color;      // plasma color (bright cyan/teal)
    float time;           // global game time or per-frame increment
    float intensity;      // alpha / brightness factor
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    float n = noise(uv * 6.0 + time * 3.0);
    float core = smoothstep(0.3, 0.7, n);
    vec3 col = mix(base_color.rgb * 0.5, vec3(1.0), core);
    frag_color = vec4(col, intensity * (0.6 + n * 0.4));
}
@end

@program plasma_beam vs plasma_beam_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color   = glm::vec4(0.0f, 1.0f, 0.8f, 1.0f), // cyan
    .time         = game_time,
    .intensity    = 1.0f,
};
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

---

### 2.2 Explosion Effect

Burst effect on a quad — expands outward with fade. Used when an asteroid or ship is destroyed.

```glsl
@fs explosion_fs
uniform fs_params {
    vec4 base_color;      // explosion color (warm orange/yellow)
    float time;           // 0.0 = just spawned, 1.0 = fully faded
    float intensity;      // initial brightness multiplier
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    float d = length(uv - 0.5) * 2.0;  // 0 at center, 1 at edges

    float expand = smoothstep(0.0, 0.3, time);
    float contract = smoothstep(0.3, 1.0, time);
    float brightness = 1.0 - contract;

    float radial = 1.0 - smoothstep(0.0, expand + 0.3 * (1.0 - expand), d);

    float flicker = fract(sin(dot(uv + time, vec2(127.1, 311.7))) * 43758.5453);

    vec3 col = base_color.rgb * (radial * brightness + flicker * 0.2 * brightness);
    float alpha = intensity * brightness * radial;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program explosion vs explosion_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color    = glm::vec4(1.0f, 0.6f, 0.1f, 1.0f), // orange-yellow
    .time          = explosion_age / explosion_lifetime, // 0.0 → 1.0
    .intensity     = 1.0f,
};
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

---

### 2.3 Shield VFX

Translucent dome/hexagonal shield effect around the player ship. Uses fresnel-like rim lighting.

```glsl
@fs shield_fs
uniform fs_params {
    vec4 base_color;      // shield color (blue/cyan)
    float time;           // subtle animation
    float health;         // 0.0 to 1.0 — alpha scales with HP
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3 V = normalize(vec3(0.0, 0.0, 1.0));  // simplified: view from +Z
    vec3 N = normalize(world_normal);
    float rim = 1.0 - max(dot(N, V), 0.0);
    rim = pow(rim, 2.0);

    float pulse = sin(time * 2.0) * 0.1 + 0.9;

    vec3 col = base_color.rgb * (rim * 0.8 + 0.2) * pulse;
    float alpha = rim * health * 0.7;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program shield vs shield_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f), // blue
    .time       = game_time,
    .health     = ship_health / max_health,            // 0.0 → 1.0
};
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

---

### 2.4 Laser Beam (Game Workstream)

Thin alpha-blended line quad using the line-quad vertex shader with a glow fragment.

```glsl
@fs laser_fs
uniform fs_params {
    vec4 base_color;      // laser color (red for player, yellow for enemy)
    float intensity;      // fade over distance/lifetime
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    float edge = smoothstep(0.0, 0.15, min(uv.x, 1.0 - uv.x));
    vec3 col = base_color.rgb * (0.6 + 0.4 * edge);
    frag_color = vec4(col, base_color.a * intensity);
}
@end

@program laser vs laser_fs
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA` / `ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 3. Uniform Conventions for Game FX Shaders

### Common uniform struct fields

| Field | Type | Purpose | Set by |
|-------|------|---------|--------|
| `base_color` | `vec4` | Base color + alpha of the effect | Engine/game |
| `time` | `float` | Per-frame or global game time for animation | Engine |
| `intensity` | `float` | Alpha/brightness multiplier, fades over lifetime | Engine |
| `health` | `float` | 0.0–1.0 for health-dependent VFX (shields) | Game logic |

### Shader file naming convention

```
shaders/game/<effect_name>.frag.glsl    fragment shader only
shaders/game/<effect_name>.glsl         full program (vs + fs) if custom vertex needed
```

**Recommended:** Always use a separate `.frag.glsl` file with the shared vertex from the renderer. This keeps game workstream code minimal.

---

## 4. Shader Error Handling Pattern

When a custom game-workstream shader fails to compile:

1. The renderer catches `SG_RESOURCESTATE_FAILED` from `sg_make_shader`.
2. Logs the error via `sg_report_error(...)`.
3. Creates and returns the magenta fallback pipeline for that material.
4. The object renders as solid magenta — visible but obviously broken.

**Critical:** Never propagate shader compilation errors to the game workstream. The renderer owns the fallback contract. Custom shaders are best-effort.

---

## 5. Quick Reference: Lighting Formulas (for FX that include lighting)

| Model | Formula | Notes |
|-------|---------|-------|
| **Unlit** | `color` | No lighting math |
| **Lambertian** | `max(N.L, 0) x albedo x light_color x intensity + albedo x 0.15` | Diffuse only |
| **Blinn-Phong** | `N.L term + pow(max(N.H, 0), shininess) x spec x 0.5 + ambient` | H = normalize(L + V) |
| **Fresnel rim** | `(1 - N.V)^exponent x color` | Shield VFX style |

**Common vectors:**
- `N`: surface normal (world space, normalized)
- `L`: light direction vector pointing **toward** the light (normalized)
- `V`: view direction = normalize(camera_pos - world_pos) (normalized)
- `H`: half vector = normalize(L + V) (normalized)

---

## 6. Depth and Alpha State Reference

| Pipeline type | depth compare | depth write | cull mode | blend state | Draw order |
|---------------|--------------|-------------|-----------|-------------|------------|
| Opaque (unlit, Lambertian, Phong) | `LESS_EQUAL` | `true` | `BACK` | none | any |
| Skybox | `LESS` | `false` | `NONE` | none | **first** |
| Alpha-blended (line quads, VFX) | `LESS_EQUAL` | `false` | `NONE` | `SRC_ALPHA` / `ONE_MINUS_SRC_ALPHA` | back-to-front |
| Error/fallback pipeline | `LESS_EQUAL` | `true` | `BACK` | none | same as opaque |

**Discard pattern:**
```glsl
if (frag_color.a < 0.01) discard;
```

---

## 7. Companion Files

- `.agents/skills/glsl-patterns/SKILL.md` — main skill: sokol-shdc dialect, uniform block layout rules, shader error handling
- `.agents/skills/glsl-patterns/references/glsl-patterns-lighting.md` — unlit, Lambertian, Blinn-Phong, normal maps
- `.agents/skills/glsl-patterns/references/glsl-patterns-special-shaders.md` — skybox, procedural sky, line quads
- `.agents/skills/sokol-shdc/SKILL.md` — annotation dialect, CLI, CMake integration, generated headers
- `.agents/skills/sokol-api/` — sokol_gfx resource creation, render passes, draw calls
