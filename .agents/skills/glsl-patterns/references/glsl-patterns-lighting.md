# GLSL Patterns / Lighting Shaders

Open this when authoring or reviewing unlit, Lambertian, Blinn-Phong, or normal-mapped fragment shaders. Covers lighting math, uniform structs, and pipeline state. For skybox/line-quads, see `glsl-patterns-special-shaders.md`. For game FX, see `glsl-patterns-game-fx.md`. For custom shader hooks, see `glsl-patterns-custom-shaders.md`.

---

## 1. Vertex Shader Template (shared by all lighting shaders)

The **normal matrix is computed on the CPU** and uploaded as a `mat4` (packing a mat3 into a mat4 for std140 safety — `glm::mat3` is 36 bytes but std140 pads each column to 16 bytes, so passing a raw `glm::mat3` produces a layout mismatch). Extracting `mat3(normal_mat)` inside the shader is free.

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

@vs vs
uniform vs_params {
    mat4 mvp;         // model-view-projection
    mat4 model;       // model matrix (for world-space position)
    mat4 normal_mat;  // transpose(inverse(mat3(model))), packed as mat4 for std140 safety
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
```

**C++ VS uniform upload (all lighting pipelines):**
```cpp
vs_params_t vs_p = {
    .mvp        = proj * view * model_m,
    .model      = model_m,
    .normal_mat = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_m)))),
};
sg_apply_uniforms(SLOT_vs_params, &SG_RANGE(vs_p));
```

**Vertex attributes:**
| Attribute | Type | Purpose |
|-----------|------|---------|
| `position` | `vec3` | Vertex position |
| `normal` | `vec3` | Surface normal (unit length) |
| `texcoord0` | `vec2` | UV coordinates |

---

## 2. Unlit Shader (R-M1)

Solid-color rendering, no lighting. Simplest pipeline.

**Shader file: `shaders/renderer/unlit.glsl`**

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

@fs fs
uniform fs_params {
    vec4 base_color;  // RGBA from material
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    frag_color = base_color;
}
@end

@program unlit vs fs
```

**C++ uniform setup:**
```cpp
vs_params_t vs_p = {
    .mvp        = proj * view * model_m,
    .model      = model_m,
    .normal_mat = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_m)))),
};
sg_apply_uniforms(SLOT_vs_params, &SG_RANGE(vs_p));

fs_params_t fs_p = { .base_color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f) };
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `true`, cull `BACK`.

---

## 3. Lambertian Shader (R-M2)

Diffuse lighting from a single directional light. Math: `Lambert = max(N dot L, 0)` where `L` points **toward** the light.

**Shader file: `shaders/renderer/lambertian.glsl`**

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

@fs fs
// All vec3 light fields packed into vec4 — avoids std140 glm::vec3 size-mismatch (glm::vec3
// is 12 bytes; std140 vec3 occupies 16 bytes). Use the .xyz swizzle in shader code.
uniform fs_params {
    vec4 base_color;         // .rgba = material albedo + alpha
    vec4 light_dir_ws;       // .xyz = direction FROM light (world space); shader negates
    vec4 light_color_inten;  // .xyz = light color, .w = intensity scalar
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3 N = normalize(world_normal);
    vec3 L = normalize(-light_dir_ws.xyz);  // negate: FROM light → toward light
    float NdotL  = max(dot(N, L), 0.0);
    vec3 diffuse = base_color.rgb * light_color_inten.rgb * light_color_inten.w * NdotL;
    vec3 ambient = base_color.rgb * 0.15;
    frag_color   = vec4(ambient + diffuse, base_color.a);
}
@end

@program lambertian vs fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color        = glm::vec4(0.8f, 0.3f, 0.2f, 1.0f),
    .light_dir_ws      = glm::vec4(glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f)), 0.0f),
    .light_color_inten = glm::vec4(1.0f, 0.95f, 0.9f, 1.2f),  // xyz=color, w=intensity
};
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));
```

**Convention:** `light_dir_ws.xyz` points from the light source toward the scene (matches `set_directional_light(dir, ...)`). The shader negates it to obtain the vector toward the light for `dot(N, L)`.

---

## 4. Blinn-Phong Shader (R-M4, Desirable)

Adds specular highlight: `H = normalize(L + V)`, `specular = pow(max(N dot H, 0), shininess)`.

**Shader file: `shaders/renderer/phong.glsl`**

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

@fs fs
// All vec3 fields packed into vec4 for std140 safety.
uniform fs_params {
    vec4 base_color;         // .rgba = material albedo + alpha
    vec4 light_dir_ws;       // .xyz = direction FROM light, .w = unused
    vec4 light_color_inten;  // .xyz = color, .w = intensity
    vec4 view_pos_w;         // .xyz = camera world position, .w = unused
    vec4 spec_color_shin;    // .xyz = specular color, .w = shininess exponent (16–256)
    vec4 flags;              // .x = has_texture (0.0 = solid color, 1.0 = sample texture)
};

