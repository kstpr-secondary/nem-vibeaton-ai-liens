# Visual Improvements & Bug Fix Plan
*Branch: feature/renderer and feature/game*  
*Written: 2026-05-03 — revised 2026-05-03 (architecture rethink)*

---

## 1. Status Quo Diagnostics

### BUG-1 — Transparent BlinnPhong is doubly broken

Two independent defects make BlinnPhong transparency impossible today:

**Defect A — Routing never fires for BlinnPhong.**  
`renderer_make_blinnphong_material` sets `material.base_color[3] = 1.0f` but never sets
`material.alpha` (it stays at the default `1.0f`). The transparent routing gate in
`renderer_end_frame()` reads `cmd.material.alpha >= 1.0f`, so every BlinnPhong object
is treated as fully opaque regardless of its `base_color[3]`.

**Defect B — Wrong shader even if routing worked.**  
The transparent pipeline (`state.pipeline_transparent`) is created with the `unlit` shader,
not the `blinnphong` shader. A BlinnPhong mesh sent through this pipeline renders as a flat
unlit colour with alpha blending — no lighting.

Both defects are **fixed architecturally** by the material system redesign in Section 3.1:
the new `Material` carries a `PipelineState` with an explicit `BlendMode`, and the renderer
uses a pipeline cache keyed on `(shader_handle, pipeline_state)` — no manual routing at all.

### BUG-2 — Laser beam disappears after one frame

`laser_fire()` calls `renderer_enqueue_line_quad` exactly once, in the frame the weapon
fires. Because the laser has a 5-second cooldown, the beam is submitted once and gone
after ~16 ms — invisible in practice. Holding right-mouse does nothing because
`weapon_ready()` returns false until the cooldown expires again.

Fix: store the beam as a `LaserBeam` component and re-submit every frame for its duration.

### BUG-3 — Laser beam origin is at the camera, not at the ship

The visual beam is drawn from `ray_origin` (camera near-plane position) to
`end` (camera + ray_dir * max_range). The damage raycast correctly starts from
`muzzle_origin` (ship nose). In third-person the beam appears to originate inside the
camera. Fix: draw from `muzzle_origin`.

### BUG-4 — Laser is an instant point-and-click kill (design fix)

No skill requirement — aim once, click, enemy HP drops instantly. Fix: charge-up hold
(Section 3.3).

### BUG-5 — No alpha-cutout (alpha-tested) geometry support

No pipeline discards fragments based on alpha threshold. Needed for decals / shield impact
splats. Added as part of the material redesign (`BlendMode::Cutout`).

---

## 2. Overview of Planned Changes

| Area | Change |
|------|--------|
| `renderer.h` | **Material system redesign** — remove `ShadingModel` enum, add `RendererShaderHandle`, `PipelineState`, `BlendMode`/`CullMode`, `renderer_create_shader()`, `renderer_builtin_shader()`, `renderer_set_time()`. Add BlendMode param to `renderer_enqueue_line_quad`. Publish `UnlitFSParams` / `BlinnPhongFSParams` structs. FROZEN v1.1 → needs human approval for v1.2. |
| `renderer.cpp` | Replace per-`ShadingModel` pipeline switch with pipeline cache. Add additive line-quad pipeline. Generic draw loop. |
| `shaders/game/shield.glsl` | NEW — Fresnel rim, translucent sphere. In game shader directory. |
| `shaders/game/plasma_ball.glsl` | NEW — procedural energy bolts, animated core. In game shader directory. |
| `src/game/game_shaders.h/.cpp` | NEW — register game shaders in `game_init()`, store `RendererShaderHandle` globals. |
| `src/game/components.h` | New components: `LaserBeam`, `LaserCharge`, `ShieldSphere`, `VFXData` |
| `src/game/constants.h` | New tuning constants for laser, shield, VFX |
| `src/game/weapons.cpp` | Laser persistence, charging mechanic, muzzle origin fix |
| `src/game/spawn.cpp` | Shield sphere spawn added to player/enemy; plasma material updated |
| `src/game/damage.cpp` | Trigger impact VFX and shield-hit VFX on collision |
| `src/game/shield_vfx.cpp/.h` | NEW — shield sphere follow + alpha update system |
| `src/game/vfx.cpp/.h` | NEW — animated VFX system (scale + fade) |
| `src/game/game.cpp` | Wire up new update functions in `game_tick` |
| `CMakeLists.txt` | No changes needed — sources and shaders are auto-globbed. |

---

## 3. Technical Design

### 3.1 Material System Redesign (renderer.h — FROZEN v1.1)

**Human approval required. Target: bump to v1.2.**

#### 3.1.1 Core principle

The old design (flat `Material` with `ShadingModel` enum, `shininess`, `alpha`) mixes three
concerns: *which shader runs*, *what pipeline state to use*, and *shader parameters*. Adding
game-specific effects required new `ShadingModel` variants — leaking game concepts into the
renderer.

The new design separates these cleanly:
- **Shader identity**: a `RendererShaderHandle` — an opaque handle to a compiled VS+FS pair.
- **Pipeline state**: explicit `PipelineState` struct on the `Material` — blend mode, cull,
  depth write, render queue. No implicit behavior from the shader name.
- **Parameters**: a raw uniform byte blob (`uint8_t uniforms[256]`) — the caller knows its own
  shader's struct layout and casts into it. The renderer forwards it verbatim to
  `sg_apply_uniforms(FS, 1, ...)`.

The renderer is completely ignorant of Shield, PlasmaBall, Laser, or any other game concept.
It executes: *get pipeline from cache (shader × pipeline_state), apply VS transforms, apply FS
blob, bind textures, draw.* That's the entire loop.

#### 3.1.2 New types and functions

Add to renderer.h:

