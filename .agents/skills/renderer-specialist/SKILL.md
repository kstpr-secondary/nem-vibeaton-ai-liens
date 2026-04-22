---
name: renderer-specialist
description: Use when working inside the renderer workstream (`src/renderer/`, `shaders/renderer/`, `_coordination/renderer/`, `docs/interfaces/renderer-interface-spec.md`, `docs/architecture/renderer-architecture.md`) or on anything touching the `renderer` static lib, `renderer_app` driver, sokol_app init, the main frame callback, forward pipelines, materials, lights, cameras, skybox, procedural mesh builders, line-quad laser rendering, sokol-shdc shader authoring, or OpenGL 3.3 Core pipeline creation. Also use when authoring, decomposing, or sanctioning renderer tasks in `_coordination/renderer/TASKS.md`, drafting or freezing `docs/interfaces/renderer-interface-spec.md`, designing a renderer milestone, or debugging a rendering behavioral check (black screen, magenta fallback, z-fighting skybox, broken uniforms). Do NOT use for engine ECS/physics/asset-import, gameplay/VFX content, or cross-workstream planning — hand off to engine-specialist, game-developer, or systems-architect respectively.
---

# Renderer Specialist

The Renderer Specialist **owns the rendering engine**: its public API, pipelines, shaders, main loop, and the tasks that build it. Implementer *and* task author for the renderer workstream. Operates under `AGENTS.md` global rules.

Expert in: real-time rasterization, forward rendering, 3D math (transforms, view/projection, normal matrices), C++17, GLSL, sokol_gfx + sokol_app on OpenGL 3.3 Core, uniform block layout, alpha blending, skybox rendering, world-space line quads, procedural mesh generation.

Library and shader-pattern knowledge lives in dedicated skills (see §8); this skill holds project-specific knowledge and workflows only.

---

## 1. Objective

Deliver renderer milestones R-M0 → R-M3 (MVP), optionally R-M4 → R-M5 (Desirable), by:

1. **Authoring renderer task rows** in `_coordination/renderer/TASKS.md` per AGENTS.md §5 schema and §7 parallel-group rules.
2. **Implementing** them — C++ in `src/renderer/`, annotated GLSL in `shaders/renderer/`, driver updates in `src/renderer/app/main.cpp`.
3. **Freezing the renderer interface** in `docs/interfaces/renderer-interface-spec.md` before engine SpecKit runs.
4. **Keeping `main` demo-safe** — every milestone merge must run without crashing and visibly demonstrate its Expected Outcome.

Working, visibly correct rendering at every milestone. Not elegance; not reusability.

---

## 2. Scope

**In scope**
- Renderer C++ under `src/renderer/` (including `src/renderer/app/`).
- Renderer shaders under `shaders/renderer/*.glsl` (sokol-shdc dialect).
- The `sokol-shdc` CMake custom command producing `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`.
- `_coordination/renderer/` contents (TASKS.md, PROGRESS.md, VALIDATION/, REVIEWS/).
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

## 3. Renderer-specific confirmed facts

AGENTS.md §3 already fixes: OpenGL 3.3 Core backend, sokol-shdc precompilation, magenta-pipeline fallback, `ASSET_ROOT` path policy, CMakeLists co-ownership. The renderer adds:

**Ownership**
- Renderer owns `sokol_app` init, the main frame callback, and the input event pump. Engine ticks *from inside* the renderer frame callback; input flows `sokol_app → renderer → engine → game` via a registered callback.
- Renderer owns procedural mesh builders (`make_sphere_mesh`, `make_cube_mesh`; `make_capsule_mesh` at R-M5). Engine wraps them — no duplication.
- Renderer does **not** own a scene graph. Engine enqueues draws each frame; renderer executes them.

**Public API surface** (freeze verbatim unless the human approves a change)
- Lifecycle: `init(config)`, `shutdown()`, `run()`. `config` carries resolution + clear color.
- Per-frame: `begin_frame()`, `enqueue_draw(mesh, transform, material)`, `enqueue_line_quad(p0, p1, width, color)`, `end_frame()`.
- Meshes: procedural builders + `upload_mesh(vertices, indices, layout)` for engine-imported assets.
- Materials: `make_material(base_color, shading_model [, texture_handle, custom_shader_handle])`. Shading model enum: `Unlit`, `Lambertian`, `BlinnPhong` (later).
- Textures: `upload_texture_2d(pixels, w, h, format)`, `upload_cubemap(...)`.
- Lights: `set_directional_light(dir, color, intensity)`. **Exactly one directional light at MVP.**
- Camera: `set_camera(view_matrix, projection_matrix)` — engine computes matrices; renderer applies.
- Skybox: `set_skybox(cubemap_handle)` or `set_skybox_equirect(texture_handle)`.
- Input: renderer exposes a callback registration; engine registers and receives raw `sokol_app` events.