uniform texture2D albedo_tex;
uniform sampler smp;

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3 N = normalize(world_normal);
    vec3 L = normalize(-light_dir_ws.xyz);
    vec3 V = normalize(view_pos_w.xyz - world_pos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float spec  = pow(NdotH, spec_color_shin.w) * step(0.0, NdotL);

    vec4 albedo = flags.x > 0.5
        ? texture(sampler2D(albedo_tex, smp), uv)
        : base_color;

    vec3 diffuse   = albedo.rgb * light_color_inten.rgb * light_color_inten.w * NdotL;
    vec3 spec_term = spec_color_shin.rgb * light_color_inten.rgb * light_color_inten.w * spec * 0.5;
    vec3 ambient   = albedo.rgb * 0.15;

    frag_color = vec4(ambient + diffuse + spec_term, albedo.a);
}
@end

@program phong vs fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color        = glm::vec4(0.6f, 0.6f, 0.7f, 1.0f),
    .light_dir_ws      = glm::vec4(glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f)), 0.0f),
    .light_color_inten = glm::vec4(1.0f, 1.0f, 0.95f, 1.2f),
    .view_pos_w        = glm::vec4(camera_world_pos, 0.0f),
    .spec_color_shin   = glm::vec4(1.0f, 1.0f, 1.0f, 64.0f),  // .w = shininess
    .flags             = glm::vec4(0.0f),  // set .x = 1.0f when texture is bound
};
sg_apply_uniforms(SLOT_fs_params, &SG_RANGE(fs_p));

// When flags.x == 1.0 (textured):
sg_bindings bind = {
    .vertex_buffers[0]       = vbuf,
    .index_buffer            = ibuf,
    .images[SLOT_albedo_tex] = tex,   // .images[], not .views[]
    .samplers[SLOT_smp]      = smp,
};
sg_apply_bindings(&bind);
```

**Pipeline state:** depth `LESS_EQUAL`, write `true`, cull `BACK`.

---

## 5. Normal Map Shader (R-M7, Stretch)

Adds tangent-space normal map sampling to Blinn-Phong. Requires tangent + bitangent vertex attributes.

```glsl
@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

@vs vs_with_tangents
uniform vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;  // CPU-computed, packed as mat4
};

in vec3 position;
in vec3 normal;
in vec2 texcoord0;
in vec3 tangent;
in vec3 bitangent;

out vec3 world_pos;
out vec3 T;   // tangent in world space
out vec3 B;   // bitangent in world space
out vec3 N;   // normal in world space
out vec2 uv;

void main() {
    world_pos   = (model * vec4(position, 1.0)).xyz;
    mat3 N_mat  = mat3(normal_mat);
    T           = normalize(N_mat * tangent);
    B           = normalize(N_mat * bitangent);
    N           = normalize(N_mat * normal);
    uv          = texcoord0;
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs fs_with_normals
uniform fs_params {
    vec4 base_color;
    vec4 light_dir_ws;
    vec4 light_color_inten;
    vec4 view_pos_w;
    vec4 spec_color_shin;
    vec4 flags;          // .x = has_normal_map (0.0 or 1.0)
};

uniform texture2D albedo_tex;
uniform texture2D normal_tex;
uniform sampler smp;

in vec3 world_pos;
in vec3 T;
in vec3 B;
in vec3 N;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec4 albedo  = texture(sampler2D(albedo_tex, smp), uv);
    vec3 N_world = normalize(N);

    if (flags.x > 0.5) {
        vec3 tnn = texture(sampler2D(normal_tex, smp), uv).rgb;
        tnn      = normalize(tnn * 2.0 - 1.0);
        N_world  = normalize(mat3(T, B, N_world) * tnn);
    }

    vec3 L = normalize(-light_dir_ws.xyz);
    vec3 V = normalize(view_pos_w.xyz - world_pos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N_world, L), 0.0);
    float NdotH = max(dot(N_world, H), 0.0);
    float spec  = pow(NdotH, spec_color_shin.w) * step(0.0, NdotL);

    vec3 diffuse   = albedo.rgb * light_color_inten.rgb * light_color_inten.w * NdotL;
    vec3 spec_term = spec_color_shin.rgb * light_color_inten.rgb * light_color_inten.w * spec * 0.5;
    vec3 ambient   = albedo.rgb * 0.15;

    frag_color = vec4(ambient + diffuse + spec_term, albedo.a);
}
@end

@program normal_phong vs_with_tangents fs_with_normals
```

**Mesh builder:** Tangent and bitangent generated during procedural mesh creation. For a sphere, derive via finite differences of UVs. For a cube, derive from face normals.