```cpp
// ---------------------------------------------------------------------------
// Shader handle
// ---------------------------------------------------------------------------
struct RendererShaderHandle { uint32_t id = 0; };

// ---------------------------------------------------------------------------
// Pipeline state — travels with the material instance
// ---------------------------------------------------------------------------
enum class BlendMode  : uint8_t { Opaque = 0, Cutout, AlphaBlend, Additive };
enum class CullMode   : uint8_t { Back   = 0, Front,  None };

struct PipelineState {
    BlendMode blend        = BlendMode::Opaque;
    CullMode  cull         = CullMode::Back;
    bool      depth_write  = true;
    uint8_t   render_queue = 0;
    // render_queue: 0=opaque, 1=cutout, 2=transparent, 3=additive
    // Renderer draws passes in this order. Custom shaders choose their queue.
};

// ---------------------------------------------------------------------------
// Material — replaces the old struct entirely
// ---------------------------------------------------------------------------
static constexpr int k_material_uniform_bytes = 256;
static constexpr int k_material_texture_slots  = 4;

struct Material {
    RendererShaderHandle  shader;
    PipelineState         pipeline;
    uint8_t               uniforms[k_material_uniform_bytes] = {};
    uint8_t               uniforms_size  = 0;
    RendererTextureHandle textures[k_material_texture_slots] = {};
    uint8_t               texture_count  = 0;
};

// Typed helpers — both declared inline in renderer.h
template<typename T>
void material_set_uniforms(Material& m, const T& params) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    memcpy(m.uniforms, &params, sizeof(T));
    m.uniforms_size = static_cast<uint8_t>(sizeof(T));
}
template<typename T>
T* material_uniforms_as(Material& m) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    return reinterpret_cast<T*>(m.uniforms);
}

// ---------------------------------------------------------------------------
// Published FS param structs for built-in shaders
// (Game code may reinterpret_cast into Material::uniforms using these)
// ---------------------------------------------------------------------------
struct UnlitFSParams {
    glm::vec4 color;             // rgba
    glm::vec4 flags;             // .x = use_texture (1.0 or 0.0)
};
struct BlinnPhongFSParams {
    glm::vec4 base_color;
    glm::vec4 light_dir_ws;
    glm::vec4 light_color_inten;
    glm::vec4 view_pos_w;
    glm::vec4 spec_shin;         // .rgb = specular color, .w = shininess
    glm::vec4 flags;             // .x = use_texture
};

// ---------------------------------------------------------------------------
// Shader API
// ---------------------------------------------------------------------------

// Register a shader from a sokol-shdc generated descriptor.
// Call once at init after renderer_init(). Returns an opaque handle.
RendererShaderHandle renderer_create_shader(const sg_shader_desc* desc);

// Retrieve handles to the built-in shaders (no GPU work — just returns a stored handle).
enum class BuiltinShader : uint8_t { Unlit = 0, BlinnPhong, Lambertian };
RendererShaderHandle renderer_builtin_shader(BuiltinShader s);

// ---------------------------------------------------------------------------
// New per-frame API
// ---------------------------------------------------------------------------

// Propagate elapsed time to animated materials.
// Called once per tick from game_tick, before enqueue calls.
void renderer_set_time(float seconds_since_start);

// ---------------------------------------------------------------------------
// Updated line-quad API — adds optional BlendMode parameter
// Default = Opaque preserves all existing call-sites without changes.
// ---------------------------------------------------------------------------
void renderer_enqueue_line_quad(
    const float p0[3],
    const float p1[3],
    float       width,
    const float color[4],
    BlendMode   blend = BlendMode::Opaque   // NEW — Additive for laser/VFX
);
```

#### 3.1.3 What is REMOVED from renderer.h

- `enum class ShadingModel` — deleted entirely.
- `Material::shininess` named field — absorbed into `BlinnPhongFSParams::spec_shin.w`.
- `Material::alpha` named field — absorbed into `PipelineState::blend` + `BlinnPhongFSParams::base_color.a`.
- `Material::base_color[4]` named field — absorbed into `UnlitFSParams::color` / `BlinnPhongFSParams::base_color`.
- `Material::texture` named field — replaced by `Material::textures[0]`.

#### 3.1.4 Convenience factories (unchanged signatures, updated internals)

These stay in renderer.h so callers don't need to change:
```cpp
Material renderer_make_unlit_material(const float rgba[4]);
Material renderer_make_lambertian_material(const float rgb[3]);
Material renderer_make_blinnphong_material(const float* rgb, float shininess,
                                            RendererTextureHandle texture = {});
```
Internally they now fill the `uniforms` blob using `material_set_uniforms<UnlitFSParams>(…)`
etc. and set `m.shader = renderer_builtin_shader(BuiltinShader::Unlit)`.

To make a **transparent BlinnPhong** (previously impossible due to BUG-1 and BUG-2), after
calling the factory:
```cpp
Material m = renderer_make_blinnphong_material(rgb, shininess);
m.pipeline.blend       = BlendMode::AlphaBlend;
m.pipeline.depth_write = false;
m.pipeline.render_queue = 2;
// Update alpha in the uniforms blob:
material_uniforms_as<BlinnPhongFSParams>(m)->base_color.a = desired_alpha;
```
The pipeline cache creates the right BlinnPhong+AlphaBlend pipeline automatically on first use.

### 3.2 Renderer.cpp: Pipeline Cache and Generic Draw Loop

#### 3.2.1 Pipeline cache

Replace the seven named `sg_pipeline` fields in `RendererState` with a linear cache:

```cpp
struct PipelineCacheEntry {
    uint32_t     shader_id;   // RendererShaderHandle::id
    PipelineState state;
    sg_pipeline  pipeline;
};
static constexpr int k_pipeline_cache_max = 64;
PipelineCacheEntry pipeline_cache[k_pipeline_cache_max];
int                pipeline_cache_count = 0;
```

