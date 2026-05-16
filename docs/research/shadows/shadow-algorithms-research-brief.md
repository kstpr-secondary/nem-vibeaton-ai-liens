# Shadow Algorithms Research Brief — Nem Vibeaton AI Liens

> **Purpose:** Provide a deep-research agent with full context about the project, its rendering architecture, engine capabilities, game domain, and constraints — so it can research, compare, and recommend real-time shadow algorithms suitable for implementation.
> **Date:** 2026-05-16
> **Status:** Draft

---

## 1. Project Overview

**Nem Vibeaton AI Liens** is a C++17, OpenGL 3.3 Core, forward-rendered 3D space shooter built across three tightly coupled workstreams:

| Workstream | Repository path | Executable | Role |
|------------|-----------------|------------|------|
| **Renderer** | `src/renderer/` | `renderer_app` | Window/app lifecycle (sokol_app), GPU pipeline creation, shader compilation (sokol-shdc), mesh upload, draw submission, skybox, line-quads, Dear ImGui integration |
| **Engine** | `src/engine/` | `engine_app` | ECS scene management (entt v3.13.2), asset import (cgltf glTF, tinyobjloader OBJ), 6-DOF Euler physics at 120 Hz, AABB + convex-hull collision, raycasting, camera matrix computation, input routing |
| **Game** | `src/game/` | `game` | Player controls, combat loop, enemy AI, HUD (ImGui), match state machine, game-layer ECS components |

The project uses **FetchContent-only** dependency management, CMake + Ninja build, and Clang with `-Wall -Wextra -Wpedantic`. No Vulkan, no DirectX. OpenGL 3.3 Core Profile is the hard floor and ceiling.

---

## 2. Rendering Architecture

### 2.1 Frame Pipeline

```
sokol_app frame event
  └► renderer internal frame_callback()
       ├─ simgui_new_frame()              (ImGui frame start)
       ├─ call registered FrameCallback(dt, user_data)
       │    └─ consumer calls:
       │         renderer_begin_frame()
       │         renderer_set_camera(view, projection)
       │         renderer_set_directional_light(dir, color, intensity)
       │         renderer_set_skybox(cubemap)
       │         renderer_enqueue_draw(mesh, transform, material) × N
       │         renderer_enqueue_line_quad(p0, p1, width, color, blend) × M
       │         ImGui::* widget calls
       │         renderer_end_frame()
       └─ renderer post-frame:
             ├─ pass 0: skybox (depth write OFF, pos.xyww trick)
             ├─ pass 1: opaque draws, sorted front-to-back (render_queue=0)
             ├─ pass 2: cutout (render_queue=1), transparent (render_queue=2)
             ├─ pass 3: additive / line quads (render_queue=3)
             └─ pass 4: simgui_render() (ImGui UI overlay)
```

### 2.2 Frozen Renderer Interface (`docs/interfaces/renderer-interface-spec.md`, v1.2)

The renderer exposes a single header `renderer.h`. All consuming code includes only this header. Key APIs relevant to shadows:

| API | Purpose |
|-----|---------|
| `renderer_set_directional_light(const DirectionalLight&)` | Sets the single directional light (direction, color/intensity). Called once per frame by engine. |
| `renderer_enqueue_draw(mesh, world_transform[16], material)` | Per-entity draw submission with mat4 model matrix and Material struct. |
| `Material` | Holds `RendererShaderHandle`, `PipelineState`, 256-byte uniform blob, up to 4 texture slots. |
| `renderer_create_shader(const sg_shader_desc*)` | Register a custom shader; returns opaque handle. |
| `renderer_builtin_shader(BuiltinShader)` | Returns handle to Unlit / BlinnPhong / Lambertian built-in shaders. |
| `RendererCamera { view[16], projection[16] }` | Column-major 4×4 matrices. |

**Constraint:** The renderer enqueue API takes a single `world_transform[16]` per draw call. There is no multi-pass render-to-texture or framebuffer object (FBO) API exposed — this must be added if shadow maps require offscreen rendering.

### 2.3 Current Shaders

| Shader | File | Lighting Model | Notes |
|--------|------|----------------|-------|
| Unlit | `shaders/renderer/unlit.glsl` | None (flat color) | Has texture flag support |
| Lambertian | `shaders/renderer/lambertian.glsl` | Diffuse only (N·L) | Single light; no specular |
| Blinn-Phong | `shaders/renderer/blinnphong.glsl` | Diffuse + specular (N·H)^shininess | Has texture support, ambient term (0.15 multiplier) |
| Skybox | `shaders/renderer/skybox.glsl` | Cubemap sample only | Uses pos.xyww trick for depth |
| Line Quad | `shaders/renderer/line_quad.glsl` | N/A | Additive blending for lasers/VFX |
| Magenta | `shaders/renderer/magenta.glsl` | N/A | Fallback on shader failure |

