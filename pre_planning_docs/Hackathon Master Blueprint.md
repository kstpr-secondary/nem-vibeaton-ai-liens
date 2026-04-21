# Vibe Coding Hackathon — Master Blueprint (Iteration 8)

> Authoritative reference for human team members and AI agents. Covers project context, agentic setup, workflow, source-control protocol, and pre-hackathon preparation. Technical specs for the rendering engine, game engine, and game live in separate documents.
> **Status:** Living document. Decisions from iterations 1–7 are **FIXED**; new decisions are **FIXED (Iter 8)**.

---

## 1. Project Concept

### 1.1 What We Are Building

Three interconnected C++ projects built from scratch in a 5–6 hour hackathon:

1. **Rendering engine** — forward rendering, physically plausible shading, skybox, textures, line rendering. Built on `sokol_gfx`.
2. **Game engine** — ECS scene management, game loop, physics, input, asset loading. Built on the rendering engine's public API.
3. **3D space shooter** — asteroids, spaceship, enemy AI, weapons, shields/HP, containment field. Built on the game engine's public API.

All three live in one repo in separate directories, integrated at milestone boundaries. Each workstream begins against frozen interface contracts and mocks.

### 1.2 Goals

Live demo where every line of C++ is AI-generated under human supervision. **Speed over long-term maintainability.** Behavioral correctness, milestone predictability, and integration discipline take priority over elegance or extensibility.

### 1.3 The Human Team

Three expert Unity 3D / C# / 3D math programmers with strong shader knowledge but rusty C++ syntax.

- **Person A** — Rendering Engine
- **Person B** — Game Engine
- **Person C** — Game

Humans supervise, approve interface changes, run behavioral checks, and manage git sync at milestones. Manual coding is an emergency-only path.

---

## 2. Hardware and Platform

**FIXED:** Ubuntu-only — three Ubuntu 24.04 LTS laptops + one Ubuntu 24.04 LTS RTX 3090 desktop.

| Machine | Owner | Primary Role |
| :-- | :-- | :-- |
| Laptop A | Person A | Rendering engine |
| Laptop B | Person B | Game engine |
| Laptop C | Person C | Game |
| RTX 3090 PC | Shared | Validation, tests, local model, primary demo machine |

**FIXED:** Build stack: **CMake + Ninja + Clang**. Graphics target: **desktop Linux**. Rendering abstraction: `sokol_gfx`.

The RTX 3090 handles bounded tasks (review, test-gen, validation, overflow) and must not sit on the critical path for routine implementation.

---

## 3. Technology Stack

All items **FIXED** unless noted.

### 3.1 Core Libraries

| Library | Role | Integration | Version |
| :-- | :-- | :-- | :-- |
| **sokol_gfx** | Graphics abstraction | FetchContent | pinned |
| **sokol_app** | Window + event loop | FetchContent | same pin |
| **sokol_time** | High-res timer | FetchContent | same pin |
| **entt** | ECS | FetchContent | v3.x pinned |
| **cgltf** | glTF loading | FetchContent | pinned |
| **tinyobjloader** | OBJ fallback | FetchContent | pinned |
| **GLM** | 3D math | FetchContent | 1.x pinned |
| **Catch2 v3** | Unit testing | FetchContent | v3.x pinned |
| **Dear ImGui** | HUD / debug overlay | FetchContent | pinned |

### 3.2 Language and Build

- **C++17**, **CMake + Ninja**
- Warnings: `-Wall -Wextra -Wpedantic`; `-Werror` disabled. Sokol headers wrapped with `-w` or suppression header.
- **FetchContent only** — no Conan or vcpkg.
- Explicit cuts: no audio, networking, skeletal animation, particles, editor tooling.

### 3.3 Shader Pipeline and Graphics Backend

**FIXED:** Primary backend: **Vulkan** on desktop Linux. Fallback: **OpenGL 3.3 Core** via build flag.

**FIXED:** Rendering engine owns `sokol_app` init and main frame callback. Game engine runs its tick from inside the renderer's frame callback. Input flows from `sokol_app` through renderer to engine.

**FIXED (Iter 9, revised):** Shaders are annotated `.glsl` files under `/shaders/renderer/` or `/shaders/game/`, **precompiled ahead of time via `sokol-shdc`** into per-backend headers (`${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`). Consumed via `#include` + `shd_<name>_shader_desc(sg_query_backend())`. Reverses the earlier "runtime file loading is default" decision.

**Rationale:** `sokol_gfx`'s Vulkan backend requires SPIR-V bytecode plus reflection descriptors (uniform block layout, vertex attributes, sampler bindings) on `sg_shader_desc`. A runtime GLSL path requires glslang/shaderc + SPIRV-Cross integration and dual GL/Vulkan code paths — estimated 2–4h of plumbing with no visible output. `sokol-shdc` is the intended sokol path and gives backend-portable bytecode + reflection for free. Build-time errors replace runtime errors; a separate `validate_shaders` target is no longer needed (the sokol-shdc CMake custom command is the validation).