`get_or_create_pipeline(RendererShaderHandle sh, PipelineState state)`:
1. Linear scan for `(sh.id, state)` match.
2. If found: return cached `sg_pipeline`.
3. If not: build new `sg_pipeline_desc` from the shader's `sg_shader` object plus the
   blend/cull/depth fields from `state`. `BlendMode::Additive` → `src=ONE, dst=ONE`.
   `BlendMode::Cutout` → normal opaque pipeline (frag discard handled in shader).
   Store and return.

The magenta fallback pipeline is pre-inserted at cache index 0 with `shader_id=0` as the
sentinel.

#### 3.2.2 VS uniform convention

Every mesh shader (including game shaders in `shaders/game/`) **must** have VS binding 0 as:
```glsl
layout(binding=0) uniform vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};
```
Shaders that don't use `model`/`normal_mat` (unlit) still declare them; they are simply ignored.
The renderer always sends this block at binding 0 for every mesh draw.

#### 3.2.3 Generic draw loop in `renderer_end_frame()`

Replace the large `switch (cmd.material.shading_model)` with:
```
Sort draw_queue by render_queue (stable, ascending)
For each batch of draw calls with the same render_queue:
  For each draw call:
    sg_pipeline pip = get_or_create_pipeline(cmd.material.shader, cmd.material.pipeline)
    sg_apply_pipeline(pip)
    sg_apply_uniforms(VS, 0, SG_RANGE(vs_params))      // computed from camera + transform
    sg_apply_uniforms(FS, 1, {uniforms, uniforms_size}) // verbatim from material blob
    bind textures and sampler
    sg_draw(...)
```

**For built-in shaders** (BlinnPhong, Lambertian), the renderer must patch per-frame uniforms
(light direction, view position) into a local copy of the FS params block before calling
`sg_apply_uniforms`. It reads the material blob as `BlinnPhongFSParams`, patches the
`light_dir_ws`, `light_color_inten`, `view_pos_w` fields from the current frame state, then
submits. Per-material fields (`base_color`, `spec_shin`, `flags`) come from the blob as-is.

For custom shaders (game-registered), the renderer forwards the blob verbatim — the game is
responsible for all fields, including any view_pos needed by its shader.

#### 3.2.4 Line-quad additive support

Add `pipeline_line_quad_additive` — same shader and vertex layout as `pipeline_line_quad`,
but `blend = ONE, ONE`, depth write off, cull NONE. When `renderer_enqueue_line_quad` is
called with `BlendMode::Additive`, route to this pipeline. All existing call sites pass
`BlendMode::Opaque` (the default), so no call-site changes are required elsewhere.

#### 3.2.5 Time uniform

Add `float current_time = 0.0f` to `RendererState`. `renderer_set_time(t)` stores it.
Game shaders that need time include it in their own FS params struct and read it from the
material uniforms blob (the game fills it each frame from `engine_now()`).

### 3.3 New Game Shaders (shaders/game/)

The `shaders/game/` directory and its `game_shaders` CMake target already exist in the build
system. Any `.glsl` file placed there is compiled automatically by sokol-shdc into
`build/generated/shaders/<name>.glsl.h`.

**VS convention**: all mesh shaders follow the VS binding 0 convention from Section 3.2.2.

#### 3.3.1 shaders/game/shield.glsl

Vertex inputs: `position` (vec3), `normal` (vec3).  
VS uniform binding 0: `{ mat4 mvp; mat4 model; mat4 normal_mat; }` (standard convention).  
VS outputs: `v_normal` (vec3), `v_world_pos` (vec3).

FS uniform binding 1 (std140 — all vec4):
```glsl
layout(binding=1) uniform shield_fs_params {
    vec4 shield_color;    // .rgb = tint (e.g. 0.3,0.5,1.0 blue), .a = base opacity scale
    vec4 view_pos_w;      // .xyz = camera world position (filled by game each frame)
    vec4 fresnel_params;  // .x = exponent (3.0), .y = rim intensity (1.2)
};
```

Fragment logic:
```glsl
vec3 N = normalize(v_normal);
vec3 V = normalize(view_pos_w.xyz - v_world_pos);
float rim = pow(1.0 - max(dot(N, V), 0.0), fresnel_params.x);
float alpha = rim * fresnel_params.y * shield_color.a;
frag_color = vec4(shield_color.rgb * (0.3 + rim), alpha);
```

Pipeline state when creating this material: `BlendMode::AlphaBlend, CullMode::None,
depth_write=false, render_queue=2`.

#### 3.3.2 shaders/game/plasma_ball.glsl

Vertex inputs: `position` (vec3), `normal` (vec3).  
VS uniform binding 0: standard convention.  
VS outputs: `v_normal`, `v_world_pos`, `v_obj_pos` (object-space position, for noise).

FS uniform binding 1 (std140):
```glsl
layout(binding=1) uniform plasma_fs_params {
    vec4 core_color;    // .rgb = warm white (1.0,1.0,0.9)
    vec4 rim_color;     // .rgb = blue-white (0.4,0.6,1.0)
    vec4 bolt_color;    // .rgb = per-instance tint (from game)
    vec4 params;        // .x=time, .y=fresnel_exp(3.0), .z=core_threshold(0.65), .w=opacity(1.0)
    vec4 view_pos_w;    // .xyz = camera world position (filled by game each frame)
};
```