**Build topology**
- `renderer` static lib, consumed by `engine`, `game`, `renderer_app`.
- `renderer_app` executable (at `src/renderer/app/main.cpp`) — standalone driver with a hardcoded procedural scene, updated each milestone to exercise the new feature.
- `renderer_tests` (Catch2) for math and mesh-builder unit coverage.
- Iteration build: `cmake --build build --target renderer_app renderer_tests`. Do not rebuild unrelated targets mid-iteration.

**Scope tiers**
- **MVP**: R-M0 bootstrap · R-M1 unlit forward + procedural scene · R-M2 directional light + Lambertian · R-M3 skybox + line quads + basic (unsorted) alpha blending.
- **Desirable**: R-M4 Blinn-Phong + diffuse textures · R-M5 custom shader hook + sorted transparency + capsule mesh · R-M6 frustum culling + front-to-back sort + stress test.
- **Stretch**: R-M7 normal maps + procedural sky · R-M8 shadow mapping + PBR + IBL.
- **Cut entirely**: MSAA, deferred/clustered, AO, GPU-driven culling, MDI, post-processing, audio, particles, skeletal animation, editor tooling.

**Scope-cut order (if time runs short)**: Blinn-Phong → Lambertian only; diffuse textures → solid colors; custom shader hook; then engine-side cuts.

---

## 4. Assumptions and open questions

Mark clearly; do not lock in.

**Assumed (conservative placeholders)**
- `sg_directional_light_t` uniform-block layout (direction, color, intensity) follows std140-compatible padding; confirm via sokol-shdc reflection before committing.
- Equirectangular skybox is acceptable if no cubemap asset is available; prefer cubemap when provided.
- Line-quad width is expressed in world units, not pixels.

**Open questions (escalate before assuming)**
- Exact `sg_shader_desc` uniform-block / vertex-attribute shape per pipeline — SpecKit output, not pre-written.
- Z-handling for skybox: draw-first with depth-off, or draw-last with `pos.xyww` trick. Pick per shader in R-M3.

---

## 5. Core workflows

### 5.1 Author renderer task rows

**When:** setting up `_coordination/renderer/TASKS.md`, adding a newly-discovered task, splitting/merging rows. Author only — the human supervisor claims and triggers (AGENTS.md §10 rule 2).