**FIXED (Iter 9):** Shader author format follows sokol-shdc annotations (`@vs`, `@fs`, `@module`, `@program`, `@block`). Dedicated `.agents/skills/sokol-shdc/SKILL.md` covers the dialect for agents.

**FIXED (Iter 9):** Runtime `sg_make_shader` / pipeline-creation callback logs errors and falls back to a magenta error pipeline — never crashes. Acceptance criterion on the shading milestone.

**Cut:** Runtime GLSL loading; shader hot-reload. Hot-reload may be added later as a debug-only GL-backend path if time permits.

### 3.4 Assets

Pre-verified for integrity, format, and licensing. Prefer **glTF/GLB**; OBJ as fallback. Max two mesh formats.

### 3.5 Build Topology and Per-Workstream Commands

**FIXED (Iter 9):** CMake targets. Renderer and engine are consumed as static libs by downstream workstreams **and** have standalone driver executables so each workstream can iterate on procedural scenes without building the game.

| Target | Kind | Source | Links | Purpose |
| :-- | :-- | :-- | :-- | :-- |
| `renderer` | static lib | `src/renderer/` | sokol, glm | Linked by `engine`, `game`, drivers |
| `renderer_app` | executable | `src/renderer/app/` | `renderer` | Standalone renderer driver — procedural scene, no engine/game |
| `engine` | static lib | `src/engine/` | `renderer`, entt, cgltf | Linked by `game`, `engine_app` |
| `engine_app` | executable | `src/engine/app/` | `engine`, `renderer` | Standalone engine driver — procedural ECS scene, no game |
| `game` | executable | `src/game/` | `engine`, `renderer` | Full game |
| `renderer_tests` | executable | Catch2 | `renderer` | Unit tests |
| `engine_tests` | executable | Catch2 | `engine` | Unit tests |

Driver `main.cpp` files own `sokol_app` entry, build a hardcoded procedural scene against the workstream's public API, and stay small (demo/iteration harness only — not shipped with the game).

One-time configure (repo root):
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Per-workstream iteration builds — **do not rebuild unrelated targets**:
```bash
cmake --build build --target renderer_app renderer_tests   # renderer workstream
cmake --build build --target engine_app engine_tests       # engine workstream
cmake --build build --target game                          # game workstream
cmake --build build                                        # full milestone-sync
```

Rule: after a milestone merge the downstream workstream runs the full build once to refresh upstream static libs; routine iteration stays target-scoped. Ninja + static libs yield correct incremental behavior with no extra caching infra.

### 3.6 Asset and Shader Path Resolution

**FIXED (Iter 9):** Absolute paths baked at configure time — no cwd dependency. `cmake/paths.h.in` → `${CMAKE_BINARY_DIR}/generated/paths.h`, included by `renderer` (PUBLIC):

```c
#define PROJECT_SOURCE_ROOT "@CMAKE_SOURCE_DIR@"
#define ASSET_ROOT          PROJECT_SOURCE_ROOT "/assets"
```

Shaders need no runtime path macro — they are compiled into headers at build time (§3.3). Only runtime-loaded content (meshes, textures) uses `ASSET_ROOT`.

Rule (add to `AGENTS.md`): never hard-code relative asset paths; always compose from `ASSET_ROOT`. Trade-off: binary is not relocatable across machines — acceptable for hackathon demo; add a `--asset-root` CLI override only if relocation becomes necessary.

---

## 4. Rendering Engine — Scope

> Full details in Rendering Engine Spec.

**MVP:** perspective camera, forward rendering, unlit→Lambertian→Blinn-Phong, diffuse textures, static skybox, line renderer, simple procedural geometry, runtime GLSL loading, minimal shader-error reporting.
**Desirable:** alpha blending queues, normal maps, GPU instancing, material hooks.
**Stretch/Cut:** shadow mapping, PBR, deferred rendering, post-processing.

---

## 5. Game Engine — Scope

> Full details in Game Engine Spec.

**MVP:** ECS via `entt`, asset/mesh upload bridge, game loop synced with renderer, input, Euler physics, simple collision, raycasting, thin gameplay API.
**Desirable:** enemy steering, multiple lights, prefab spawning helpers.
**Stretch/Cut:** pathfinding, animation, audio, networking, editor.

---

## 6. Game — Scope

> Full details in `GAME_DESIGN.md`.

**MVP:** procedural asteroid field, player spaceship, enemy ships, plasma gun, laser beam, shields/HP, containment boundary, Dear ImGui HUD.
**Stretch/Cut:** rocket launcher, power-ups, advanced VFX, mobile controls, station regen.

Game workstream starts from frozen interfaces + mocks; must not block on full engine delivery.

---

## 7. AI Agent Stack

### 7.1 Agent Roles

| Role | SKILL.md | Activated By | Responsibility |
| :-- | :-- | :-- | :-- |
| Systems Architect | `systems-architect/SKILL.md` | Human explicitly | Architecture, cross-workstream plans |
| Renderer Specialist | `renderer-specialist/SKILL.md` | Renderer worktree session | Rendering engine implementation |
| Engine Specialist | `engine-specialist/SKILL.md` | Engine worktree session | Game engine implementation |
| Game Developer | `game-developer/SKILL.md` | Game worktree session | Game implementation |
| Test Author | `test-author/SKILL.md` | Human or queue | Catch2 tests, smoke tests |
| Spec Validator | `spec-validator/SKILL.md` | Human or queue | Spec adherence checks ("Did this implement the right thing?") |
| Code Reviewer | `code-reviewer/SKILL.md` | Human on diff/PR | Risk and bug review ("Is this obviously risky or broken?") |