Fragment logic:
```glsl
float NdotV = max(dot(N, V), 0.0);
float rim  = pow(1.0 - NdotV, params.y);
float core = smoothstep(params.z - 0.15, params.z + 0.05, NdotV);

vec3 p = normalize(v_obj_pos);
float t = params.x;
float n1 = vnoise(p * 4.0 + vec3(t*0.6, 0.0, t*0.4));
float n2 = vnoise(p * 9.0 + vec3(-t*0.9, t*0.5, 0.0));
float bolts = pow(max(n1 * n2, 0.0), 2.5) * 4.0;

vec3 col = core_color.rgb * core * 3.0
         + bolt_color.rgb * bolts
         + rim_color.rgb * rim;
float alpha = (core * 0.9 + rim * 0.7 + bolts * 0.5) * params.w;
frag_color = vec4(col, alpha);
```

Include the `vnoise()` function from Section 5 Appendix A.

Pipeline state: `BlendMode::Additive, CullMode::None, depth_write=false, render_queue=3`.

**Color parameter responsibility**: `core_color` and `rim_color` are hardcoded constants
in the game-side material creation helper (`make_plasma_material()`). `bolt_color` is the
per-instance tint from game code. All three are packed into the FS params blob by the game.
The renderer has no knowledge of any of these fields.

### 3.4 Game Shader Init: game_shaders.h / game_shaders.cpp

A small new translation unit pairs each game shader with its `RendererShaderHandle`:

```cpp
// src/game/game_shaders.h
#pragma once
#include <renderer.h>
extern RendererShaderHandle g_shield_shader;
extern RendererShaderHandle g_plasma_shader;
void game_shaders_init();  // must be called from game_init(), after renderer_init()
```

```cpp
// src/game/game_shaders.cpp
#include "game_shaders.h"
// Generated headers live in build/generated/shaders/ — already on include path for game target.
#include "shield.glsl.h"
#include "plasma_ball.glsl.h"

RendererShaderHandle g_shield_shader = {};
RendererShaderHandle g_plasma_shader = {};

void game_shaders_init() {
    g_shield_shader = renderer_create_shader(shd_shield_shader_desc(sg_query_backend()));
    g_plasma_shader = renderer_create_shader(shd_plasma_ball_shader_desc(sg_query_backend()));
}
```

Call `game_shaders_init()` from `game_init()`, right after `renderer_init()`. Both `game_shaders.h`
and `game_shaders.cpp` are auto-included by the CMake source glob (no build file changes).

### 3.5 Laser Beam Visual: Two Additive Line Quads

No new shader is needed for the laser beam. The core+halo look is achieved with two layered
calls to the extended `renderer_enqueue_line_quad`:

```cpp
// In laser_update() — called every frame while LaserBeam component exists:
renderer_enqueue_line_quad(p0, p1,
    constants::laser_core_width * opacity, constants::laser_core_color,
    BlendMode::Additive);
renderer_enqueue_line_quad(p0, p1,
    constants::laser_halo_width * opacity, constants::laser_halo_color,
    BlendMode::Additive);
```

The renderer routes additive line quads through `pipeline_line_quad_additive` (same line_quad
shader, `blend = ONE/ONE`, depth write off). Layering a thin bright-white quad over a wide
cyan quad produces: a hard glowing core with a soft diffuse halo — the desired visual.

This is simpler than a UV-gradient shader because the "falloff across the beam width" becomes
the hard step between the two quad widths, which is visually acceptable and matches many
sci-fi laser aesthetics. If a smoother UV gradient is desired in future, a dedicated shader
can be added later without changing the API.

### 3.6 Laser Charging Mechanic Design

**Chosen: hold-to-charge, auto-fire at charge completion.**

- Player holds right-mouse (press edge): if weapon is Laser and `weapon_ready()`: add
  `LaserCharge { charge_start = engine_now(), charge_time = constants::laser_charge_time }`.
- While charging (age < `laser_charge_time = 0.8s`):
  - Submit dim aiming beam each frame (aiming beam opacity = `(age/charge_time) * 0.35`).
  - No damage yet.
  - If player releases right-mouse before charge completes: remove `LaserCharge`, no fire.
- When age >= `laser_charge_time`:
  - Execute `laser_fire_instant()`: raycast, apply damage, apply impulse.
  - Remove `LaserCharge`. Add `LaserBeam` component. Update cooldown.
- Every frame in `laser_update()` (unconditional, not input-gated):
  - Entities with `LaserBeam`: age = `engine_now() - lb.fire_time`.  
    If age >= `lb.duration`: remove component. Otherwise:  
    `opacity = (1.0f - age / lb.duration) * (1.0f + 0.3f * sinf(age * 30.0f))`
    — provides a warm flicker in the first ~0.3s.
  - Submit full brightness beam (two additive line quads, per Section 3.5).

### 3.7 Shield Sphere System

#### 3.7.1 Shield as Collision Boundary

The user requirement: *projectiles/laser must collide with the shield sphere, NOT the ship mesh.*

**Required behavior**:
- While `Shield.current > 0`, effective collision radius = `collider.half_extents.x * shield_sphere_scale`.
- Projectile-vs-shielded-entity in `damage_resolve()`:
  ```
  shield_r = col.half_extents.x * constants::shield_sphere_scale
  dist     = length(projectile_pos - target_pos)
  overlap  = dist < (projectile_r + shield_r)
  ```
  If overlap AND `sh.current > 0`: apply damage, `spawn_shield_impact(projectile_pos)`,
  mark projectile DestroyPending. Do NOT fall through to AABB-vs-mesh stage.  
  If `sh.current == 0`: fall through to original AABB overlap (hull hit).
- Laser-vs-shielded-entity in `laser_fire_instant()`: after raycast hit, if target has
  `Shield.current > 0`, compute hit point on shield surface:
  ```cpp
  float shield_r = col.half_extents.x * constants::shield_sphere_scale;
  float t = sphere_intersect_t(muzzle_origin, ray_dir, target_pos, shield_r);
  glm::vec3 hit_point = (t > 0) ? muzzle_origin + ray_dir * t : target_pos;
  spawn_shield_impact(hit_point);
  ```
  Apply damage via `apply_damage()`. No extra AABB check needed.

