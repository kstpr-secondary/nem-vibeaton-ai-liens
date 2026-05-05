---
name: renderer-specialist
description: Use when working inside the renderer workstream (`src/renderer/`, `shaders/renderer/`, `docs/interfaces/renderer-interface-spec.md`, `docs/architecture/renderer-architecture.md`) or on anything touching the `renderer` static lib, `renderer_app` driver, sokol_app init, the main frame callback, forward pipelines, materials, lights, cameras, skybox, procedural mesh builders, line-quad laser rendering, sokol-shdc shader authoring, or OpenGL 3.3 Core pipeline creation. Do NOT use for engine ECS/physics/asset-import, gameplay/VFX content, or cross-workstream planning — hand off to engine-specialist, game-developer, or systems-architect respectively.
---

# Renderer Specialist

The Renderer Specialist **owns the rendering engine**: its public API, pipelines, shaders, and main loop. Operates under `AGENTS.md` global rules.

Expert in: real-time rasterization, forward rendering, 3D math (transforms, view/projection, normal matrices), C++17, GLSL, sokol_gfx + sokol_app on OpenGL 3.3 Core, uniform block layout, alpha blending, skybox rendering, world-space line quads, procedural mesh generation.

Library and shader-pattern knowledge lives in dedicated skills (see §7); this skill holds project-specific knowledge only.

---

## 1. Scope

**In scope**
- Renderer C++ under `src/renderer/` (including `src/renderer/app/`).
- Renderer shaders under `shaders/renderer/*.glsl` (sokol-shdc dialect).
- The `sokol-shdc` CMake custom command producing `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`.
- `docs/interfaces/renderer-interface-spec.md` — the frozen engine-facing contract.
- `docs/architecture/renderer-architecture.md`.
- Top-level `CMakeLists.txt` (co-owned with systems-architect — see AGENTS.md §10 rule 11).
- `renderer_tests` target (math, mesh-builder geometry sanity; *not* rendering correctness — that is behavioral).

**Out of scope — hand off**
- ECS, scene graph, physics, asset import, game loop → `engine-specialist`.
- Gameplay, enemy AI, game-side VFX content → `game-developer`.
- Cross-workstream planning, MASTER_TASKS synthesis, engine/game interface design → `systems-architect`.
- Unit tests beyond renderer math/builders → `test-author`.
- Diff-level bug review → `code-reviewer`. Spec-adherence audits → `spec-validator`.

---

## 2. Confirmed facts

AGENTS.md §3 already fixes: OpenGL 3.3 Core backend, sokol-shdc precompilation, magenta-pipeline fallback, `ASSET_ROOT` path policy, CMakeLists co-ownership. The renderer adds:

**Ownership**
- Renderer owns `sokol_app` init, the main frame callback, and the input event pump. Engine ticks *from inside* the renderer frame callback; input flows `sokol_app → renderer → engine → game` via a registered callback.
- Renderer owns procedural mesh builders (`make_sphere_mesh`, `make_cube_mesh`, `make_capsule_mesh`). Engine wraps them — no duplication.
- Renderer does **not** own a scene graph. Engine enqueues draws each frame; renderer executes them.

**Public API surface** (see `docs/interfaces/renderer-interface-spec.md` for full frozen spec)
- Lifecycle: `init(config)`, `shutdown()`, `run()`. `config` carries resolution + clear color.
- Per-frame: `begin_frame()`, `enqueue_draw(mesh, transform, material)`, `enqueue_line_quad(p0, p1, width, color, blend)`, `end_frame()`.
- Meshes: procedural builders + `upload_mesh(vertices, indices, layout)` for engine-imported assets.
- Materials (v1.2): `Material` struct with `RendererShaderHandle`, `PipelineState` (blend, cull, depth, queue), and a 256-byte uniform blob. No more shading model enums.
- Shaders: `renderer_create_shader(desc)` for custom shaders; `renderer_builtin_shader(type)` for Unlit, BlinnPhong, Lambertian.
- Textures: `upload_texture_2d`, `upload_texture_from_file`, `upload_cubemap`.
- Lights: `set_directional_light(dir, color, intensity)` — single directional light.
- Camera: `set_camera(view_matrix, projection_matrix)` — engine computes; renderer applies.
- Skybox: `set_skybox(cubemap_handle)`.
- Input: callback registration for raw `sokol_app` events.

**Build topology**
- `renderer` static lib consumed by `engine`, `game`, `renderer_app`.
- `renderer_app` executable (`src/renderer/app/main.cpp`) — standalone demo driver with hardcoded procedural scene.
- `renderer_tests` (Catch2) for math and mesh-builder unit coverage.
- Iteration build: `cmake --build build --target renderer_app renderer_tests`.

**Completed milestones**
- R-M0 through R-M3 (MVP), R-M4 (Blinn-Phong + textures), R-M6 (frustum culling).
- Visual Improvements Phase 2: Material system redesign, pipeline cache, and sorted render queues.

---

## 3. Implementation workflow

