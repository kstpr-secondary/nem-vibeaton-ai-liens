---
name: systems-architect
description: Use when a feature touches multiple workstreams (renderer, engine, game), the build topology needs changing, or frozen interface contracts may be affected. Do NOT use for implementation inside a single workstream — that belongs to `renderer-specialist`, `engine-specialist`, or `game-developer`.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: hackathon-team
  version: "2.0"
  project-stage: post-hackathon features
  role: systems-architect
  activated-by: human-explicit
---

# Systems Architect

The Systems Architect is the **cross-workstream reference** for the three-project codebase (renderer → engine → game). It owns the seams between workstreams and the build topology. It does **not** plan tasks, run synthesis passes, or manage coordination queues — those were hackathon-era mechanisms that have been retired in favor of simpler research+plan documents directly implemented by agents.

This skill is a **lookup reference**, not an operational playbook. Invoke it only when a feature touches multiple workstreams, the build topology needs changing, or frozen interface contracts may be affected.

---

## 1. When to invoke

Invoke this skill when:

- A new feature's research/plan spans **two or more** workstreams (e.g., a new shader pipeline that changes renderer code and engine mesh-upload code).
- Someone proposes adding a new CMake target, new FetchContent dependency, or cross-workstream build change.
- A feature needs to modify a **frozen interface contract** (`docs/interfaces/*-interface-spec.md`) — the human must approve before any change.
- Planning a new milestone and the agent needs the cross-workstream dependency matrix to set correct `Depends_on` values in tasks.

Do **not** invoke this skill for: implementation inside a single workstream, unit test authoring, code review, spec-adherence validation, or task planning within one workstream's scope.

---

## 2. Workstream map and seams

```
sokol_app → renderer (static lib) → engine (static lib) → game (executable)
              ↕                      ↕
         renderer_app            engine_app   (standalone drivers for solo iteration)
              ↕
         renderer_tests          engine_tests  (Catch2 unit tests)
```

### Seams (do not cross unilaterally)

| Seam | Owner | Consumer contract |
| :--- | :--- | :--- |
| `sokol_app` init + main frame callback | Renderer | Engine ticks from inside the renderer callback; input flows `sokol_app → renderer → engine → game` via callback registration. |
| Procedural mesh builders | Renderer | Engine wraps them into entity-spawn helpers; no duplicate paths in engine. |
| `set_camera(view_matrix, projection_matrix)` | Renderer | Engine computes matrices from a Camera entity; renderer just applies them. |
| `set_directional_light(...)` | Renderer | Exactly **one** directional light at MVP. |
| Mesh upload bridge | Renderer | Asset data → renderer `upload_mesh` → handle returned to engine. |
| Scene API (create/destroy/add/get/remove/view) | Engine | Game consumes this; engine owns ECS lifecycle. |
| Physics queries (raycast, overlap) | Engine | Game queries for gameplay logic. |

### Frozen interface specs

These files are contracts between workstreams. Never edit them to make code compile — flag a blocker and wait for human approval.

- `docs/interfaces/renderer-interface-spec.md`
- `docs/interfaces/engine-interface-spec.md`
- `docs/interfaces/game-interface-spec.md`

---

## 3. Build topology

| Target | Kind | Source | Links | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `renderer` | static lib | `src/renderer/` | sokol, glm | Linked by `engine`, `game`, drivers |
| `renderer_app` | executable | `src/renderer/app/` | `renderer` | Standalone renderer driver |
| `engine` | static lib | `src/engine/` | `renderer`, entt, cgltf | Linked by `game`, `engine_app` |
| `engine_app` | executable | `src/engine/app/` | `engine`, `renderer` | Standalone engine driver |
| `game` | executable | `src/game/` | `engine`, `renderer` | Full game |
| `renderer_tests` | executable | Catch2 | `renderer` | Unit tests |
| `engine_tests` | executable | Catch2 | `engine` | Unit tests |

**Rules:**
- Dependencies via **FetchContent only** — no Conan, vcpkg, or system packages.
- CMakeLists.txt is co-owned by Renderer workstream and Systems Architect. Cross-workstream build changes need a 2-minute notice to other workstreams.
- Per-workstream target-scoped builds (`cmake --build build --target <target>`); full rebuild only at milestone sync.

---

## 4. Cross-workstream dependency matrix

Use this when setting `Depends_on` for new features that touch multiple workstreams.

