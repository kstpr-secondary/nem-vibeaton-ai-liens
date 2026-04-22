# GLSL Patterns / Special Shaders

Open this when authoring or reviewing skybox shaders, line-quad shaders, or the procedural sky. For lighting shaders (unlit, Lambertian, Blinn-Phong), see `glsl-patterns-lighting.md`. For game FX, see `glsl-patterns-game-fx.md`. For custom shader hooks, see `glsl-patterns-custom-shaders.md`.

---

## 1. Skybox Shader (R-M3)

Uses the `pos.xyww` depth trick so all skybox fragments land at NDC z = 1.0 (far plane). The cubemap lookup direction is simply the unit-cube vertex position in world space — no model matrix needed.

**Critical:** The view matrix translation must be stripped before combining with projection; otherwise the skybox moves with the camera instead of staying at infinity. Pass `mat4(mat3(view))` (translation stripped) combined with projection as the single `vp_no_trans` uniform.

**Pipeline state:** depth compare `LESS_EQUAL`, depth write `false`, draw **before** opaque geometry. (With `pos.xyww`, skybox fragments are at z = 1.0. A `LESS` compare would reject them against a depth buffer cleared to 1.0 — the skybox would be invisible. Use `LESS_EQUAL`.)

**Shader file: `shaders/renderer/skybox.glsl`**

```glsl
@ctype mat4 glm::mat4

@vs vs
uniform vs_params {
    mat4 vp_no_trans;  // projection * mat4(mat3(view)) — translation stripped from view
};

in vec3 position;  // unit-cube vertex; position IS the cubemap lookup direction

out vec3 uvw;

void main() {
    uvw = position;  // world-space direction for cubemap lookup
    vec4 pos    = vp_no_trans * vec4(position, 1.0);
    gl_Position = pos.xyww;  // z = w → NDC z = 1.0 (far plane) after perspective divide
}
@end

@fs fs
uniform textureCube skybox_tex;
uniform sampler smp;

in vec3 uvw;

out vec4 frag_color;

void main() {
    frag_color = texture(samplerCube(skybox_tex, smp), normalize(uvw));
}
@end

@program skybox vs fs
```

**C++ VS uniform upload:**
```cpp
vs_params_t vs_p = {
    .vp_no_trans = proj * glm::mat4(glm::mat3(view)),  // strip translation from view
};
sg_apply_uniforms(SLOT_vs_params, &SG_RANGE(vs_p));

// Skybox cubemap binding:
sg_bindings bind = {
    .vertex_buffers[0]        = skybox_vbuf,
    .images[SLOT_skybox_tex]  = cubemap_img,
    .samplers[SLOT_smp]       = smp,
};
sg_apply_bindings(&bind);
```

**Vertex layout for skybox:** position only — `in vec3 position`. The pipeline descriptor binds only `SG_ATTR_vs_position`. No normal, no texcoord needed.

---

## 2. Line-Quad Shader (R-M3)

World-space quad between two endpoints for laser beams and line rendering. Alpha-blended for fade effects.

**Generation strategy:** quads are **expanded on the CPU** before upload — the CPU computes the 4 billboard vertices using the camera right vector. The vertex shader just applies MVP. This is simpler and more correct than doing billboard math in the VS.

**UV convention:** u = 0..1 along beam length, **v = 0..1 across beam width**. Game FX shaders (laser glow) sample `v` for the cross-section fade — see `glsl-patterns-game-fx.md` §4.

**Shader file: `shaders/renderer/line_quad.glsl`**

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

@vs vs
uniform vs_params {
    mat4 mvp;
};

in vec3 position;    // CPU-expanded world-space vertex position
in vec2 texcoord0;   // u=0..1 along beam length, v=0..1 across beam width
in vec4 color;       // RGBA per-vertex color (encodes beam color + alpha)

out vec4 vert_color;
out vec2 uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    vert_color  = color;
    uv          = texcoord0;
}
@end

@fs fs
in vec4 vert_color;
in vec2 uv;

out vec4 out_color;

void main() {
    if (vert_color.a < 0.01) discard;
    out_color = vert_color;
}
@end

@program line_quad vs fs
```

**CPU-side quad generation (C++ helper):**
```cpp
// Call once per line per frame; push 4 vertices into a dynamic vertex buffer.
void push_line_quad(LineBatch& batch, glm::vec3 p0, glm::vec3 p1,
                    float width, glm::vec4 color, glm::vec3 cam_right) {
    glm::vec3 dir  = glm::normalize(p1 - p0);
    glm::vec3 perp = glm::normalize(glm::cross(dir, cam_right)) * (width * 0.5f);
    // 4 corners of the billboard quad:
    glm::vec3 v0 = p0 - perp, v1 = p0 + perp;
    glm::vec3 v2 = p1 + perp, v3 = p1 - perp;
    // Upload with UVs: u=0 at p0 end, u=1 at p1 end; v=0/1 across width.
    batch.push(v0, {0,0}, color); batch.push(v1, {0,1}, color);
    batch.push(v2, {1,1}, color); batch.push(v3, {1,0}, color);
}
```

**Pipeline state:** depth `LESS_EQUAL`, write `false`, blend `SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, cull `NONE`.

---

## 3. Procedural Sky Shader (R-M7, Stretch Alternative)

Procedural star field as an alternative to cubemap skybox — no texture asset needed. Same depth trick and `vp_no_trans` approach as the cubemap skybox.

**Shader file: `shaders/renderer/skybox_proc.glsl`**

```glsl
@ctype mat4 glm::mat4

@vs vs
uniform vs_params {
    mat4 vp_no_trans;  // projection * mat4(mat3(view))
};

in vec3 position;

out vec3 uvw;

void main() {
    uvw = position;
    vec4 pos    = vp_no_trans * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
@end

@fs fs
in vec3 uvw;

out vec4 frag_color;

float hash(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

void main() {
    vec3  dir  = normalize(uvw);
    float star  = hash(floor(dir * 800.0));
    star        = step(0.998, star);  // sparse bright dots

    float nebula = hash(floor(dir * 50.0)) * 0.03;
    vec3  col    = vec3(star) + vec3(nebula * 0.5, nebula * 0.3, nebula * 0.8);
    frag_color   = vec4(col, 1.0);
}
@end

@program procedural_sky vs fs
```

**Pipeline state:** same as cubemap skybox — depth `LESS_EQUAL`, write `false`, draw **before** opaque geometry.
