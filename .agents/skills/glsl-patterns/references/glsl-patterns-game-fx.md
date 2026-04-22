# GLSL Patterns / Game FX Shaders

Open this when authoring or reviewing game workstream FX shaders: plasma beam, explosion, shield VFX, and laser beam. These are complete sokol-shdc programs (VS + FS in one file) residing in `shaders/game/`. They are consumed by the renderer's custom shader hook (R-M5). For the hook architecture and wiring, see `glsl-patterns-custom-shaders.md`. For lighting shaders, see `glsl-patterns-lighting.md`.

**Important:** Every `.glsl` file in `shaders/game/` must be a complete sokol-shdc program — `@vs`, `@fs`, and `@program` must all be present in the same file. sokol-shdc cannot link across files.

---

## 1. Plasma Beam (Game workstream)

Animated plasma effect for projectile rendering. Uses time-based noise on a standard lit quad.

**Shader file: `shaders/game/plasma.glsl`**

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

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

@fs plasma_fs
uniform fs_params {
    vec4 base_color;    // plasma color (e.g., bright cyan/teal)
    vec4 anim_params;   // .x = time (game time), .y = intensity (alpha/brightness), .zw unused
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
    float n    = noise(uv * 6.0 + anim_params.x * 3.0);
    float core = smoothstep(0.3, 0.7, n);
    vec3  col  = mix(base_color.rgb * 0.5, vec3(1.0), core);
    frag_color = vec4(col, anim_params.y * (0.6 + n * 0.4));
}
@end

@program plasma vs plasma_fs
```

**C++ uniform setup:**
```cpp
vs_params_t vs_p = {
    .mvp = proj * view * model_m, .model = model_m,
    .normal_mat = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_m)))),
};
sg_apply_uniforms(SLOT_vs_params, &SG_RANGE(vs_p));

fs_params_t fs_p = {
    .base_color  = glm::vec4(0.0f, 1.0f, 0.8f, 1.0f),  // cyan
    .anim_params = glm::vec4(game_time, 1.0f, 0.0f, 0.0f),
};
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 2. Explosion Effect (Game workstream)

Burst effect on a quad — expands outward with fade. Used when an asteroid or ship is destroyed.

**Shader file: `shaders/game/explosion.glsl`**

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

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

@fs explosion_fs
uniform fs_params {
    vec4 base_color;    // explosion color (warm orange/yellow)
    vec4 anim_params;   // .x = normalized age 0.0–1.0, .y = intensity multiplier, .zw unused
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    float t          = anim_params.x;                            // 0 = spawned, 1 = faded
    float d          = length(uv - 0.5) * 2.0;                  // 0 at center, 1 at edge
    float expand     = smoothstep(0.0, 0.3, t);
    float contract   = smoothstep(0.3, 1.0, t);
    float brightness = 1.0 - contract;
    float radial     = 1.0 - smoothstep(0.0, expand + 0.3 * (1.0 - expand), d);
    float flicker    = fract(sin(dot(uv + t, vec2(127.1, 311.7))) * 43758.5453);

    vec3  col   = base_color.rgb * (radial * brightness + flicker * 0.2 * brightness);
    float alpha = anim_params.y * brightness * radial;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program explosion vs explosion_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color  = glm::vec4(1.0f, 0.6f, 0.1f, 1.0f),
    .anim_params = glm::vec4(explosion_age / explosion_lifetime, 1.0f, 0.0f, 0.0f),
};
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 3. Shield VFX (Game workstream)

Translucent dome shield effect using fresnel-like rim lighting. Requires `view_pos` for correct 3D rim behavior.

**Shader file: `shaders/game/shield.glsl`**

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

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

@fs shield_fs
uniform fs_params {
    vec4 base_color;   // shield color (blue/cyan)
    vec4 view_pos_w;   // .xyz = camera world position (required for correct fresnel), .w unused
    vec4 anim_params;  // .x = time (for pulsing), .y = health (0.0–1.0), .zw unused
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3  N   = normalize(world_normal);
    vec3  V   = normalize(view_pos_w.xyz - world_pos);  // correct view vector per fragment
    float rim = 1.0 - max(dot(N, V), 0.0);
    rim       = pow(rim, 2.0);

    float pulse = sin(anim_params.x * 2.0) * 0.1 + 0.9;
    vec3  col   = base_color.rgb * (rim * 0.8 + 0.2) * pulse;
    float alpha = rim * anim_params.y * 0.7;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program shield vs shield_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color  = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f),
    .view_pos_w  = glm::vec4(camera_world_pos, 0.0f),
    .anim_params = glm::vec4(game_time, ship_health / max_health, 0.0f, 0.0f),
};
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 4. Laser Beam (Game workstream)

Thin alpha-blended line quad with a glow that fades toward the beam edges (across the beam width, not along it).

Uses the **line-quad vertex layout** from `shaders/renderer/line_quad.glsl` (CPU-expanded quad: position + texcoord + color per vertex). UV convention: **u = 0..1 along beam length, v = 0..1 across beam width**. The glow samples `v` for the cross-section fade.

**Shader file: `shaders/game/laser.glsl`**

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

@vs vs
// Matches line_quad vertex buffer layout: position(vec3) + texcoord(vec2) + color(vec4)
uniform vs_params {
    mat4 mvp;
};

in vec3 position;    // CPU-expanded world-space quad vertex
in vec2 texcoord0;   // u=0..1 along beam, v=0..1 across beam width
in vec4 color;       // per-vertex color (RGBA, encodes beam color + alpha)

out vec4 vert_color;
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    vert_color  = color;
    uv          = texcoord0;
}
@end

@fs laser_fs
uniform fs_params {
    vec4 params;  // .x = intensity multiplier (fades over lifetime), .yzw unused
};

in vec4 vert_color;
in vec2 uv;

out vec4 frag_color;

void main() {
    // Glow across beam width: bright at center (v=0.5), dark at edges (v=0 or 1)
    float center_v = 1.0 - abs(uv.y * 2.0 - 1.0);   // 0 at edges, 1 at center
    float glow     = smoothstep(0.0, 0.8, center_v);
    vec3  col      = vert_color.rgb * (0.5 + 0.5 * glow);
    float alpha    = vert_color.a * params.x * (0.2 + 0.8 * glow);
    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program laser vs laser_fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = { .params = glm::vec4(laser_intensity, 0.0f, 0.0f, 0.0f) };
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 5. Uniform Conventions for Game FX Shaders

| Field | Type | Purpose | Set by |
|-------|------|---------|--------|
| `base_color` | `vec4` | Base color + alpha of the effect | Engine/game |
| `anim_params.x` | `float` | Game time for animation | Engine |
| `anim_params.y` | `float` | Intensity/alpha/health multiplier | Engine / game logic |
| `view_pos_w.xyz` | `vec3` (in vec4) | Camera world position for view-dependent FX | Engine |

**All vec3 quantities are packed into `vec4`** to avoid std140 layout mismatches (see `SKILL.md` §2).

**Shader file naming:**
```
shaders/game/<effect>.glsl       complete VS + FS program (sokol-shdc requirement)
```

---

## 6. Shader Error Handling

When a game FX shader fails to compile at pipeline-creation time:

1. The renderer detects failure: `sg_query_shader_state(shader) == SG_RESOURCESTATE_FAILED`.
2. Logs the failure via the custom logger configured in `sg_setup()`.
3. Creates and binds a magenta fallback pipeline for that material.
4. The object renders as solid magenta — visible but clearly broken; does not crash.

The renderer owns this contract. Game workstream never sees the compilation error directly.