`sphere_intersect_t` helper (static function in `damage.cpp`):
```cpp
static float sphere_intersect_t(const glm::vec3& ro, const glm::vec3& rd,
                                 const glm::vec3& center, float radius) {
    glm::vec3 oc = ro - center;
    float b    = glm::dot(oc, rd);
    float c    = glm::dot(oc, oc) - radius * radius;
    float disc = b*b - c;
    if (disc < 0.0f) return -1.0f;
    return -b - sqrtf(disc);
}
```

#### 3.7.2 Shield Sphere Entity

Spawned alongside every ship. Uses the game-registered shield shader.

```cpp
// In spawn.cpp — called at end of spawn_player() and spawn_enemy():
static entt::entity spawn_shield_sphere(entt::entity owner, float half_extent) {
    float radius = half_extent * constants::shield_sphere_scale;

    // Build shield material using the game-registered shader handle.
    // ShieldFSParams is defined in shield_vfx.h (game-side, knows the layout).
    Material mat{};
    mat.shader = g_shield_shader;
    mat.pipeline = { BlendMode::AlphaBlend, CullMode::None, false, 2 };
    ShieldFSParams p{};
    p.shield_color  = {0.3f, 0.5f, 1.0f, constants::shield_max_alpha};
    p.view_pos_w    = {};  // updated every frame by shield_vfx_update
    p.fresnel_params = {constants::shield_fresnel_exp, constants::shield_rim_intensity, 0, 0};
    material_set_uniforms(mat, p);

    const auto& ot = engine_get_component<Transform>(owner);
    entt::entity e = engine_spawn_sphere(ot.position, radius, mat);
    auto& ss = engine_add_component<ShieldSphere>(e);
    ss.owner  = owner;
    ss.radius = radius;
    return e;
}
```

`ShieldFSParams` struct is declared in `shield_vfx.h` (not in renderer.h — it is a game
concept). Its layout must exactly match `shield_fs_params` in `shield.glsl`.

#### 3.7.3 shield_vfx_update() — called each frame

```
for each entity with ShieldSphere + Transform + EntityMaterial:
    owner_t  = Transform of ss.owner
    owner_sh = Shield of ss.owner  (if missing: alpha = 0)
    t.position = owner_t.position
    t.scale    = vec3(ss.radius)
    t.rotation = identity

    float shield_alpha = (sh.current / sh.max) * constants::shield_max_alpha
    // Update the view_pos and alpha fields in the uniform blob:
    ShieldFSParams* p = material_uniforms_as<ShieldFSParams>(em.mat)
    p->view_pos_w.x = camera_pos.x  (from CameraRigState of player entity)
    p->view_pos_w.y = camera_pos.y
    p->view_pos_w.z = camera_pos.z
    p->shield_color.a = shield_alpha
    if (shield_alpha < 0.01f): mark entity hidden / set scale to 0
```

#### 3.7.4 Shield hit VFX

When `damage_resolve()` detects a hit on an entity with active shield: call
`spawn_shield_impact(impact_position)`. Creates an expanding+fading sphere per Section 3.8.
Material: `renderer_make_unlit_material({0.5f, 0.7f, 1.0f, 0.9f})` with
`m.pipeline.blend = BlendMode::AlphaBlend; m.pipeline.render_queue = 2;`

**Known ordering limitation**: shield (AlphaBlend, queue=2) and plasma ball (Additive, queue=3)
and laser (Additive, queue=3) execute in fixed render-queue order regardless of depth.
Additive passes are order-independent by definition. Two overlapping shields may composite
slightly wrong (both read depth from opaque geo rather than each other). Accepted limitation
for current scope.

### 3.8 Plasma Ball Enhanced Visual

Change `spawn_projectile()` to use the game-registered plasma shader:

```cpp
Material mat{};
mat.shader   = g_plasma_shader;
mat.pipeline = { BlendMode::Additive, CullMode::None, false, 3 };
PlasmaBallFSParams p{};
p.core_color  = {1.0f, 1.0f, 0.9f, 1.0f};
p.rim_color   = {0.4f, 0.6f, 1.0f, 1.0f};
p.bolt_color  = {0.5f, 0.7f, 1.0f, 1.0f};  // bolt tint (adjustable per projectile)
p.params      = {0.0f, 3.0f, 0.65f, 1.0f}; // time=0 (updated each frame in vfx or never)
p.view_pos_w  = {};                          // updated each frame by the game
material_set_uniforms(mat, p);
```

`PlasmaBallFSParams` struct is declared in `vfx.h` or a dedicated `plasma_material.h`. Its
layout must exactly match `plasma_fs_params` in `plasma_ball.glsl`.

For the plasma ball the view_pos and time can be updated each frame by `vfx_update()` or
`weapon_update()` via `material_uniforms_as<PlasmaBallFSParams>(em.mat)`. If per-frame
updates are skipped (plasma is additive, so view-pos just affects the Fresnel direction —
acceptable approximation to set once at spawn), the ball will look fine without updates.

### 3.9 Impact VFX System (animated expanding sphere)

**New component `VFXData`** (added to `components.h`):
```cpp
struct VFXData {
    float  initial_scale = 1.0f;
    float  final_scale   = 5.0f;
    float  initial_alpha = 1.0f;
    float  final_alpha   = 0.0f;
    float  r = 1.0f, g = 0.5f, b = 0.1f;
};
```