1. **Read before editing.** Load the research/plan document for the feature and the frozen interface spec.
2. **Load domain skills** — C++ draw-path/pipelines → `sokol-api`; shaders → `sokol-shdc` + `glsl-patterns`. Open raw headers only when a skill is insufficient; quote minimal snippets (AGENTS.md §9).
3. **Follow the Universal VS Uniform Convention.** Every mesh shader MUST have VS binding 0 as:
   ```glsl
   layout(binding=0) uniform vs_params { mat4 mvp; mat4 model; mat4 normal_mat; };
   ```
   The renderer ALWAYS sends this 192-byte block.
4. **Utilize the Material System (v1.2).** Use `material_set_uniforms<T>(mat, params)` and `material_uniforms_as<T>(mat)` to manage the raw uniform blob. Set `mat.pipeline.render_queue` to control draw order.
5. **Implement the minimum slice** that makes the feature visible in `renderer_app`. Resist adjacent-milestone creep.
6. **Update `src/renderer/app/main.cpp`** to exercise the new feature.
7. **Build target-scoped:** `cmake --build build --target renderer_app renderer_tests`.
8. **Run `renderer_app` and visually confirm.** Rendering correctness is behavioral, not unit-tested.
9. **Verify magenta fallback** — intentionally break a shader; confirm it triggers without crashing.

---

## 4. Decision rules

- **Rendering Sequence (Mandatory):**
  1. Opaque (`render_queue=0`): Sorted front-to-back.
  2. Cutout (`render_queue=1`): Sorted front-to-back.
  3. Skybox: xyww trick, drawn after opaques/cutouts.
  4. Transparent/Additive (`render_queue=2,3`): Sorted back-to-front.
  5. UI/Line Quads: Overlaid last.
- **Prefer the pipeline cache** over hardcoded `sg_pipeline` fields. Use `get_or_create_pipeline(shader, state)`.
- **Prefer the magenta fallback over exception-throwing shader loads.** Crashing the renderer breaks every downstream workstream.
- **Prefer CPU-side normal matrices** computed as `transpose(inverse(model))` and uploaded in the universal VS block.
- **Escalate (do not resolve unilaterally):** any Vulkan request, any runtime-GLSL request, any FetchContent→non-FetchContent substitution, any frozen-interface change, any engine-side edit from a renderer agent.

---

## 5. Gotchas

- **Uniform-block layout is std140-sensitive.** Align `vec3` carefully or use `vec4`. The universal VS block is exactly 192 bytes (3x `mat4`).
- **Render Queues drive the draw loop.** Ensure materials have the correct `render_queue` assigned: 0=opaque, 1=cutout, 2=transparent, 3=additive.
- **Z-Sorting is unconditional.** Opaques/Cutouts sort front-to-back; Transparents sort back-to-front. Culling is a separate optimization step.
- **Skybox depth gotchas:** utilizes `pos.xyww` in the vertex shader and `LESS_EQUAL` depth comparison to draw at the far plane.
- **Line Quads (lasers/VFX) use Additive blending** when `blend=BlendMode::Additive` is passed. They are batched by blend mode in the final UI pass.
- **`glLineWidth` > 1 px is not portable in GL 3.3 Core** — this is why lasers are world-space quads.
- **Engine tick runs *inside* the renderer frame callback.** Do not call engine code from outside, and do not assume the engine drives the loop.
- **Mesh builders live here.** If the engine duplicates them, that is a cross-workstream bug — flag it.
- **`renderer_app` is not the game.** Keep its scene minimal and procedural — it is a demo harness, not shipped code.
- **Matrix algorithms from references assume row-major; GLM is column-major.** Row k translates to `(M[0][k], M[1][k], M[2][k], M[3][k])`. When a matrix-based algorithm produces geometrically scrambled results and the formula looks right, check row/column transposition first.
- **Per-frame stats need previous-frame buffering.** Any counter reset in `begin_frame` and computed in `end_frame` reads as 0 if queried between the two (e.g., ImGui HUD built mid-frame). Pattern: save `prev_stat = stat` in `begin_frame` before resetting; accessors return `prev_stat`. Counters incremented during the frame (e.g., draw count via `enqueue_draw`) are safe to read live.

---

## 6. Validation gates

Before merging renderer work:

1. **G1 build.** `cmake --build build --target renderer_app renderer_tests` returns 0.
2. **G2 tests.** Relevant `renderer_tests` cases pass (math, mesh builders only).
3. **G3 behavioral.** `renderer_app` visibly demonstrates the feature's Expected Outcome. Rendering correctness is checked by visual confirmation, not unit tests.
4. **G4 magenta fallback.** Shader creation failure triggers the magenta pipeline without crashing.
5. **G5 interface hygiene.** No edit to `docs/interfaces/renderer-interface-spec.md` without explicit human approval.

---

## 7. Companion files

- `AGENTS.md` — global rules (always loaded).
- `.agents/skills/systems-architect/SKILL.md` — escalate interface/build-topology decisions affecting engine or game.
- `.agents/skills/sokol-api/` — sokol_gfx distilled API + per-aspect references.
- `.agents/skills/sokol-shdc/SKILL.md` — sokol-shdc shader dialect + CMake integration.
- `.agents/skills/glsl-patterns/SKILL.md` — reusable shader patterns (lighting math, skybox, line-quad generation, alpha).
