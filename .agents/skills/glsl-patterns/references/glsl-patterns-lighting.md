# GLSL Patterns / Lighting Shaders

Open this when authoring or reviewing unlit, Lambertian, Blinn-Phong, or normal-mapped fragment shaders. Covers lighting math, uniform structs, and pipeline state. For skybox/line-quads, see `glsl-patterns-special-shaders.md`. For game FX, see `glsl-patterns-game-fx.md`. For custom shader hooks, see `glsl-patterns-custom-shaders.md`.

---

## 1. Vertex Shader Template (shared by all lighting shaders)

Every lighting vertex shader follows this pattern:

```glsl
@vs vs
uniform vs_params {
    mat4 mvp;       // model-view-projection
    mat4 model;     // model matrix (for normal computation, world position)
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
fs_params_t fs_p = { .base_color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f) }; // orange
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

**Pipeline state:** depth `LESS_EQUAL`, write `true`, cull `BACK`.

---

## 3. Lambertian Shader (R-M2)

Diffuse lighting from a single directional light. Math: `Lambert = max(N dot -L, 0)`.

**Shader file: `shaders/renderer/lambertian.glsl`**

```glsl
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

@fs fs
uniform fs_params {
    vec4 base_color;      // material albedo
    vec3 light_dir;       // normalized light direction (world space, FROM light)
    vec3 light_color;     // light intensity/color
    float light_intensity;// scalar multiplier
};

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3 N = normalize(world_normal);
    vec3 L = normalize(-light_dir);  // negate: FROM light → TO surface
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = base_color.rgb * light_color * light_intensity * NdotL;
    vec3 ambient = base_color.rgb * 0.15;  // minimal ambient to avoid pure black
    frag_color = vec4(ambient + diffuse, base_color.a);
}
@end

@program lambertian vs fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color        = glm::vec4(0.8f, 0.3f, 0.2f, 1.0f),
    .light_dir         = glm::vec3(0.5f, -1.0f, 0.3f),
    .light_color       = glm::vec3(1.0f, 0.95f, 0.9f),
    .light_intensity   = 1.2f,
};
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

**Gotcha:** `light_dir` points **from the light source toward the scene** (world-space). The shader negates it to get the vector **toward the light** for the dot product. Matches `set_directional_light(dir, color, intensity)`.

---

## 4. Blinn-Phong Shader (R-M4, Desirable)

Adds specular highlight: `H = normalize(L + V)`, `specular = pow(max(N dot H, 0), shininess)`.

**Shader file: `shaders/renderer/phong.glsl`**

```glsl
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

@fs fs
uniform fs_params {
    vec4 base_color;      // material albedo (may come from texture)
    vec3 light_dir;       // world-space direction FROM light
    vec3 light_color;
    float light_intensity;
    vec3 view_pos;        // camera world position
    vec3 spec_color;      // specular highlight color
    float shininess;      // specular exponent (16–256 typical)
    int has_texture;      // 0 = solid color, 1 = sample texture
};

// Optional sampler for diffuse texture
uniform texture2D albedo_tex;
uniform sampler smp;

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;

out vec4 frag_color;

void main() {
    vec3 N = normalize(world_normal);
    vec3 L = normalize(-light_dir);
    vec3 V = normalize(view_pos - world_pos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, shininess) * step(0.0, NdotL); // specular only when lit

    vec4 albedo = has_texture > 0 ? texture(sampler2D(albedo_tex, smp), uv) : base_color;

    vec3 diffuse = albedo.rgb * light_color * light_intensity * NdotL;
    vec3 spec_contrib = spec_color * light_color * light_intensity * spec * 0.5;
    vec3 ambient = albedo.rgb * 0.15;

    frag_color = vec4(ambient + diffuse + spec_contrib, albedo.a);
}
@end

@program phong vs fs
```

**C++ uniform setup:**
```cpp
fs_params_t fs_p = {
    .base_color        = glm::vec4(0.6f, 0.6f, 0.7f, 1.0f),
    .light_dir         = glm::vec3(0.5f, -1.0f, 0.3f),
    .light_color       = glm::vec3(1.0f, 1.0f, 0.95f),
    .light_intensity   = 1.2f,
    .view_pos          = camera_world_pos,
    .spec_color        = glm::vec3(1.0f, 1.0f, 1.0f),
    .shininess         = 64.0f,
    .has_texture       = 0,  // or 1 if texture bound
};
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));

// If has_texture == 1:
sg_bindings bind = {
    .vertex_buffers[0] = vbuf,
    .index_buffer = ibuf,
    .views[SG_SLOT_albedo_tex] = tex_view,
    .samplers[SG_SLOT_smp] = smp,
};
sg_apply_bindings(&bind);
```

**Pipeline state:** depth `LESS_EQUAL`, write `true`, cull `BACK`.

---

## 5. Normal Map Shader (R-M7, Stretch)

Adds tangent-space normal map sampling to Blinn-Phong. Requires vertex attributes for tangent and bitangent.

**Key changes from standard vertex shader:**
```glsl
@vs vs_with_tangents
uniform vs_params {
    mat4 mvp;
    mat4 model;
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
    world_pos = (model * vec4(position, 1.0)).xyz;
    mat3 normal_mat = mat3(transpose(inverse(model)));
    T = normalize(normal_mat * tangent);
    B = normalize(normal_mat * bitangent);
    N = normalize(normal_mat * normal);
    uv = texcoord0;
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs fs_with_normals
uniform fs_params {
    vec4 base_color;
    vec3 light_dir;
    vec3 light_color;
    float light_intensity;
    vec3 view_pos;
    vec3 spec_color;
    float shininess;
    int has_normal_map;
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
    vec3 albedo = has_normal_map > 0
        ? texture(sampler2D(albedo_tex, smp), uv).rgb
        : base_color.rgb;

    vec3 N_world = normalize(N);
    if (has_normal_map > 0) {
        vec3 tangent_normal = texture(sampler2D(normal_tex, smp), uv).rgb;
        tangent_normal = normalize(tangent_normal * 2.0 - 1.0);
        mat3 TBN = mat3(T, B, N_world);
        N_world = normalize(TBN * tangent_normal);
    }

    vec3 L = normalize(-light_dir);
    vec3 V = normalize(view_pos - world_pos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N_world, L), 0.0);
    float NdotH = max(dot(N_world, H), 0.0);
    float spec = pow(NdotH, shininess) * step(0.0, NdotL);

    vec3 diffuse = albedo * light_color * light_intensity * NdotL;
    vec3 spec_contrib = spec_color * light_color * light_intensity * spec * 0.5;
    vec3 ambient = albedo * 0.15;

    frag_color = vec4(ambient + diffuse + spec_contrib, 1.0);
}
@end

@program normal_phong vs_with_tangents fs_with_normals
```

**Mesh builder:** Tangent and bitangent must be generated during procedural mesh creation (sphere, cube, capsule). For a sphere, compute via finite differences of UVs. For a cube, use face normals to derive consistent tangent frames.