**`vfx_update()`** (in new `vfx.cpp`, called from `game_tick`):
```
for each entity with Lifetime + VFXData + Transform + EntityMaterial:
    double age = engine_now() - lt.spawn_time
    float t = clamp(age / lt.lifetime, 0, 1)
    float sc = lerp(vd.initial_scale, vd.final_scale, t)
    tr.scale = vec3(sc)
    float alpha = lerp(vd.initial_alpha, vd.final_alpha, t)
    // Update the color+alpha in the uniforms blob (assuming UnlitFSParams):
    auto* p = material_uniforms_as<UnlitFSParams>(em.mat)
    p->color = {vd.r, vd.g, vd.b, alpha}
```

**Spawn helpers** (in `vfx.cpp`):
```cpp
void spawn_plasma_impact(const glm::vec3& position);  // orange, 0.5s
void spawn_laser_impact(const glm::vec3& position);   // red-white, 0.4s
void spawn_shield_impact(const glm::vec3& position);  // blue-white, 0.25s
```

Each creates a sphere via `engine_spawn_sphere()` with an unlit material, alpha-blended,
render_queue=2, and attaches `Lifetime` + `VFXData`. `engine_spawn_sphere` must produce
a mesh — pre-flight check T-1 verifies this includes normals.

Impact parameters:

| Type | radius | lifetime | initial_scale | final_scale | initial_alpha | rgb |
|------|--------|----------|---------------|-------------|---------------|-----|
| Plasma | 1.0 | 0.5s | 1.0 | 4.0 | 0.9 | 1.0, 0.5, 0.1 |
| Laser | 1.0 | 0.4s | 0.5 | 3.5 | 1.0 | 1.0, 0.2, 0.05 |
| Shield | 1.0 | 0.25s | 1.5 | 3.0 | 0.9 | 0.5, 0.7, 1.0 |

Impact sphere material setup:
```cpp
Material mat = renderer_make_unlit_material(rgba);
mat.pipeline.blend       = BlendMode::AlphaBlend;
mat.pipeline.depth_write = false;
mat.pipeline.render_queue = 2;
```

### 3.10 Alpha-Cutout Support

Supported automatically by the new material system. Any material with
`pipeline.blend = BlendMode::Cutout` gets a pipeline with depth write ON, no color blending.
The shader must call `if (alpha < 0.5) discard;` itself.

For the built-in unlit shader, add a `flags.y = use_cutout` field (future work — not needed
for the current visual improvements). For now, game-side custom shaders that want cutout
simply include the discard call in their FS.

---

## 4. Pre-Flight Check

### T-1 — Verify engine_spawn_sphere mesh has normals

`shield.glsl` and `plasma_ball.glsl` both read `normal` in their VS. If `engine_spawn_sphere`
returns a position-only mesh (no normal attribute), vertex binding will produce silent black
normals and both shaders will look wrong.

**Action**: open `src/engine/` and find `engine_spawn_sphere` (and the underlying
`renderer_make_sphere_mesh`). Confirm the returned mesh has normals. Looking at
`struct Vertex { float position[3]; float normal[3]; float uv[2]; float tangent[3]; }` in
`renderer.h` — the standard vertex layout always includes normals. Verify that
`renderer_make_sphere_mesh` populates them (smooth shading normals). If it generates
position-only vertices: file a blocker; do not proceed with T04-game and T08.

---

## 5. Task List

### Phase 0 — Foundation (prerequisite human action)

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T-1 | Pre-flight: verify `engine_spawn_sphere` mesh has normals per Section 4. Read-only investigation, no edits. | LOW | (read-only) | — | SEQUENTIAL |
| T00 | **Human approval**: bump renderer.h to v1.2 with the full Material redesign from Section 3.1. Remove `ShadingModel` enum. Update `docs/interfaces/renderer-interface-spec.md`. | LOW | `src/renderer/renderer.h`, `docs/interfaces/renderer-interface-spec.md` | T-1 | BOTTLENECK |

### Phase 1 — New Game Shaders (parallel, disjoint files)

No changes to `shaders/renderer/`. New shaders go in `shaders/game/` (auto-compiled by
existing `game_shaders` CMake target — no build file changes needed).

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T01 | Write `shaders/game/shield.glsl` per Section 3.3.1. Verify with `sokol-shdc -i shaders/game/shield.glsl -o /tmp/test.h -l glsl430`. | MED | `shaders/game/shield.glsl` | — | PG-1 |
| T02 | Write `shaders/game/plasma_ball.glsl` per Section 3.3.2 (include noise function from Appendix A). Verify compilation. | HIGH | `shaders/game/plasma_ball.glsl` | — | PG-1 |

### Phase 2 — Renderer Rework (single file bottleneck)

Depends on T00. T01/T02 can proceed in parallel with T04 (different files).

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T03 | **Renderer implementation** — all changes to `renderer.cpp` and `debug_draw.h` per Sections 3.2. | HIGH | `src/renderer/renderer.cpp`, `src/renderer/debug_draw.h` | T00 | BOTTLENECK |

**T03 detail — ordered subtasks:**

1. **Pipeline cache** (Section 3.2.1):
   - Replace 7 named pipeline fields in `RendererState` with `PipelineCacheEntry pipeline_cache[64]` +
     `int pipeline_cache_count`.
   - Implement `get_or_create_pipeline(RendererShaderHandle, PipelineState)`.
   - Pre-populate index 0 with the magenta fallback (shader_id=0).
   - `renderer_init()`: call `renderer_create_shader` for each built-in shader; store returned handles
     in `RendererState::builtin_shaders[3]`.

2. **`renderer_create_shader()` and `renderer_builtin_shader()`**:
   - `renderer_create_shader(const sg_shader_desc*)`: call `sg_make_shader`, store `sg_shader` handle
     in an internal slot array, return `RendererShaderHandle{id}`.
   - `renderer_builtin_shader(BuiltinShader)`: return pre-stored handle.