### 7.2 AI Tool Priority

**FIXED:**
1. Claude Code Sonnet — primary for hard/ambiguous tasks
2. GitHub Copilot agent mode/CLI — primary backup and medium-task worker
3. Gemini CLI — validation, review, research, overflow
4. z.ai GLM 5.1 — backup for Claude-heavy work; parallel hard-task executor in second half
5. Local Qwen3.6-35B-A3B on RTX 3090 — tests, spec checks, shader review, small-file analysis. Larger/more capable than the previously planned GLM-4.7-Flash; still treated as bounded-context per §7.3.
6. Claude Opus — escalation-only for thorny math/architecture

**Trigger:** If agent stalls >90 seconds, escalate immediately and log in `BLOCKERS.md`.

### 7.3 Local Agent (RTX 3090) Limits

Use for: tests for one file, one shader/diff review, one task checklist with trimmed prompt, small interface checks. Treat as **small-context, bounded-output** even if configured for 64K context; prompts must stay narrow.

**Worktree strategy:** RTX 3090 keeps one clone + worktrees mirroring laptops, targeting a specific `worktree/branch` per task. Mostly inspects committed/saved state, not live edits.

---

## 8. Instruction Files, Skills, and Header Handling

### 8.1 Design Principles

1. Cross-tool compatibility via a single root behavior file.
2. Concise root — protect context budgets.
3. Progressive disclosure — specialized knowledge loaded on demand.
4. Stable locations — rules, specs, tasks, validation, and skills always findable.

### 8.2 Canonical Root Files

**FIXED:** `AGENTS.md` is the cross-tool root behavior file.

- `AGENTS.md`: global rules (task claiming, milestones, validation, interface changes, queue usage).
- `CLAUDE.md`: imports `AGENTS.md` via `@AGENTS.md`; adds only Claude-specific notes.
- `.github/copilot-instructions.md`: short mirror of critical rules for Copilot.
- `.gemini/settings.json`: `"contextFileName": "AGENTS.md"`.

Workstream-specific and library-specific content goes into SKILL files, not `AGENTS.md`.

### 8.3 Repository Structure

```text
<repo-root>/
├── AGENTS.md
├── CLAUDE.md
├── .gemini/settings.json
├── .github/copilot-instructions.md
├── .agents/skills/
│   ├── systems-architect/SKILL.md
│   ├── renderer-specialist/SKILL.md
│   ├── engine-specialist/SKILL.md
│   ├── game-developer/SKILL.md
│   ├── test-author/SKILL.md
│   ├── spec-validator/SKILL.md
│   ├── code-reviewer/SKILL.md
│   ├── sokol-api/
│   │   ├── SKILL.md
│   │   └── references/
│   │       ├── sokol-api-work-with-descriptors.md
│   │       ├── sokol-api-work-with-buffers.md
│   │       ├── sokol-api-render-passes.md
│   │       ├── sokol-api-draw-calls.md
│   │       └── sokol-api-compute-dispatch.md
│   ├── entt-ecs/
│   │   ├── SKILL.md
│   │   └── references/
│   │       ├── entt-registry-basics.md
│   │       ├── entt-component-views.md
│   │       └── entt-entity-lifecycle.md
│   ├── cgltf-loading/
│   │   ├── SKILL.md
│   │   └── references/
│   │       └── cgltf-mesh-extraction.md
│   ├── glsl-patterns/SKILL.md
│   └── physics-euler/SKILL.md
├── docs/
│   ├── architecture/
│   │   ├── ARCHITECTURE.md
│   │   ├── renderer-architecture.md
│   │   ├── engine-architecture.md
│   │   └── game-architecture.md
│   ├── interfaces/
│   │   ├── INTERFACE_SPEC.md
│   │   ├── renderer-interface-spec.md
│   │   ├── engine-interface-spec.md
│   │   └── game-interface-spec.md
│   ├── game-design/
│   │   ├── GAME_DESIGN.md
│   │   └── game-workstream-design.md
│   └── planning/speckit/{renderer,engine,game}/
├── _coordination/
│   ├── overview/
│   │   ├── PROGRESS.md
│   │   ├── BLOCKERS.md
│   │   ├── MERGE_QUEUE.md
│   │   └── MASTER_TASKS.md
│   ├── {renderer,engine,game}/
│   │   ├── TASKS.md
│   │   ├── PROGRESS.md
│   │   ├── VALIDATION/
│   │   └── REVIEWS/
│   └── queues/
│       ├── VALIDATION_QUEUE.md
│       ├── REVIEW_QUEUE.md
│       └── TEST_QUEUE.md
├── shaders/
│   ├── renderer/{lambertian.frag.glsl, phong.frag.glsl, skybox.frag.glsl}
│   └── game/{explosion.frag.glsl, plasma.frag.glsl}
└── src/{renderer,engine,game}/
```

