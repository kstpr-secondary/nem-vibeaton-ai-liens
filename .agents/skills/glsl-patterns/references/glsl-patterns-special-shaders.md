# GLSL Patterns / Special Shaders

Open this when authoring or reviewing skybox shaders, line-quad shaders, or the procedural sky. For lighting shaders (unlit, Lambertian, Blinn-Phong), see `glsl-patterns-lighting.md`. For game FX, see `glsl-patterns-game-fx.md`. For custom shader hooks, see `glsl-patterns-custom-shaders.md`.

---

## 1. Skybox Shader (R-M3)

Uses the `pos.xyww` depth trick so the skybox is always at the far plane regardless of camera position — no z-fight with scene geometry.

**Shader file: `shaders/renderer/skybox.glsl`**

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;       // view-projection with model's translation zeroed (no camera movement effect)
    mat4 model;     // rotation-only model matrix for skybox orientation
};

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

out vec3 uvw;

void main() {
    // Extract only rotation from mvp (zero out translation) to make skybox camera-centered
    mat4 rot_only = mvp;
    rot_only[3] = vec4(0.0, 0.0, 0.0, 1.0);
    uvw = (rot_only * vec4(position, 1.0)).xyz;
    // Depth trick: place at far plane always
    gl_Position = mvp * vec4(position, 1.0);
    gl_Position.z = gl_Position.w;  // force to far plane
}
@end

@fs fs
uniform textureCube skybox_tex;
uniform sampler smp;

in vec3 uvw;

out vec4 frag_color;

void main() {
    vec3 dir = normalize(uvw);
    frag_color = texture(samplerCube(skybox_tex, smp), dir);
}
@end

@program skybox vs fs
```

**Pipeline state for skybox:** depth write `false`, depth test `LESS` (or disabled entirely). Draw **before** opaque geometry.

**Vertex layout:** skybox uses position only — no normal, no texcoord needed (cubemap lookup uses the direction vector from position). The pipeline descriptor should only bind `SG_ATTR_vs_position`.

---

## 2. Line-Quad Shader (R-M3)

World-space billboarded quad between two points for laser beams and line rendering. Alpha-blended for fade effects.

**Shader file: `shaders/renderer/line_quad.glsl`**

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;       // full model-view-projection (no model needed separately for lines)
};

// Line quad vertices: 4 vertices forming a billboarded quad
// Layout: pos0(3) + tangent(3) + width(1) + color(4) = 11 floats per vertex (2 verts per quad)
in vec3 position;     // line endpoint (p0 or p1)
in vec3 tangent;      // line direction (normalized p1 - p0)
in float width;       // quad half-width in world units
in vec4 color;        // RGBA color with alpha for fade

out vec4 frag_color;
out vec2 local_uv;

void main() {
    vec3 world_pos = position;
    vec3 T = normalize(tangent);

    // Create orthogonal basis: use world up, reject against T
    vec3 W = vec3(0.0, 1.0, 0.0);
    vec3 R = normalize(cross(T, W));
    if (length(R) < 0.001) {
        // Line is parallel to world up, use a different reference
        R = normalize(cross(T, vec3(1.0, 0.0, 0.0)));
    }
    vec3 U = normalize(cross(R, T));

    // position.x in buffer = 0 for p0, 1 for p1 (quad span direction)
    float half_w = width * 0.5;
    vec3 offset = R * (position.x - 0.5) * 2.0 * half_w;

    vec3 final_pos = world_pos + U * 0.0 + offset;
    gl_Position = mvp * vec4(final_pos, 1.0);
    frag_color = color;
    local_uv = vec2(position.x, 0.0);
}
@end

@fs fs
in vec4 frag_color;
out vec4 out_color;

void main() {
    if (frag_color.a < 0.01) discard;
    out_color = frag_color;
}
@end

@program line_quad vs fs
```

**Pipeline state:** depth write `false`, blend enabled (`SRC_ALPHA` / `ONE_MINUS_SRC_ALPHA`), cull `NONE`.

**Vertex layout for line quads:** Each quad has 4 vertices. The vertex buffer interleaves:
- `position`: `vec3` — x=0 or 1 (quad span), y/z = world position of the endpoint
- `tangent`: `vec3` — normalized direction vector of the line
- `width`: `float` — half-width in world units
- `color`: `vec4` — RGBA

---

### Simpler billboarded variant (camera-facing, recommended for lasers)

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;
    mat4 model;       // contains world position in .xyz, width in a separate uniform
};

uniform line_params {
    vec3 p0;          // world space start point
    vec3 p1;          // world space end point
    float width;      // half-width in world units
};

in vec3 position;   // 0 or 1 (quad span)
in vec4 color;

out vec4 frag_color;

void main() {
    vec3 dir = normalize(p1 - p0);
    // Get camera right vector from view matrix
    vec3 cam_right = vec3(model[0][0], model[1][0], model[2][0]);
    vec3 offset = cam_right * (position.x - 0.5) * width * 2.0;
    vec3 world_pos = mix(p0, p1, position.x) + offset;
    gl_Position = mvp * vec4(world_pos, 1.0);
    frag_color = color;
}
@end
```

---

## 3. Procedural Sky Shader (R-M7, Stretch Alternative)

Procedural star field as an alternative to cubemap skybox — no texture asset needed.

**Shader file: `shaders/renderer/skybox_proc.glsl`**

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;
};

in vec3 position;
out vec3 uvw;

void main() {
    mat4 rot_only = mvp;
    rot_only[3] = vec4(0.0, 0.0, 0.0, 1.0);
    uvw = (rot_only * vec4(position, 1.0)).xyz;
    gl_Position = mvp * vec4(position, 1.0);
    gl_Position.z = gl_Position.w;  // far plane trick
}
@end

@fs fs
in vec3 uvw;

out vec4 frag_color;

float hash(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

void main() {
    vec3 dir = normalize(uvw);

    // Star field: sample noise at high frequency
    float star = hash(floor(dir * 800.0));
    star = step(0.998, star);  // sparse bright dots

    // Subtle nebula glow
    float nebula = hash(floor(dir * 50.0)) * 0.03;

    vec3 col = vec3(star) + vec3(nebula * 0.5, nebula * 0.3, nebula * 0.8);
    frag_color = vec4(col, 1.0);
}
@end

@program procedural_sky vs fs
```

**Pipeline state:** same as skybox — depth write `false`, draw **before** opaque geometry.