3. **Generic draw loop** (Section 3.2.3):
   - Replace the `switch (cmd.material.shading_model)` with the generic pipeline-cache loop.
   - For BlinnPhong and Lambertian shader IDs: patch per-frame uniforms before `sg_apply_uniforms`.
   - For all other shader IDs: forward blob verbatim.
   - Sort draw_queue by `cmd.material.pipeline.render_queue` before the loop.

4. **Additive line-quad pipeline** (Section 3.2.4):
   - Add `pipeline_line_quad_additive` to `RendererState`.
   - `renderer_enqueue_line_quad` checks blend param, routes accordingly.

5. **`renderer_set_time()`**: store `state.current_time = t` (available for game shaders that
   include time in their FS params blob).

6. **Convenience factories** (`renderer_make_unlit_material`, etc.): update to fill uniforms
   blob using `material_set_uniforms<T>` and set `shader = builtin_shader(...)`.

**T03 acceptance**: `cmake --build build --target renderer_app` exits 0; renderer_app shows
colored spheres with BlinnPhong lighting; transparent BlinnPhong sphere is transparent; no GL
errors; existing line_quad VFX still renders.

### Phase 3 — Game Components & Constants (parallel to T03)

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T04 | **Add new components** to `components.h`: `LaserBeam`, `LaserCharge`, `ShieldSphere`, `VFXData` per Sections 3.6, 3.7, 3.9. | LOW | `src/game/components.h` | — | PG-3 |
| T05 | **Add new constants** to `constants.h`. See constants list below. | LOW | `src/game/constants.h` | — | PG-3 |

**T05 new constants:**
```cpp
// Laser
constexpr float  laser_charge_time     = 0.8f;
constexpr float  laser_beam_duration   = 2.0f;
constexpr float  laser_core_width      = 0.3f;
constexpr float  laser_halo_width      = 2.5f;
constexpr float  laser_core_color[4]   = {1.0f, 1.0f, 0.9f, 1.0f};
constexpr float  laser_halo_color[4]   = {0.3f, 0.85f, 1.0f, 1.0f};

// Shield
constexpr float  shield_sphere_scale   = 1.45f;
constexpr float  shield_max_alpha      = 0.55f;
constexpr float  shield_fresnel_exp    = 3.0f;
constexpr float  shield_rim_intensity  = 1.2f;

// Impact VFX
constexpr float  plasma_impact_duration  = 0.5f;
constexpr float  laser_impact_duration   = 0.4f;
constexpr float  shield_impact_duration  = 0.25f;
```

### Phase 4 — Game Systems (multiple parallel groups)

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T06 | **Game shader init**: create `game_shaders.h` and `game_shaders.cpp` per Section 3.4. | LOW | `src/game/game_shaders.h` (new), `src/game/game_shaders.cpp` (new) | T01, T02, T03, T04 | PG-4 |
| T07 | **Laser: persistence + charging mechanic** in `weapons.cpp` per Section 3.6. Remove old one-frame beam code. Implement charge logic, `laser_update()`, additive line-quad submission. Fix beam origin to `muzzle_origin`. | HIGH | `src/game/weapons.cpp` | T03, T04, T05 | PG-4 |
| T08 | **Spawn changes** in `spawn.cpp`. Add `spawn_shield_sphere()`. Call it in `spawn_player()` / `spawn_enemy()`. Change `spawn_projectile()` material to plasma shader per Section 3.8. | MED | `src/game/spawn.cpp` | T03, T04, T05, T06 | PG-4 |
| T09 | **VFX system**: write `vfx.cpp` and `vfx.h`. Implement `vfx_update()`, three spawn helpers per Section 3.9. | MED | `src/game/vfx.cpp` (new), `src/game/vfx.h` (new) | T03, T04, T05 | PG-4 |

**T07 detailed steps:**
1. Remove `k_laser_color`, old `renderer_enqueue_line_quad` call, and single-shot beam logic.
2. Split `laser_fire()` into `laser_fire_instant(entity, ws)` (raycast + damage + return origin/end).
3. Add `laser_update(dt)` scanning entities with `LaserCharge` and `LaserBeam` per Section 3.6.
4. Modify `weapon_update()`: detect press-edge on right-mouse; start charge if weapon ready;
   call `laser_update(dt)` unconditionally each frame.
5. In `laser_fire_instant`: compute `muzzle_origin` from ship forward `(0,1,0)` + offset.
   Store in `LaserBeam.origin`/`.end`.

**T08 detailed steps:**
1. Add `spawn_shield_sphere()` static function per Section 3.7.2.
2. Declare `ShieldFSParams` struct in `shield_vfx.h` (T10 dependency — can stub for now).
3. Call `spawn_shield_sphere(e, k_player_half_extent)` at end of `spawn_player()`.
4. Call `spawn_shield_sphere(e, k_enemy_half_extent)` at end of `spawn_enemy()`.
5. Replace `spawn_projectile()` material with plasma material per Section 3.8.

**T09 detailed steps:**
1. Create `vfx.h` declaring `vfx_update()`, the three spawn functions.
2. Create `vfx.cpp` implementing them per Section 3.9 using `UnlitFSParams` / `material_uniforms_as`.

**After T06–T09 complete — Phase 4B:**

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T10 | **Shield VFX update system**: write `shield_vfx.cpp` and `shield_vfx.h`. Declare `ShieldFSParams` struct (must match `shield_fs_params` in `shield.glsl`). Implement `shield_vfx_update()` per Section 3.7.3. | MED | `src/game/shield_vfx.cpp` (new), `src/game/shield_vfx.h` (new) | T08 | PG-4B |
| T11 | **Impact VFX triggers**: modify `damage_resolve()` in `damage.cpp` to call `spawn_plasma_impact()` / `spawn_shield_impact()`. Modify `laser_fire_instant()` in `weapons.cpp` to call `spawn_laser_impact()` and `spawn_shield_impact()`. Add `sphere_intersect_t()` helper. Implement shield-as-collision-boundary logic per Section 3.7.1. | MED | `src/game/damage.cpp`, `src/game/weapons.cpp` | T07, T09 | PG-4B |