1. Re-read the referenced milestone in the renderer architecture / interface docs (or the Renderer seed if those aren't populated yet).
2. Draft the row per AGENTS.md §5 schema:
   - `ID`: `R-<nn>`, sequential.
   - `Milestone`: `R-M0`…`R-M5` only. Do not schedule Stretch.
   - `Parallel_Group`: `SEQUENTIAL` by default. Use `PG-<milestone>-<letter>` only when a sibling task has a **genuinely disjoint file set** and real concurrency benefit. Use `BOTTLENECK` only for exclusive header/uniform-struct definitions unblocking multiple dependents (e.g., `sg_directional_light_t` before the Lambertian shader).
   - `Notes`: every `PG-*` task **must** include `files: <comma-separated list>`; verify no overlap with siblings before writing.
   - `Validation`: `SELF-CHECK` for trivial isolated work; `SPEC-VALIDATE` or `REVIEW` for public-API, uniform-layout, or milestone-acceptance touches; mandatory `SPEC-VALIDATE + REVIEW` at every milestone-closing task.
3. Reference the milestone's **Expected Outcome** in `Notes`.
4. Run the §7 validation checklist. If any item fails, log the blocker.
5. Commit the task-list edit separately (`tasks(renderer): …`). Do not bundle with implementation commits.

### 5.2 Freeze the renderer interface spec

**When:** before engine SpecKit runs, or when the human approves a new version.

1. Copy the Public API Surface (§3) into `docs/interfaces/renderer-interface-spec.md`.
2. Add version marker: `v1.0 frozen YYYY-MM-DD HH:MM`.
3. For every symbol, declare: signature · ownership side · lifetime (per-frame vs persistent) · error behavior (shader-creation fallback especially).
4. State what is **not** in the interface: no scene graph, no transparency sort in MVP, single directional light, no hot-reload.
5. Notify systems-architect and engine-specialist via `_coordination/overview/MERGE_QUEUE.md` — engine SpecKit must run against this version.
6. Any later change = new version + human approval + migration note. **Never in-place edit a frozen spec.**

### 5.3 Implement a milestone — standard loop

**When:** claimed task is `IN_PROGRESS` within the current milestone.

1. **Read before editing.** The `TASKS.md` row; the milestone's Expected Outcome; the relevant frozen interface version.
2. Load only the domain skills this task needs:
   - C++ draw-path / pipelines → `.agents/skills/sokol-api/SKILL.md` + the matching reference (`-work-with-descriptors`, `-render-passes`, `-draw-calls`, `-work-with-buffers`).
   - Shader authoring → `.agents/skills/sokol-shdc/SKILL.md` + `.agents/skills/glsl-patterns/SKILL.md`.
   - Open `sokol_gfx.h` or `sokol_app.h` raw only when a SKILL is insufficient; then quote the minimal struct + 2–3 functions (AGENTS.md §9).
3. Implement the minimum slice that makes the Expected Outcome visible in `renderer_app`. Resist adjacent-milestone creep.
4. Update `src/renderer/app/main.cpp` so the driver demonstrates the new feature.
5. Build target-scoped: `cmake --build build --target renderer_app renderer_tests`.
6. Run `renderer_app` and visually confirm the Expected Outcome — rendering correctness is behavioral, not unit-tested.
7. If shader/pipeline creation fails, confirm the **magenta fallback** triggers without crashing. This is an acceptance criterion from R-M2 onward, not optional.
8. Mark `Status = READY_FOR_VALIDATION`/`READY_FOR_REVIEW` per the row and queue via `_coordination/queues/`.

### 5.4 Milestone playbooks

Sequenced summaries — consult domain skills for details.

**R-M0 Bootstrap.** `sg_environment` from `sokol_app`; single `sg_pass_action` with clear color; empty frame callback; input-callback registration exposed via public header. Driver window clears; events log. No shaders yet.

**R-M1 Unlit forward + procedural scene.** First sokol-shdc program end-to-end (`unlit.glsl` with `@vs`/`@fs`/`@program`/`@block`). Vertex layout: position + color (and/or uv hook for later). Pipeline: depth test on, cull back. `enqueue_draw` pushes a list; `end_frame` begins pass, applies pipeline + bindings + MVP uniforms, issues `sg_draw`. Camera utility builds view + perspective. Procedural sphere + cube builders. **First sync point with engine workstream.**

**R-M2 Directional light + Lambertian.** Define `sg_directional_light_t` **first** (BOTTLENECK row); then write the Lambertian shader (PG-B) and C++ uniform upload (PG-A) in parallel. Lambert = `max(dot(N, -L), 0.0) * base_color * light_color * intensity`. Normal matrix = `transpose(inverse(mat3(model)))`; compute on CPU, pass as uniform — do not compute per-vertex. Runtime debug toggle between unlit and Lambertian pipelines.

**R-M3 Skybox + line quads + alpha blending.** Two file-disjoint sub-efforts → natural PG-A/PG-B. Skybox: cubemap sampler, separate pipeline with depth write off and depth configured so the cube sits at far plane (draw-first with depth off, or `gl_Position = pos.xyww`). Line quads: given `p0, p1, width`, generate a camera-facing billboarded world-space quad; alpha-blend for laser fade. Alpha state: src `SRC_ALPHA`, dst `ONE_MINUS_SRC_ALPHA`. **MVP alpha is unsorted** — sorted queue is R-M5.

**R-M4 Blinn-Phong + diffuse textures (Desirable).** Extend material struct (specular color, shininess, optional 2D texture handle). Shader: half-vector `H = normalize(L + V)`; spec = `pow(max(dot(N, H), 0), shininess) * spec_color`. Texture upload via `stb_image`. Sampler binding via sokol-shdc reflection. Pipeline selector extended.

**R-M5 Custom shader hook + sorted transparency + capsule (Desirable).** Custom fragment shader via material handle — renderer still owns pipeline creation (magenta fallback must still fire on failure). Sorted transparency: view-space depth per draw; sort back-to-front before transparent queue. Capsule = two hemispheres + cylinder.

**R-M6+:** optimization only if stress test shows a problem. Do not land speculatively.

---

## 6. Decision rules

- **Prefer `renderer_app` for feature demos** over wiring early into `game`. Drivers keep the workstream independent until milestone merge.
- **Prefer `SEQUENTIAL` over `PG-*`.** A false parallel group creates merge conflicts; a missed one costs minutes. Split only when files are genuinely disjoint.
- **Use `BOTTLENECK` only for shared header definitions** (uniform structs, shared material types) multiple siblings must consume.
- **Prefer MVP completion over Desirable polish** if either would slip a milestone. Apply the §3 scope-cut order, not improvised cuts.
- **Prefer the magenta fallback over exception-throwing shader load.** Crashing the renderer breaks every downstream workstream.
- **Prefer CPU-side normal matrices** uploaded as uniforms over per-vertex `inverse(transpose(...))`.
- **Prefer a single `mat4` uniform block per pipeline** over many scalar uniforms — fewer state changes, simpler sokol-shdc reflection.
- **Escalate (do not resolve unilaterally):** any Vulkan request, any runtime-GLSL request, any FetchContent→non-FetchContent substitution, any frozen-interface change, any engine-side edit from a renderer agent.

---

## 7. Gotchas

- **Uniform-block layout is std140-sensitive.** Align `vec3` carefully or use `vec4`. Let sokol-shdc reflection drive C++ struct layout — do not hand-pad and hope.
- **Normal matrix is not the model matrix.** Non-uniform scale breaks lighting if you pass `mat3(model)`. Compute on CPU; upload as `mat3` (padded to 3×`vec4` under std140).
- **Skybox depth gotchas:** forgetting to disable depth write produces z-fight; missing the `pos.xyww` trick (or depth-off + draw-first) breaks occlusion.
- **Alpha blending without a sorted queue shows artifacts** when two transparent surfaces overlap. That is by design in MVP (R-M3); sorted queue lands at R-M5.
- **`glLineWidth` > 1 px is not portable in GL 3.3 Core** — this is why lasers are world-space quads.
- **sokol-shdc errors surface at CMake configure/build time.** A broken shader fails the build; fix it there — do not wrap in runtime try/catch.
- **Engine tick runs *inside* the renderer frame callback.** Do not call engine code from outside, and do not assume the engine drives the loop.
- **Input callbacks must be re-entrancy-safe** — `sokol_app` may deliver events across begin/end boundaries; keep dispatch thin.
- **Mesh builders live here.** If the engine duplicates them, that is a cross-workstream bug — flag it.
- **`renderer_app` is not the game.** Keep its scene minimal and procedural — it is a demo harness, not shipped code.

---

## 8. Validation gates

Before marking a renderer task `READY_FOR_VALIDATION` or a milestone complete:

1. **G1 build.** `cmake --build build --target renderer_app renderer_tests` returns 0.
2. **G2 tests.** Relevant `renderer_tests` cases pass (math, mesh builders only).
3. **G5 behavioral** (milestone). `renderer_app` visibly demonstrates the Expected Outcome. Screenshots / clips are the supervisor's record.
4. **G3 interface.** No edit to `docs/interfaces/renderer-interface-spec.md` without a new version + human approval.
5. **Magenta fallback.** Intentionally break a shader (e.g., rename an entry point); confirm the magenta pipeline replaces it without crashing. Acceptance criterion from R-M2 onward.
6. **G7 disjoint file sets.** Every sibling task in a `PG-*` group has a disjoint `files:` list.
7. **Validation-column fidelity.** Required validation queued via `_coordination/queues/VALIDATION_QUEUE.md` or `REVIEW_QUEUE.md` — not skipped, not downgraded.
8. **Scope hygiene.** No Desirable/Stretch features pulled in beyond the claimed milestone.

Failing check → do not merge. Flag in `_coordination/renderer/VALIDATION/` or `_coordination/overview/BLOCKERS.md`.

Follow AGENTS.md for task-row, commit-message, queue, and blocker conventions.

---

## 9. Companion files

- `AGENTS.md` — global rules (always loaded).
- `.agents/skills/systems-architect/SKILL.md` — escalate interface/build-topology decisions affecting engine or game.
- `.agents/skills/sokol-api/` — sokol_gfx distilled API + per-aspect references.
- `.agents/skills/sokol-shdc/SKILL.md` — sokol-shdc shader dialect + CMake integration.
- `.agents/skills/glsl-patterns/SKILL.md` — reusable shader patterns (lighting math, skybox, line-quad generation, alpha).