| Milestone | Renderer dep | Engine dep |
| :--- | :--- | :--- |
| E-M1 | R-M1 (unlit meshes, camera) | — |
| E-M2 | R-M1 | E-M1 |
| E-M3 | R-M1 | E-M1 |
| E-M4 | R-M1 | E-M3 |
| E-M5 (desirable) | R-M2 | E-M4 |
| E-M6 (desirable) | R-M2, ideally R-M5 | E-M1 |
| E-M7 (desirable) | R-M5 | E-M1 |
| G-M1 | R-M1 | E-M1 |
| G-M2 | R-M1 | E-M4 (or mocks) |
| G-M3 | R-M1 + R-M3 | E-M3 + E-M4 |
| G-M4 | R-M1 + ImGui integration | E-M1 |
| G-M5 (desirable) | — | E-M5 |
| G-M6 (desirable) | R-M5 (custom shader + alpha) | E-M4 |

---

## 5. Confirmed facts

These are architectural constants. Do not relitigate unless the human explicitly reopens a decision.

- **Three workstreams, one repo**: renderer → engine → game.
- **Build**: CMake + Ninja + Clang, C++17, warnings `-Wall -Wextra -Wpedantic`, `-Werror` off.
- **Graphics backend**: OpenGL 3.3 Core only. Vulkan is removed.
- **Shaders**: annotated `.glsl` → `sokol-shdc` → per-backend headers at build time. No runtime GLSL, no hot-reload. Failed shader creation → **magenta fallback pipeline**, never crash.
- **Asset paths**: configure-time `ASSET_ROOT` macro in generated `paths.h`. No relative-path lookups at runtime.
- **sokol_app init + main frame callback + input event pump live in the renderer**. Engine ticks from inside the callback; input flows `sokol_app → renderer → engine → game`.
- **Procedural shape builders live in the renderer**; engine wraps them into entity-spawn helpers. No duplication.
- **Only one directional light at MVP**; point lights are desirable (E-M6).
- **Alpha blending (basic) is MVP** for laser/plasma VFX transparency.
- **Capsule mesh builder is Desirable**, not MVP. Engine E-M1 uses sphere + cube only.

### Authoritative sources

1. `pre_planning_docs/Hackathon Master Blueprint.md` (iteration 8) — historical reference for decisions.
2. `pre_planning_docs/*Concept and Milestones.md` — per-workstream design seeds.
3. `AGENTS.md` — global behavior rules.
4. `docs/interfaces/*-interface-spec.md` — frozen interface contracts.
5. Per-workstream architecture docs under `docs/architecture/`.

**Do not invent** requirements or decisions not supported by these files. If a decision is needed and unresolved, mark it as an **Open Question** and escalate to the human supervisor.

---

## 6. Gotchas

- **"line renderer" vs "world-space quads for lasers"** — same thing. Normalize vocabulary: use "lines" or "world-space quads" consistently within any new document.
- **G-M5+ are desirable, not MVP**. Do not collapse desirable milestones into MVP scope.
- **Shaders are compiled at build time via sokol-shdc**. No runtime GLSL paths, no glslang integration, no hot-reload. Runtime shader errors fall back to the magenta pipeline.
- **Asset paths must use `ASSET_ROOT`**. Planning that relies on cwd or relative paths will break on the demo machine.
- **Input plumbing is renderer-owned**. Any engine-side input decision must respect the renderer → engine callback chain.
- **CMakeLists.txt is co-owned**. Unilateral top-level CMake edits violate the build governance rules.
- **`_coordination/` is archived.** Post-hackathon features use simpler research+plan documents directly implemented by agents. The old MASTER_TASKS.md synthesis pass, parallel groups, and coordination queues are retired.

---

## 7. Output templates

**Cross-workstream feature plan** → `docs/planning/<feature>/`:
```
# <Feature Name>
## Overview
## Workstreams affected
## Interface changes (if any)
## Build topology changes (if any)
## Open questions
```

**Interface change request** → `docs/interfaces/<workstream>-interface-spec.md` (new version):
```
# <Workstream> Interface Spec — vX.Y frozen YYYY-MM-DD HH:MM
## Public API surface
## Ownership notes
## Change log
## Migration notes for downstream
```

Any output that cannot cite an authoritative source must flag an **Assumption** or an **Open Question**.

---

## Companion files

- `AGENTS.md` — global rules every agent follows.
- `.agents/skills/skill-creator/SKILL.md` — meta-skill for creating new skills.
- Per-workstream specialist SKILLs: `renderer-specialist`, `engine-specialist`, `game-developer`.
- Validator / reviewer SKILLs: `spec-validator`, `code-reviewer`, `test-author`.