- `docs/` — stable reference (architecture, interfaces, design, planning).
- `_coordination/` — live operational state (tasks, progress, blockers, queue, validation/review).
- `.agents/skills/` — reusable skills and distilled APIs.

### 8.4 Coordination Files and Task Schema

**FIXED (Iter 8):** Workstream `TASKS.md` files are authoritative on their feature branches. Cross-workstream progress is visible only after milestone merge to `main` + `git pull`. `MASTER_TASKS.md` is the human supervisor's integration dashboard.

Task row schema:
```
| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
```

`Validation`: `NONE` | `SELF-CHECK` | `SPEC-VALIDATE` | `REVIEW` | `SPEC-VALIDATE + REVIEW`

`Parallel_Group`: see §10.

### 8.5 AGENTS.md Rules

1. Before editing code, read the workstream `TASKS.md` and the spec referenced by that task.
2. Task claiming: human supervisor updates `TASKS.md`, commits, and pushes before triggering an agent.
3. Do not change frozen shared interfaces without explicit human approval.
4. Every implementation task must reference its milestone unlock or dependency.
5. Respect the `Validation` column; do not silently downgrade required checks.
6. Milestone-ready work is not merged until: (a) acceptance checklist met, (b) required validation complete, (c) human behavioral check done.
7. Review and validation are triggered via queue files, not hidden hooks.
8. Pull before starting new work when multiple people/machines touch the same workstream.
9. When working with a library that has a dedicated SKILL, load and follow the SKILL first; open headers only when the SKILL is insufficient.
10. `Owner` = `<agent_name>@<machine>` (e.g., `claude@laptopA`). If machine name is unknown, add a note in `Notes` and do not overwrite a populated owner.
11. **CMakeLists.txt** is owned by Renderer workstream / Systems Architect. Cross-workstream build changes require 2-minute notice.
12. **Agent outage:** Stall >90 seconds → switch to next priority tier; log in `BLOCKERS.md`.
13. **Emergency manual path:** Agent fails a task twice or is >30 min behind milestone → switch to `FirstName@machine`.
14. **SKILL drift:** If a SKILL contains an error or missing API, human supervisor fixes and pushes immediately so all agents benefit.
15. **Milestone validation:** Every milestone merge requires behavioral check by a different person from the implementer if possible; otherwise human supervisor verifies each acceptance criterion on demo machine.

### 8.6 Validation Granularity

- **Low/trivial:** self-check unless touching shared interfaces, build system, or milestone-critical behavior.
- **Medium:** spec validation or review for nontrivial logic or integration surfaces.
- **High/hard:** at least one secondary check before merge.
- **Every milestone:** spec validator + human behavioral check + lightweight review.
- **Testing:** Catch2 for math, parsers, ECS logic. Rendering correctness verified via human behavioral check + smoke-test visuals, not unit tests.

### 8.7 Architecture and Spec Documents

- `docs/architecture/ARCHITECTURE.md`: integrated cross-workstream view.
- `{renderer,engine,game}-architecture.md`: workstream-local architecture.
- `docs/interfaces/INTERFACE_SPEC.md`: shared contracts.
- Agents prefer local architecture/spec files + relevant excerpt from integrated docs.

### 8.8 SpecKit Planning — Per Workstream

**FIXED (Iter 8):** SpecKit runs **once per workstream**, sequentially, to prevent scope explosion and keep each plan focused:

1. **Renderer SpecKit:** produces tasks, architecture, and `renderer-interface-spec.md`.
2. **Engine SpecKit:** uses frozen renderer interfaces; produces engine tasks and interface contracts.
3. **Game SpecKit:** uses frozen renderer + engine interfaces; produces game tasks.

Systems Architect then runs a cross-workstream synthesis pass → `MASTER_TASKS.md`. Tasks touching a frozen interface must set `Depends_on` to the relevant interface spec version.

SpecKit outputs stored under `docs/planning/speckit/{renderer,engine,game}/`.

### 8.9 SpecKit Output Mapping

| SpecKit artifact | Stable schema location |
| :-- | :-- |
| `plan.md` / `research.md` | `docs/architecture/` or `docs/planning/speckit/<workstream>/` |
| `contracts/` | `docs/interfaces/INTERFACE_SPEC.md` + workstream-local interface specs |
| `tasks.md` | `_coordination/.../TASKS.md` + compact `MASTER_TASKS.md` |
| `quickstart.md` | **Mandatory** runbook covering build/run commands and environment |

SpecKit layout is not used for live coordination; translated once, then read-only.

### 8.10 Skills and Large-Header Libraries

**Problem:** Libraries like `sokol_gfx.h`, `entt.hpp`, `cgltf.h` are 20–30k LOC — loading them naively wastes context.

**Strategy: Skill + per-aspect reference files**

**Library SKILL (`SKILL.md`):** Short description, project-specific usage rules, distilled API cheat-sheet (key public types/functions needed for this project only, each with short description). No full header. Strict size budget. References aspect files in a "Deeper references" section.

