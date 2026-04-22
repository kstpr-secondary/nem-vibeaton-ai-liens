# GLSL Patterns / Game FX Shaders

Open this when authoring or reviewing game workstream FX shaders: plasma beam, explosion, shield VFX, and laser beam. These are custom fragment shaders consumed by the renderer's custom shader hook (R-M5). They share the standard vertex shader structure from `glsl-patterns-lighting.md` (or the line-quad vertex shader for laser beams). For lighting shaders, see `glsl-patterns-lighting.md`. For skybox/line-quads, see `glsl-patterns-special-shaders.md`.

---

## 1. Plasma Beam (Game workstream)

Animated plasma effect for projectile rendering. Uses a quad with time-based noise animation.

**Shader file: `shaders/game/plasma.frag.glsl`**

```glsl
@fs plasma_beam_fs
uniform fs_params {
    vec4 base_color;      // plasma color (e.g., bright cyan/teal)
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

@program plasma_beam custom_vs plasma_beam_fs
```

---

## 2. Explosion Effect (Game workstream)

Burst effect on a quad — expands outward with fade. Used when an asteroid or ship is destroyed.

**Shader file: `shaders/game/explosion.frag.glsl`**

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
    // Distance from center of quad
    float d = length(uv - 0.5) * 2.0;  // 0 at center, 1 at edges

    // Expand outward: the "fireball" grows then shrinks
    float expand = smoothstep(0.0, 0.3, time);
    float contract = smoothstep(0.3, 1.0, time);

    // Brightness fades over time
    float brightness = 1.0 - contract;

    // Radial gradient: bright center, fading to edge
    float radial = 1.0 - smoothstep(0.0, expand + 0.3 * (1.0 - expand), d);

    // Flicker noise
    float flicker = fract(sin(dot(uv + time, vec2(127.1, 311.7))) * 43758.5453);

    vec3 col = base_color.rgb * (radial * brightness + flicker * 0.2 * brightness);
    float alpha = intensity * brightness * radial;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program explosion custom_vs explosion_fs
```

---

## 3. Shield VFX (Game workstream)

Translucent dome/hexagonal shield effect around the player ship. Uses fresnel-like rim lighting.

**Shader file: `shaders/game/shield.frag.glsl`**

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
    // Rim lighting: stronger at edges
    vec3 V = normalize(vec3(0.0, 0.0, 1.0));  // simplified: view from +Z
    vec3 N = normalize(world_normal);
    float rim = 1.0 - max(dot(N, V), 0.0);
    rim = pow(rim, 2.0);

    // Subtle pulsing
    float pulse = sin(time * 2.0) * 0.1 + 0.9;

    vec3 col = base_color.rgb * (rim * 0.8 + 0.2) * pulse;
    float alpha = rim * health * 0.7;

    if (alpha < 0.01) discard;
    frag_color = vec4(col, alpha);
}
@end

@program shield custom_vs shield_fs
```

---

## 4. Laser Beam (Game workstream)

Thin alpha-blended line quad using the line-quad vertex shader (see `glsl-patterns-special-shaders.md`) with a glow fragment.

**Shader file: `shaders/game/laser.frag.glsl`**

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
    // Glow: brighter in center, fading at edges
    float edge = smoothstep(0.0, 0.15, min(uv.x, 1.0 - uv.x));
    vec3 col = base_color.rgb * (0.6 + 0.4 * edge);
    frag_color = vec4(col, base_color.a * intensity);
}
@end

@program laser custom_vs laser_fs
```

**Pipeline state:** depth write `false`, blend enabled (`SRC_ALPHA` / `ONE_MINUS_SRC_ALPHA`), cull `NONE`. Alpha blending per `glsl-patterns-lighting.md` §11 reference table.
