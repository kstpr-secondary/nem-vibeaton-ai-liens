# Renderer — Concept, MVP, Milestones

Seed document for Renderer SpecKit. Concise but complete. Supersedes the renderer sections of `Milestones_MVPs_Concepts.md` after the feasibility review.

---

## Concept

A forward-rendered, mid-2010s-era graphics library built on `sokol_gfx` (graphics) and `sokol_app` (window + input). Target: desktop Linux; primary backend **Vulkan**, fallback **OpenGL 3.3 Core** via CMake flag. Shaders are authored as annotated `.glsl` and **precompiled via `sokol-shdc`** into per-backend headers (no runtime GLSL). Renderer owns `sokol_app` init, the main frame callback, and input plumbing; the game engine ticks from inside the frame callback.

Scene philosophy: the renderer does not own a scene graph. The game engine enqueues draws each frame; the renderer executes them.

Cuts (fixed): shadows (until stretch), PBR, deferred / clustered forward, post-processing, AO, MSAA, GPU-driven culling, MDI, audio — none of these exist.

---

## Public API Surface (freeze before Engine SpecKit)

Callable by engine (and `renderer_app`) — names illustrative, SpecKit confirms final signatures.

- **Lifecycle:** `init(config)`, `shutdown()`, `run()` (owns `sokol_app` main loop). `config` carries resolution, backend override, clear color.
- **Per-frame:** `begin_frame()`, `enqueue_draw(mesh, transform, material)`, `enqueue_line(p0, p1, color, thickness)`, `end_frame()`.
- **Meshes:**
  - Procedural builders: `make_sphere_mesh(radius, subdiv)`, `make_cube_mesh(extents)`, `make_capsule_mesh(radius, height, subdiv)`. **Live in renderer** — engine and `renderer_app` consume; no duplication elsewhere.
  - Upload path: `upload_mesh(vertices, indices, layout)` for engine-imported assets (glTF/OBJ).
- **Materials:** `make_material(base_color, shading_model [, texture_handle, custom_shader_handle])`. Shading model enum: `Unlit`, `Lambertian`, `BlinnPhong` (later).
- **Textures:** `upload_texture_2d(pixels, w, h, format)`, `upload_cubemap(...)`.
- **Lights:** `set_directional_light(dir, color, intensity)`. MVP has exactly one directional light.
- **Camera:** `set_camera(view_matrix, projection_matrix)` — engine computes matrices; renderer just applies.
- **Skybox:** `set_skybox(cubemap_handle)` or `set_skybox_equirect(texture_handle)`.
- **Input bridge:** renderer exposes an input-event callback registration; engine registers and receives raw `sokol_app` events.

---

## Milestone Group — MVP

### R-M0 — Bootstrap
*Target: ≤45 min. Skippable as a sync point (no downstream deliverable).*

- Sokol init with Vulkan primary + GL 3.3 fallback toggled via CMake flag (`-DRENDERER_BACKEND=VULKAN|GL`).
- `sokol_app` window (fixed resolution, fullscreen), clear-color frame.
- Input event pump exposed to engine via callback.
- `renderer_app` driver renders a solid clear color.
- **Hard gate:** decide Vulkan-vs-GL here. If Vulkan experimental backend misbehaves on any of the three laptops, switch the default to GL before R-M1 starts. Do *not* let this decision slip into shader work.

**Expected outcome:** `renderer_app` runs; window clears to color; keyboard/mouse events print to log.

**Files (indicative):** `renderer_core.{cpp,h}`, `renderer_app/main.cpp`, CMake backend flag.

### R-M1 — Unlit Forward Rendering + Procedural Scene
*Target: ~1h. **Sync point for engine workstream.***

- Forward render pass, opaque queue (unsorted).
- Perspective camera.
- `sokol-shdc` CMake custom command producing `<name>.glsl.h`; one unlit vertex+fragment program through it end-to-end.
- Unlit solid-color shader.
- Public API for `enqueue_draw` + mesh + material + camera.
- Procedural shape builders (sphere / cube / capsule) in renderer public API.
- `renderer_app` driver: hardcoded procedural scene — assorted colored primitives at varied positions/rotations/sizes in front of a fixed camera.

**Expected outcome:** `renderer_app` displays colored unlit primitives. Engine workstream can begin wiring its mock against the real API.

**Files:** `pipeline_unlit.{cpp,h}`, `shaders/renderer/unlit.glsl`, `mesh_builders.{cpp,h}`, `camera.{cpp,h}`, CMake `sokol-shdc` integration.

**Parallel hints:** shader (`unlit.glsl`) can run alongside C++ draw path if shader author is on a separate agent.

### R-M2 — Directional Light + Lambertian
*Target: ~45 min. Depends on R-M1.*

- Directional light uniform block (direction, color, intensity).
- Lambertian fragment shader via sokol-shdc.
- Material base-color parameter.
- Runtime toggle between unlit and Lambertian pipelines (debug).