**FIXED (Iter 8) — Per-aspect reference files (`.agents/skills/<lib>-api/references/<lib>-<aspect>.md`):** Lives under `.agents/skills/<lib>-api/references/`, **not** `docs/interfaces/` (these are agent-consumption artifacts, not project interface contracts). Each file covers one narrow concern, contains: a "when to use" header paragraph, minimal relevant API signatures, and 1–2 usage patterns. No duplication with `SKILL.md`. Example breakdown for `sokol-api`:
- `sokol-api-work-with-descriptors.md` — `sg_buffer_desc`, `sg_pipeline_desc`, etc.
- `sokol-api-work-with-buffers.md` — vertex/index buffer lifecycle
- `sokol-api-render-passes.md` — `sg_begin_pass`, `sg_end_pass`, attachments, `sg_pass_action`
- `sokol-api-draw-calls.md` — `sg_apply_pipeline`, `sg_apply_bindings`, `sg_apply_uniforms`, `sg_draw`
- `sokol-api-compute-dispatch.md` — compute pipeline setup/dispatch (stretch)

**Usage policy:** Use SKILL first, then the relevant aspect reference. Only open the actual header when the task requires functionality not covered by any aspect reference, or an error references an unknown symbol — then quote only the minimal necessary portion (relevant struct + 2–3 functions).

**Pre-hackathon:** Systems Architect generates distilled SKILLs and aspect references for `sokol_gfx`, `entt`, `cgltf`, and other heavy headers. Human-reviewed and kept under token budget.

### 8.11 Lazy Skill Loading

When starting a task:
1. Read the relevant `TASKS.md` row.
2. Load the role SKILL.
3. Load only the library/domain SKILLs needed for subsystems in this task.
4. Load per-aspect reference files from `.agents/skills/<lib>/references/` only when the task clearly falls in that aspect.

### 8.12 Task Claiming and Agent ID

**FIXED (Iter 8):** `Owner` = `<agent_name>@<machine>` (not `person@machine`), so multiple agents on the same machine can be distinguished.

- `agent_name`: lowercase tool name — `claude`, `gemini`, `copilot`, `glm`, `local-qwen`.
- `machine`: short agreed hostname (see table).
- Two instances of same agent: append suffix — `claude-2@laptopA`.
- Human manual path: `FirstName@machine` (e.g., `Alice@laptopA`).
- Agent unable to determine machine: do not overwrite a populated `Owner`; add `"assisted by <agent_name>, machine unknown"` to `Notes`.

**FIXED (Iter 8):** Machine hostnames set once pre-hackathon, recorded in `AGENTS.md`, and set in `/etc/hostname`:

| Hostname | Owner | Primary Role |
| :-- | :-- | :-- |
| `laptopA` | Person A | Rendering engine |
| `laptopB` | Person B | Game engine |
| `laptopC` | Person C | Game |
| `rtx3090` | Shared | Validation, tests, local model |

---

## 9. Source Control

### 9.1 Branches and Worktrees

Branches:
```
main  /  integration  /  feature/renderer  /  feature/engine  /  feature/game
```

Worktrees per machine:
```
hackathon/  hackathon-renderer/  hackathon-engine/  hackathon-game/
```

Multiple agents may work in the same worktree/branch simultaneously if their file sets are **disjoint** (§10.3). `main` stays demo-safe; `integration` is optional.

### 9.2 Milestone Merge Protocol

1. All milestone tasks marked done.
2. Build passes; smoke tests pass.
3. Spec Validator confirms acceptance criteria.
4. Human behavioral check.
5. Code Reviewer lightweight review.
6. Human merges feature branch → `integration` or `main`.
7. Other workstreams fetch and sync.
8. Mocks swapped for real implementations where applicable.

---

## 10. Task and Milestone Structure for Parallel Work

**FIXED (Iter 8).** Supersedes "one cpp file/shader per task" (iter 6) and flat milestone structure (earlier iterations).

### 10.1 Two-Level Structure

- **Milestone Group:** thematic cluster of related milestones (e.g., "Shading", "Scene Management"). Organizational label only — no mandatory integration event.
- **Milestone:** formal, tested sync point. All tasks complete + acceptance criteria met before merge. Carries an `Expected Outcome` statement.
- **Parallel Group (`PG-<milestone_id>-<letter>`):** tasks within a milestone with non-overlapping file sets that can run concurrently by different agents.
- **Bottleneck Task:** must complete exclusively before dependent milestone/parallel group tasks start. Marked `BOTTLENECK`.

### 10.2 Task Schema

```
| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
```

`Parallel_Group` values:
- `PG-M3-A` (etc.) — may run concurrently with tasks in same group
- `BOTTLENECK` — exclusive gate; must complete and be merged before dependents start
- `SEQUENTIAL` — runs after `Depends_on` tasks; no concurrency constraint with tasks outside its chain (default)

### 10.3 File-Ownership Rule for Parallel Groups

**FIXED (Iter 8):** Tasks in the same parallel group must have **disjoint file sets**, declared in `Notes`: `files: renderer.cpp, renderer.h`.