**Shader compilation pattern:** `.glsl` files are annotated with sokol-shdc dialect (`@vs`, `@fs`, `@block`, `@program`) and compiled by CMake custom command into generated headers at `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. Runtime shader creation failures fall back to the magenta pipeline.

**Universal VS block convention:** Every mesh vertex shader MUST declare `layout(binding=0)` with a 192-byte uniform containing `mat4 mvp; mat4 model; mat4 normal_mat;` (3 matrices × 64 bytes = std140). Fragment shaders use `layout(binding=1)` for light/material params.

### 2.4 Current Lighting

- **Single directional light** only (`renderer_set_directional_light`). One direction, one color, one intensity scalar.
- Light direction is in world space. Shaders compute `L` as the direction FROM surface TO light.
- Blinn-Phong shading includes: diffuse (N·L), specular (N·H)^shininess × 0.5, and ambient (albedo × 0.15).
- No point lights, no spotlights, no shadow maps currently.

### 2.5 Render Queues & Sorting

| Queue | Type | Sort Order | Depth Write |
|-------|------|------------|-------------|
| 0 | Opaque | Front-to-back | ON |
| 1 | Cutout | Front-to-back | ON |
| 2 | Transparent | Back-to-front | OFF |
| 3 | Additive / Line Quads | Unsorted (basic) | OFF |

Skybox is drawn between queues 1 and 2. Z-sorting is unconditional for all opaque/cutout/transparent draws.

---

## 3. Engine Capabilities

### 3.1 ECS Component Schema (`src/engine/components.h`)

| Component | Purpose |
|-----------|---------|
| `Transform` | position (vec3), rotation (quat), scale (vec3) |
| `Mesh` | RendererMeshHandle |
| `EntityMaterial` | Material struct with uniforms blob + textures |
| `RigidBody` | mass, inv_mass, linear/angular velocity, inertia tensors, restitution |
| `Collider` | half_extents (AABB) |
| `Light` | direction, color, intensity — single directional at MVP |
| `Camera` | fov, near_plane, far_plane |
| `ForceAccum` | transient force/torque for physics substeps |
| `ConvexCollider` | pointer to precomputed convex hull cache + uniform scale |

Tag components: `Static`, `Dynamic`, `Interactable`, `CameraActive`, `DestroyPending`.

### 3.2 Physics

- **6-DOF semi-implicit Euler** integration at fixed 120 Hz substep rate (dt-capped at 100ms).
- **Impulse-based elastic collision response** with restitution = 1.0 and Baumgarte positional correction (k_slop=0.005, k_baumgarte=0.3).
- **Collision detection:** AABB broadphase (brute-force O(N²)); convex-hull SAT narrowphase for entities with `ConvexCollider` component.
- **Zero gravity** (space environment). Forces applied explicitly by game logic.

### 3.3 Asset Pipeline

- glTF/GLB via `cgltf` v1.15; OBJ fallback via `tinyobjloader` v2.0.0rc13.
- Meshes uploaded to GPU via `renderer_upload_mesh(vertices, indices, layout)`.
- Vertex layout: `Position(3) + Normal(3) + UV(2) + Tangent(3)` = 14 floats per vertex.
- Available assets: 4 asteroid GLBs (different LODs), 3 spaceship GLBs, skybox cubemap, textures directory.

### 3.4 Camera & Frustum

- Engine computes view matrix as `inverse(camera_entity_transform)` and projection from `Camera` component (fov, near, far).
- Camera matrices pushed to renderer each frame via `renderer_set_camera()`.
- Frustum culling is available (`renderer_set_culling_enabled(bool)`).

---

## 4. Game Domain & Context

### 4.1 Setting

Third-person space shooter in a spherical containment field (~1 km radius) containing 200 procedurally-placed asteroids of varying sizes. The player fights enemy ships among the asteroids.

### 4.2 Visual Style Implications for Shadows

- **Space environment** — no natural ambient occlusion from atmosphere; hard directional light (sun-like).
- **Asteroid field** — 200 dynamic, tumbling objects with irregular convex-hull geometry; self-shadowing and cast shadows on each other are visually significant.
- **Spaceship models** — glTF meshes with potential texture detail; ship-to-ground/asteroid shadows would add depth perception in the void.
- **Containment sphere** — a large spherical boundary; could receive shadows or serve as a shadow receiver.
- **No atmosphere, no ground plane** — shadows land on asteroids and ships only, not on a flat surface. This eliminates simple shadow-plane projection tricks.

### 4.3 Performance Budget (MVP)

- ~200 asteroids + 3-4 ships + projectiles + VFX ≈ 250+ draw calls minimum.
- Target: maintain playable framerates on modest hardware (OpenGL 3.3 Core means no compute shaders, no mesh shaders).
- Culling is available but not yet heavily optimized.

---

## 5. Constraints & Requirements for Shadow Implementation

### 5.1 Hard Constraints

1. **OpenGL 3.3 Core Profile only.** No FBO extensions beyond core, no compute shaders, no tessellation, no geometry shaders (unless verified available as core or ARB extension on target HW).
2. **Forward rendering architecture.** The renderer submits draws in a single forward pass — no deferred shading, no G-buffers.
3. **Single directional light.** Shadow algorithm must work with one light source at MVP.
4. **sokol_gfx API.** Offscreen render targets (for shadow maps) must be created via `sg_make_image()` + `sg_make_sampler()` + `sg_make_pass_attachment()` within sokol_gfx constraints.
5. **sokol-shdc shader dialect.** All shaders go through sokol-shdc for cross-backend compatibility. Shadow shaders must conform to `@vs`, `@fs`, `@block` annotations.
6. **Frozen interfaces.** Neither `renderer-interface-spec.md` nor `engine-interface-spec.md` may be modified without explicit human approval.

### 5.2 Interface Extension Needs (to be evaluated)

Any shadow algorithm will likely require:

- **Offscreen rendering capability:** Shadow maps need a render target (texture + framebuffer). This means either extending the renderer API to support FBO/render-pass attachments, or adding a dedicated shadow-pass pipeline.
- **Shadow-aware shaders:** Either modify existing BlinnPhong/Lambertian shaders to sample a shadow map texture and modulate darkness, or create new shadow-receiving shader variants.
- **Depth-pass pipeline:** A separate "shadow caster" pipeline that renders only depth (no color) from the light's perspective.
- **Material extension:** Materials need a flag or field indicating whether an entity casts shadows, receives shadows, or both.

### 5.3 Acceptable Shadow Algorithms (OpenGL 3.3 Friendly)

The research agent should evaluate these algorithms against the constraints:

| Algorithm | GL 3.3 Support | Pros | Cons |
|-----------|----------------|------|------|
| **Shadow Maps** (standard) | Full core — depth texture + depth comparison sampler | Industry standard; works on all HW; per-pixel accuracy | Z-fighting in shadow map; aliasing at edges; requires FBO for depth pass |
| **PCF (Percentage-Closer Filtering)** | Full core — `texture()` with depth comparison + `GL_COMPARE_REF_TO_TEXTURE` | Smooth shadow edges; simple extension of shadow maps | Requires 4× or more texture samples; no bilinear filtering on depth textures in GL 3.3 without `GL_OES_texture_float_linear` |
| **Cascaded Shadow Maps (CSM)** | Full core (multiple shadow maps) | Good for large scenes with varying LOD | Complex; multiple depth passes; overkill for small scenes |
| **Soft Shadows (VSM/ESM)** | Full core — variance/exponential techniques | Naturally soft; few samples | Artifact-prone (light leakage); requires moment textures |
| **Contact Hardening Soft Shadows (CHSSM)** | Full core | Realistic penumbra | Complex; multiple shadow map passes |
| **Shadow Volumes** | Core + stencil extension (`GL_EXT_stencil_wrap`) | Pixel-perfect, alias-free | Stencil overflow on complex geometry; very expensive for 200+ asteroids |
| **Spherical Harmonics AO** | Full core (no FBO needed) | No extra passes; cheap | Not true shadows; approximation only; poor for directional occlusion |
| **Screen-Space Shadows (SSR-style)** | Core + FBO (ping-pong) | Perspective-correct; no light-space artifacts | Only visible pixels; expensive; aliasing at screen edges |
| **Ray Traced Shadows (GL 3.3 via shaders)** | No — requires compute or ray tracing extensions | Not feasible on GL 3.3 |

### 5.4 Most Promising Candidates for This Project

Based on the constraints, the research agent should prioritize evaluating:

1. **Standard Shadow Maps + PCF** — The most straightforward fit. Single depth pass from light perspective, then modulate existing BlinnPhong shading. Well-understood; minimal architecture changes.
2. **VSM (Variance Shadow Maps)** — If soft shadows are desired without multiple taps. Extends shadow maps with moment textures.
3. **Shadow Volumes** — Only if the team wants pixel-perfect shadows and can handle stencil complexity. Worth evaluating for comparison but likely too expensive for 200+ dynamic asteroids.

---

## 6. Integration Points (Where Shadows Would Go)

### 6.1 Renderer Side

```
renderer_begin_frame()
  ├─ [NEW] Create/activate shadow FBO + set light-space view/projection
  ├─ [NEW] Shadow depth pass: render all shadow-casting entities from light perspective
  ├─ [NEW] Deactivate shadow FBO; restore default framebuffer
  ├─ engine_tick(dt)        ← engine calls renderer_set_camera, renderer_enqueue_draw
  │    └─ each enqueue_draw must now carry shadow cast/receive flags
  ├─ [NEW] For shadow-receiving draws: bind shadow map texture + light-space matrices
  ├─ [NEW] Shadow-aware shader variant samples shadow map in fragment shader
  └─ simgui_render()