**Expected outcome:** Procedural scene shaded by a single directional light; animated camera orbit shows expected shading response across object surfaces.

**Files:** `shaders/renderer/lambertian.glsl`, light uniform block in `renderer_core.h`, pipeline selector in `renderer_core.cpp`.

**Parallel hints:** `lambertian.glsl` (shader) and C++ uniform-block/pipeline plumbing are file-disjoint → natural PG-A/PG-B split. Uniform block struct is a bottleneck: define `sg_directional_light_t` first, mark BOTTLENECK.

### R-M3 — Skybox + Line Rendering
*Target: ~45 min. Depends on R-M2 (or concurrent with it after the uniform-block bottleneck clears — file sets are disjoint).*

- Skybox: cubemap texture; separate pass behind all opaque geometry; depth write off, depth test off, draw first or use depth-trick.
- Depth-tested line primitive: thickness via geometry shader or CPU-side quad expansion. Supports colored segments for laser beams.

**Expected outcome:** Starfield / space background renders behind the scene without z-fighting; lines render with correct depth occlusion against scene geometry. **Renderer MVP complete** — sufficient for the game: lit meshes, starfield, laser visuals.

**Files:** `skybox.{cpp,h}`, `shaders/renderer/skybox.glsl`, `debug_draw.{cpp,h}`, `shaders/renderer/line.glsl`. Two file-disjoint sub-efforts → parallel group.

---

## Milestone Group — Desirable

### R-M4 — Blinn-Phong + Diffuse Textures
*Depends on R-M2/M3.*

- Specular term + shininess material param; pipeline selector extended.
- 2D texture sampling (albedo only), texture upload API + loader (stb_image or equivalent).
- Scene demo: mix of Lambertian and Blinn-Phong materials, some textured.

### R-M5 — Custom Shader Hook + Alpha-Blended Queue
*Depends on R-M4.*

- API for engine-supplied fragment shader attached to a material (plasma, fresnel, explosion). Applied on quads or simple primitives or lines.
- Alpha-blended transparency queue, sorted back-to-front.
- Scene demo: animated plasma-shader sphere on top of an opaque/textured object.

### R-M6 — Frustum Culling + Front-to-Back Sort + Stress Test
*Pure optimization. Only land if stress test shows a problem.*

- Compute frustum planes from camera + projection.
- AABB-vs-frustum per draw.
- Front-to-back sort by distance to camera for opaque queue.
- `renderer_app` stress mode: scale procedural scene up to measure FPS.

---

## Milestone Group — Stretch

### R-M7 — Normal Maps + Procedural Sky
*Only if time. Procedural sky already flagged optional.*

- Tangent-space attribute in mesh layout; normal map sampler in Blinn-Phong / PBR shader.
- Procedural sky shader (stars / nebula) as an alternative skybox.

### R-M8 — Shadows + PBR + IBL
*Last-hour exploration only.*

- Shadow mapping adapted from sokol's shadow sample.
- PBR Cook-Torrance (no subsurface, no cloth).
- IBL environment reflections from the skybox cubemap.

**Cut entirely from planning:** MSAA, deferred / clustered forward, AO, GPU-driven culling, MDI. *Optional probe:* GPU instancing and draw-call batching may be slipped into earlier milestones if the agent finds them trivial on sokol; otherwise skip.

---

## Shared Ownership Decisions

- **Procedural shape builders:** renderer owns. Engine wraps them into entity-creation helpers. No duplication.
- **`sokol_app` init + main frame callback:** renderer owns. Engine ticks from inside the callback.
- **Input plumbing:** renderer owns the `sokol_app` event pump; engine registers a callback.
- **CMakeLists.txt:** renderer workstream owns; cross-workstream build changes require 2-minute notice (blueprint §8.5 rule 11).
- **Shader dialect:** `sokol-shdc` annotated GLSL; documented in `.agents/skills/sokol-shdc/SKILL.md`.

---

## Resolved Open Decisions

- **Alpha-blended queue:** desirable (R-M5), *not* MVP. MVP laser and plasma are opaque.
- **Shader pipeline:** precompiled via `sokol-shdc`. Runtime GLSL cut. Hot-reload cut.
- **Vulkan vs GL:** decided inside R-M0; GL fallback via build flag always available.
- **Culling / sorting / stress test:** demoted to R-M6 (desirable), not MVP.

---

## Notes for SpecKit

- Each milestone maps to ~3–6 tasks; split by file-disjoint boundaries for parallel groups where possible.
- Mocks: an engine-facing stub header exposing the Public API Surface above must be generable at T+0 so the engine workstream can start before R-M1 lands.
- Acceptance checklist per milestone: build passes, `renderer_app` demonstrates the expected outcome visually, smoke test script exits 0.
- Interface spec output: `renderer-interface-spec.md` (per blueprint §3.3 workflow). Frozen before Engine SpecKit runs.
- `renderer_app` source lives at `src/renderer/app/main.cpp`; it is the canonical driver for every milestone demo and must be updated at each milestone to exercise the new feature.