If an agent discovers it must touch a file outside its declared set:
1. Pause.
2. Update the file set in `TASKS.md`.
3. Check whether the new file is claimed by another task in the same parallel group.
4. If yes: flag in `BLOCKERS.md` and wait for human resolution.

### 10.4 Bottleneck Idling Policy

When BOTTLENECK blocks others: idle agents pull work from other milestones or workstreams. Secondary tasks: write tests for completed work, add validation queue entries, update docs, prepare aspect reference files. Use `BOTTLENECK` sparingly.

### 10.5 Worked Example — Renderer Core Milestones

> For process illustration only; actual tasks are created during SpecKit planning.

#### Milestone M2 — Frustum Culling
Bottleneck: NO | PG: PG-M2-A (C++ impl), PG-M2-B (independent utility)

| ID | Task | Tier | PG | files |
|---|---|---|---|---|
| R-10 | Compute frustum planes from camera/projection matrix | MED | PG-M2-A | frustum.cpp, frustum.h |
| R-11 | AABB vs frustum test for all scene objects | MED | SEQUENTIAL | frustum.cpp (depends R-10) |
| R-12 | Filter draw list to culled objects only | MED | SEQUENTIAL | renderer.cpp (depends R-11) |
| R-12b | Frustum visualization helper (debug) | LOW | PG-M2-B | debug_draw.cpp, debug_draw.h |

Expected outcome: Disabling culling noticeably drops FPS; enabling it recovers it.
Integration event: merge feature/renderer → integration; downstream workstreams sync.

#### Milestone M3 — Directional Light and Lambertian Shading
Bottleneck: R-13 | PG: PG-M3-A (cpp), PG-M3-B (shader, after R-13) | Depends on: M2

| ID | Task | Tier | PG | files |
|---|---|---|---|---|
| R-13 | Define `sg_directional_light_t` uniform struct; add to header | HIGH | BOTTLENECK | renderer.h |
| R-14 | Add light uniform block upload in frame loop | MED | PG-M3-A | renderer.cpp |
| R-15 | Define Lambertian material param struct | MED | PG-M3-A | material.h, material.cpp |
| R-16 | Write Lambertian fragment shader | MED | PG-M3-B | shaders/renderer/lambertian.frag.glsl |
| R-17 | Wire shader + material + uniform in pipeline creation | MED | SEQUENTIAL | renderer.cpp (after R-14..R-16) |

Expected outcome: Scene lit from directional source with Lambertian diffuse shading.
Integration event: merge → integration; game engine may swap mock lighting.

#### Milestone M4 — Blinn-Phong Shading
Bottleneck: NO | PG: PG-M4-A (material/cpp), PG-M4-B (shader) | Depends on: M3

| ID | Task | Tier | PG | files |
|---|---|---|---|---|
| R-18 | Extend material struct for Blinn-Phong (specular) | MED | PG-M4-A | material.h, material.cpp |
| R-19 | Write Blinn-Phong fragment shader | MED | PG-M4-B | shaders/renderer/phong.frag.glsl |
| R-20 | Add toggle between Lambertian and Blinn-Phong pipelines | MED | SEQUENTIAL | renderer.cpp (after R-18, R-19) |

Expected outcome: Shading toggleable at runtime; specular highlights visible.

#### Milestone M5 — Skybox
Bottleneck: NO | PG: PG-M5-A | Note: M4 and M5 can proceed in parallel once R-13 is cleared (disjoint file sets).

| ID | Task | Tier | PG | files |
|---|---|---|---|---|
| R-21 | Load equirectangular or cubemap texture | MED | PG-M5-A | skybox.cpp, skybox.h |
| R-22 | Write skybox vertex + fragment shaders | MED | PG-M5-A | shaders/renderer/skybox.vert.glsl, skybox.frag.glsl |
| R-23 | Integrate skybox pass into frame loop | MED | SEQUENTIAL | renderer.cpp (after R-21, R-22) |

Expected outcome: Skybox rendered behind all geometry; no z-fight or depth artifacts.

### 10.6 Milestone Integration Cadence

Target: **~1 milestone merge per hour** per workstream (~5 total per workstream).

- Intra-workstream milestones: lightweight protocol (build passes, smoke test, human spot-check, push).
- Interface-touching milestones: full sync protocol (§9.2) including downstream sync and mock-to-real swap.

---

## 11. Hackathon Workflow

### 11.1 Pre-Hackathon Tasks

- Environment setup on all machines; agreed hostnames set in `/etc/hostname`.
- Repo and worktree creation.
- **Sequential SpecKit Planning:** Renderer → Engine → Game (three separate runs).
- Systems Architect synthesis pass → `MASTER_TASKS.md`.
- Human review and freeze: interfaces, milestones, queue formats.
- `GAME_DESIGN.md` authored and frozen.
- Asset manifest prepared and integrity-checked.
- Local model validated on RTX 3090.
- Copilot and Gemini context-loading verified.
- Per-aspect library reference files generated and reviewed.
- Dependencies pre-downloaded on all machines (FetchContent cache warmed).
- Asset and shader runtime path policy decided.
- `quickstart.md` / runbook created and tested.
- Shared API header and mock generation rehearsed.
- `sokol-shdc` binary installed on all machines; CMake custom command wired for one reference shader end-to-end.