```

### 6.2 Engine Side

- Entities need shadow cast/receive capability. This could be added as:
  - New component `ShadowCaster { bool cast = true; bool receive = true; }`
  - Or a flag within existing `EntityMaterial`
- Light-space view/projection matrices must be computed and passed to renderer (currently only camera matrices are computed).

### 6.3 Shader Changes

The existing BlinnPhong fragment shader would need modification:

```glsl
// Current (simplified):
vec3 diffuse = albedo_rgb * light_color * intensity * NdotL;
frag_color = vec4(ambient + diffuse + spec, alpha);

// With shadow map:
float shadow = shadow_sample(light_depth_tex, light_space_pos, world_pos, N);
frag_color = vec4((ambient + diffuse + spec) * shadow, alpha);
```

A new "shadow depth" vertex shader would render only position → clip space from the light's viewpoint.

---

## 7. Deliverable Expected from Research Agent

The research agent should produce a **Shadow Algorithm Evaluation Report** containing:

1. **Recommendation:** A single recommended algorithm with justification based on: GL 3.3 compatibility, architecture fit, performance impact, visual quality for the space shooter domain, implementation complexity, and frozen-interface impact.
2. **Ranking:** Comparison table of top 3-5 algorithms evaluated against the criteria above.
3. **Architecture Impact Assessment:** What changes are needed in renderer API, engine API, shaders, and materials. Specifically: what new functions must be added to `renderer.h`, whether interface freeze rules apply, and estimated scope.
4. **Implementation Outline:** High-level phase breakdown for the recommended algorithm (e.g., Phase 1: shadow map depth pass + hard shadows; Phase 2: PCF softening).
5. **Risk Assessment:** Known pitfalls (z-fighting in shadow maps, stencil overflow for volumes, etc.) and mitigation strategies.

---

## 8. Reference Documents

| Document | Path |
|----------|------|
| Renderer Interface Spec (FROZEN v1.2) | `docs/interfaces/renderer-interface-spec.md` |
| Engine Interface Spec (FROZEN v1.3) | `docs/interfaces/engine-interface-spec.md` |
| Renderer Architecture | `docs/architecture/renderer-architecture.md` |
| Engine Architecture | `docs/architecture/engine-architecture.md` |
| Game Design Document | `docs/game-design/GAME_DESIGN.md` |
| AGENTS (project operating guide) | `AGENTS.md` |
| WORKFLOW (feature process) | `WORKFLOW.md` |
| Renderer Skill | `.agents/skills/renderer-specialist/SKILL.md` |
| Engine Skill | `.agents/skills/engine-specialist/SKILL.md` |
| GLSL Patterns | `.agents/skills/glsl-patterns/SKILL.md` |
| Sokol-shdc Skill | `.agents/skills/sokol-shdc/SKILL.md` |
| Sokol API Skill | `.agents/skills/sokol-api/SKILL.md` |

**Source code:** `src/renderer/`, `src/engine/`, `src/game/`
**Shaders:** `shaders/renderer/` (sokol-shdc annotated GLSL)
**Assets:** `assets/` (glTF models, skybox cubemap, textures)

---

*End of brief.*