**Final integration:**

| ID | Task | Tier | Files | Depends | Parallel Group |
|----|------|------|-------|---------|----------------|
| T12 | **Game integration**: in `game_init()` add `game_shaders_init()`. In `game_tick()` add calls to `renderer_set_time(engine_now())`, `shield_vfx_update()`, `vfx_update()`. Verify `weapon_update()` calls `laser_update(dt)`. | LOW | `src/game/game.cpp` | T10, T11 | SEQUENTIAL |

No `CMakeLists.txt` changes needed: game `.cpp` sources and `shaders/game/*.glsl` files are both auto-globbed.

---

## 6. Dependency Graph

```
T-1 Pre-flight
    │
T00 Human approval: renderer.h v1.2
    │
    ├── (PG-1) T01 shaders/game/shield.glsl    }── parallel, disjoint files
    │          T02 shaders/game/plasma_ball.glsl}
    │
    ├── T03 renderer.cpp [BOTTLENECK] (needs T00)
    │
    ├── (PG-3) T04 components.h  }── parallel with T03 and each other
    │          T05 constants.h   }
    │
    ▼
T01+T02+T03+T04+T05 done
    │
    ├── (PG-4) T06 game_shaders.cpp/.h  }
    │          T07 weapons.cpp          }── all parallel (disjoint files)
    │          T08 spawn.cpp            }
    │          T09 vfx.cpp/.h           }
    │
T06+T07+T08+T09 done
    │
    ├── (PG-4B) T10 shield_vfx.cpp/.h          }── parallel (disjoint files)
    │            T11 damage.cpp + weapons.cpp   }
    │
T10+T11 done
    │
    └──► T12 game.cpp [FINAL INTEGRATION]
```

---

## 7. Parallel Group Summary

| Group | Tasks | Files (disjoint within group) |
|-------|-------|-------------------------------|
| PG-1 | T01, T02 | `shield.glsl`, `plasma_ball.glsl` |
| PG-3 | T04, T05 | `components.h`, `constants.h` |
| PG-4 | T06, T07, T08, T09 | `game_shaders.*`, `weapons.cpp`, `spawn.cpp`, `vfx.*` |
| PG-4B | T10, T11 | `shield_vfx.*`, `damage.cpp+weapons.cpp` |
| BOTTLENECK | T03 | `renderer.cpp`, `debug_draw.h` |
| SEQUENTIAL | T12 | `game.cpp` |

Note: T11 touches `weapons.cpp` in PG-4B; T07 touches `weapons.cpp` in PG-4. They are in
different phases (T11 depends on T07), no conflict.

---

## 8. Acceptance Criteria per Task

| Task | Acceptance criterion |
|------|----------------------|
| T-1 | Confirmation in writing that `renderer_make_sphere_mesh` populates `Vertex::normal`. If not: blocker filed. |
| T00 | renderer.h compiles cleanly; `ShadingModel` enum is gone; `Material` struct uses `RendererShaderHandle`; `renderer_create_shader` declared. |
| T01 | `sokol-shdc -i shaders/game/shield.glsl -o /tmp/test.h -l glsl430` exits 0 |
| T02 | Same for `plasma_ball.glsl` |
| T03 | `cmake --build build --target renderer_app` exits 0; BlinnPhong sphere renders correctly; transparent BlinnPhong sphere is transparent; additive line quad renders bright; no GL errors. |
| T04 | `cmake --build build --target game` exits 0 (no compile errors from new components) |
| T05 | Same |
| T06 | `cmake --build build --target game` exits 0; shield and plasma shader handles are valid (non-zero id) after `game_shaders_init()` |
| T07 | Hold right-mouse 0.8s → beam fires and persists ~2s with bright core + cyan halo; early release cancels; beam starts from ship nose |
| T08 | Blue translucent sphere visible around player and enemy ships |
| T09 | `cmake --build build --target game` exits 0; no linker errors |
| T10 | Blue sphere tracks ship position; fades as shield depletes; disappears at 0 |
| T11 | Plasma hit on asteroid: orange sphere at impact; shield hit: blue flash; hull hit (shield=0): red-white flash |
| T12 | Full `cmake --build build` exits 0; all systems run each frame without crash |

---

## 9. Notes on Frozen Interface

`renderer.h` is FROZEN at v1.1. All changes require:
1. Human supervisor approval before implementation begins.
2. Version bump to v1.2 in the file and in `docs/interfaces/renderer-interface-spec.md`.
3. Announcement to all workstreams per AGENTS.md §6.

Do not start T03 (renderer.cpp) until T00 is approved and `renderer.h` is updated.

`engine.h` is implicitly affected because `engine_spawn_sphere(pos, radius, material)` takes
a `Material` by value. The struct internals change but the function signature does not. Both
workstreams must recompile after T00 merges to `main`.

---

## Appendix A — Value Noise Function for plasma_ball.glsl

```glsl
float hash31(vec3 p) {
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float vnoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash31(i),              hash31(i+vec3(1,0,0)), f.x),
            mix(hash31(i+vec3(0,1,0)),  hash31(i+vec3(1,1,0)), f.x), f.y),
        mix(mix(hash31(i+vec3(0,0,1)),  hash31(i+vec3(1,0,1)), f.x),
            mix(hash31(i+vec3(0,1,1)),  hash31(i+vec3(1,1,1)), f.x), f.y),
        f.z);
}
```