### 11.2 Hackathon Start Sequence (T+0)

1. Freeze docs and task structure. (`TASKS.md` ownership/status continues updating.)
2. Generate shared API headers from interface spec.
3. Generate mock implementations.
   - **Mock location:** `src/<workstream>/mocks/`. Toggled via CMake option (e.g., `-DUSE_RENDERER_MOCKS=ON`).
   - **Swap:** when real impl is milestone-ready, human flips CMake toggle, verifies build, deletes mock once stable.
4. Start three workstreams in parallel.
5. Start queue-driven secondary workflow on RTX 3090 and overflow channels.

### 11.3 Secondary-Agent Dispatch

- Hard graphics/math/systems → Claude Sonnet first; GLM 5.1 or Claude Opus as needed.
- Standard implementation → Copilot first; Claude or Gemini as backup.
- Tests / narrow spec check / shader check / short diff review → local RTX 3090 first if idle; Gemini if prompt is large or local box busy.
- Milestone validation across several files → Gemini, GLM 5.1, or Claude Sonnet; local model only with aggressively trimmed prompts.

---

## 12. Spec-Driven Planning

**FIXED (Iter 8):** SpecKit runs **once per workstream** (§8.8). Planning outputs must specify:
- Clear milestone unlocks and mock-to-real transitions.
- Validation level per task and agent routing (cloud vs local, Sonnet vs GLM 5.1 vs Copilot).
- Explicit mapping from SpecKit artifacts into the stable doc and coordination schema (§8.9).

---

## 13. Quality Gates

| Gate | Rule | Enforcer | Timing |
| :-- | :-- | :-- | :-- |
| G1 | `cmake --build` returns 0 | Implementing agent | Before DONE |
| G2 | Relevant tests pass | Implementing agent / Test Author | Before DONE where applicable |
| G3 | Frozen interfaces unchanged without approval | Human + validator | Continuous |
| G4 | Required validation completed | Spec Validator / Reviewer | Before merge when required |
| G5 | Milestone-level validation completed | Spec Validator + human + reviewer | Before milestone merge |
| G6 | `main` remains demo-safe | Humans | Continuous |
| G7 | Parallel group file sets remain disjoint | Task author (pre-claim) + claiming agent (pre-edit) | Before task claim |

### 13.1 Scope-Cut Order (Contingency)

If time runs short, cut in this order:
1. Advanced VFX / Shields
2. Blinn-Phong (fallback to Lambertian)
3. Enemy AI steering (fallback to static/random)
4. Procedural asteroid complexity
5. Normal maps / Textures

### 13.2 Finalization and Demo Strategy

- **T-30 min: Feature Freeze.** No new feature merges to `main`; only bugfixes, asset/shader tweaks, demo stabilization.
- **T-10 min: Branch Freeze.** No further merges; all focus on `rtx3090`.
- **Demo machine:** RTX 3090, kept synced with `main` throughout final 60 minutes.
- **Fallback:** If a workstream is broken at T-30, re-enable its mocks on `main` to keep demo launchable.

---

## 14. Pre-Hackathon Checklist

### Infrastructure
- [ ] Ubuntu 24.04 LTS verified on all machines. **(VERIFY: NVidia drivers and Vulkan support)**
- [ ] Agreed hostnames set in `/etc/hostname` and recorded in `AGENTS.md`.
- [ ] Clang, CMake, Ninja installed and tested.
- [ ] Repo and worktrees created on all machines including RTX 3090.
- [ ] FetchContent cache warmed on all machines. **(VERIFY)**
- [ ] Asset and shader runtime paths configured and verified.
- [ ] Queue and helper scripts tested.
- [ ] Build baseline opens a Linux window and clears the screen.
- [ ] `quickstart.md` / runbook created and tested.

### Agent Configuration
- [ ] `AGENTS.md` finalized: global rules only, hostname table, `agent_name@machine` convention.
- [ ] `CLAUDE.md` imports `AGENTS.md`; contains only Claude-specific notes.
- [ ] `.github/copilot-instructions.md` mirrors critical rules directly.
- [ ] `.gemini/settings.json` context file points to `AGENTS.md`. **(VERIFY: settings syntax and path resolution)**
- [ ] Role and domain skills generated and reviewed: `sokol-api`, `entt-ecs`, `cgltf-loading`, `glsl-patterns`, `physics-euler`.
- [ ] Copilot, Claude, Gemini, and z.ai workflows verified on real repo structure. **(VERIFY: Copilot CLI availability and Gemini throughput)**

### Local Agent (RTX 3090)
- [ ] Context configured with documented prompt budget smaller than max.
- [ ] Narrow validation and medium test-file generation tested. **(VERIFY: model latency and context limits)**
- [ ] Output-cap-aware prompting strategy documented.
- [ ] `validate_task.sh` or equivalent helper created.

### Coordination Files
- [ ] `docs/` vs `_coordination/` split implemented.
- [ ] Per-workstream `TASKS.md` with extended schema including `Parallel_Group`.
- [ ] File sets in `Notes` for all tasks in parallel groups.
- [ ] `MASTER_TASKS.md` created after cross-workstream synthesis pass.
- [ ] Per-workstream `VALIDATION/` and `REVIEWS/` directories created.
- [ ] Milestone Groups and Milestones with `Expected Outcome` statements for all MVP work.

### Skills and Large Headers
- [ ] Distilled `SKILL.md` for `sokol_gfx`, `entt`, `cgltf`, and other heavy headers.
- [ ] Per-aspect reference files under `.agents/skills/<lib>-api/references/` for `sokol_gfx` at minimum.
- [ ] `SKILL.md` files contain "Deeper references" section listing aspect files.
- [ ] `AGENTS.md` rule: "Use library SKILLs before opening large headers; quote only minimal snippets when needed."
- [ ] Dry run: agent uses only SKILL docs to implement a small feature for each major library.

### Planning
- [ ] Per-workstream SpecKit runs completed (renderer, engine, game — separately).
- [ ] Systems Architect synthesis pass run; `MASTER_TASKS.md` populated.
- [ ] Interface contracts frozen before individual SpecKit passes.
- [ ] Shared API header and mock generation rehearsed. **(VERIFY)**
- [ ] Milestone sync rules documented.
- [ ] Task claim semantics confirmed (human commit and trigger).
- [ ] Milestone acceptance, unlock, and merge conditions written for every MVP milestone.
- [ ] GLM 5.1 backup policy documented. **(VERIFY: quota and latency)**
- [ ] Local validator fallback path to Gemini documented.
- [ ] Final demo strategy: RTX 3090 from `main` with improvisation for late integration.
- [ ] **10-minute demo rehearsal at T-1h on the demo machine.**

---

## 15. Fixed Decisions — Cumulative Through Iteration 8

1. Sections 1–7 are the stable high-level project and agentic baseline.
2. `AGENTS.md` contains only global guardrails; tool-specific and domain knowledge lives in SKILL files.
3. Copilot and Gemini anchor on `AGENTS.md` via their config mechanisms; minimal duplication in `.github/copilot-instructions.md`.
4. `_coordination/` (operational state) vs `docs/` (reference material) split is fixed.
5. Per-workstream `TASKS.md` + compact `MASTER_TASKS.md` is the task management structure.
6. Validation is risk-based per task; mandatory per milestone.
7. Large headers handled via **Skill + per-aspect reference files** pattern — "skills first, header snippets second."
8. Per-aspect reference files live under `.agents/skills/<lib>-api/references/`; each covers one narrow operational concern (descriptors, buffers, render passes, draw calls, etc.).
9. Task ownership: `agent_name@machine` (e.g., `claude@laptopA`). Manual tasks: `FirstName@machine`. Hostnames set pre-hackathon and recorded in `AGENTS.md`.
10. Agent unable to determine machine name: do not overwrite populated `Owner`; add note in `Notes`.
11. `Parallel_Group`: `PG-<milestone>-<letter>` for concurrent-ready tasks; `BOTTLENECK` for exclusive gates; `SEQUENTIAL` for ordered-but-not-exclusive tasks (default).
12. Parallel group tasks must have **disjoint file sets** in `Notes`. Agent discovering violation must pause, update task, check conflicts, and flag blocker if necessary.
13. Milestones grouped into **Milestone Groups** (thematic, no mandatory integration event) and individual **Milestones** (formal sync with `Expected Outcome`). Target: ~1 milestone merge/hour/workstream.
14. SpecKit runs **once per workstream** (three runs: renderer, engine, game) then Systems Architect synthesis → `MASTER_TASKS.md`. Interface contracts frozen before individual passes.
15. Skills are lazy-loaded: agents load only skills relevant to current task and workstream.
16. SpecKit outputs are precursor artifacts; must be synthesized into stable schema before hackathon starts.
17. **(Iter 9)** Build topology: `renderer` + `engine` static libs with standalone `renderer_app` / `engine_app` driver executables for solo iteration against procedural scenes; `game` executable; per-workstream target-scoped builds; full rebuild only at milestone sync (§3.5).
18. **(Iter 9, revised)** Shaders **precompiled ahead-of-time via `sokol-shdc`** into per-backend headers; runtime GLSL loading and hot-reload cut. Shader-creation failures log and fall back to a magenta pipeline, never crash (§3.3).
19. **(Iter 9)** Asset paths resolved via configure-time `ASSET_ROOT` macro from generated `paths.h`; no relative-path lookups at runtime (§3.6). Shaders require no runtime path (headers are linked in).

---

> **Suggested focus for Iteration 9:** Produce a worked example of a complete `sokol-api/SKILL.md` with per-aspect reference files (`sokol-api-work-with-buffers.md`, `sokol-api-render-passes.md`, `sokol-api-draw-calls.md`), and a fully populated renderer `TASKS.md` fragment covering Milestone Groups "Bootstrap" and "Shading" with parallel groups, file sets, and bottleneck markers.

[^1]: Vibe-Coding-Hackathon-Master-Blueprint-Iteration-7.md
