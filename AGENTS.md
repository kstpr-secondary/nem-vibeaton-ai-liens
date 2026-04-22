# AGENTS.md — Cross-Tool Root Behavior

> Authoritative behavior rules for every AI agent working in this repo (Claude, Copilot, Gemini, GLM, local Qwen, …). Tool-specific notes live in their own files (`CLAUDE.md`, `.github/copilot-instructions.md`, `.gemini/settings.json`). Workstream-specific and library-specific knowledge lives in `.agents/skills/`.
>
> Master Blueprint: `pre_planning_docs/Hackathon Master Blueprint.md` (iteration 8). This file distills the parts every agent must obey every session.

---

## 1. What we are building

Three C++17 projects, one repo, built from scratch in a 5–6 hour hackathon:

1. **Rendering engine** (`src/renderer/`) — forward, OpenGL 3.3 Core via `sokol_gfx`; owns `sokol_app` init and the main frame callback.
2. **Game engine** (`src/engine/`) — ECS (`entt`), asset import (`cgltf`/`tinyobjloader`), Euler physics, AABB collision, raycast. Ticks from inside the renderer frame callback.
3. **Game** (`src/game/`) — 3D space shooter. Consumes the game-engine public API.

**Priorities, in order:** behavioral correctness → milestone predictability → integration discipline → speed. Elegance and extensibility are explicitly deprioritized. Speed over long-term maintainability. Every line of C++ is AI-generated under human supervision.

---

## 2. Hostnames and owner tags

Ubuntu 24.04 LTS everywhere. Hostnames are set in `/etc/hostname` pre-hackathon and are the authoritative machine identifiers.

| Hostname  | Primary role                              |
| :-------- | :---------------------------------------- |
| `laptopA` | Rendering engine                          |
| `laptopB` | Game engine                               |
| `laptopC` | Game                                      |
| `rtx3090` | Validation, tests, local model, demo box  |

**Owner tag in `TASKS.md`**: `<agent_name>@<machine>`.

- `agent_name` is lowercase tool name: `claude`, `copilot`, `gemini`, `glm`, `local-qwen`.
- Two instances of the same agent on one machine: append `-2`, `-3` → `claude-2@laptopA`.
- Human manual path: `FirstName@machine` (e.g., `Alice@laptopA`).
- If you cannot determine the machine name, **do not overwrite** a populated `Owner`; add `"assisted by <agent_name>, machine unknown"` to `Notes`.

---

## 3. Build stack (reminder — do not argue with these)

- **CMake + Ninja + Clang**, **C++17**, warnings `-Wall -Wextra -Wpedantic`; `-Werror` off.
- Dependencies via **FetchContent only** — no Conan, no vcpkg, no system packages.
- Graphics backend: **OpenGL 3.3 Core only**. Vulkan removed.
- Shaders: annotated `.glsl` under `shaders/{renderer,game}/`, **precompiled via `sokol-shdc`** into `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. Include the header; call `shd_<name>_shader_desc(sg_query_backend())`. No runtime GLSL loading, no hot-reload.
- Shader creation failures log and fall back to a **magenta error pipeline**. Never crash.
- Asset paths come from the generated `ASSET_ROOT` macro (`paths.h`). **Never hard-code relative asset paths**; always compose from `ASSET_ROOT`. Shaders need no runtime path.

### Per-workstream iteration build — do not rebuild unrelated targets

```bash
# Renderer workstream
cmake --build build --target renderer_app renderer_tests
# Engine workstream
cmake --build build --target engine_app engine_tests
# Game workstream
cmake --build build --target game
# Milestone-sync full rebuild (only then)
cmake --build build
```

After a milestone merge, the downstream workstream runs the full build **once** to refresh upstream static libs; routine iteration stays target-scoped.

**CMakeLists.txt ownership:** the Renderer workstream / Systems Architect owns the top-level `CMakeLists.txt`. Cross-workstream build changes require a **2-minute notice** to the other workstreams.

---

## 4. Repository map (anchors agents must know)

```
<repo>/
├── AGENTS.md                       ← this file
├── CLAUDE.md                       ← imports AGENTS.md via @AGENTS.md
├── .github/copilot-instructions.md ← short mirror of critical rules
├── .gemini/settings.json           ← "contextFileName": "AGENTS.md"
├── .agents/skills/                 ← role, library, and domain SKILLs
├── docs/
│   ├── architecture/               ← ARCHITECTURE.md + per-workstream
│   ├── interfaces/                 ← INTERFACE_SPEC.md + per-workstream specs (frozen)
│   ├── game-design/                ← GAME_DESIGN.md
│   └── planning/speckit/{renderer,engine,game}/
├── _coordination/                  ← live operational state
│   ├── overview/                   ← PROGRESS.md, BLOCKERS.md, MERGE_QUEUE.md, MASTER_TASKS.md
│   ├── {renderer,engine,game}/     ← TASKS.md, PROGRESS.md, VALIDATION/, REVIEWS/
│   └── queues/                     ← VALIDATION_QUEUE.md, REVIEW_QUEUE.md, TEST_QUEUE.md
├── shaders/
│   ├── renderer/*.glsl
│   └── game/*.glsl
└── src/{renderer,engine,game}/
```

- `docs/` — stable reference. Read-only between milestones.
- `_coordination/` — live state. Every task, blocker, and queue entry lives here.
- `.agents/skills/` — role skills, library cheat-sheets, per-aspect references.

---

## 5. Task schema

All task tables use this row schema:

```
| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
```

- **Tier**: `LOW` | `MED` | `HIGH` (implementation risk / difficulty).
- **Status**: `TODO` | `CLAIMED` | `IN_PROGRESS` | `READY_FOR_VALIDATION` | `READY_FOR_REVIEW` | `DONE` | `BLOCKED`.
- **Validation**: `NONE` | `SELF-CHECK` | `SPEC-VALIDATE` | `REVIEW` | `SPEC-VALIDATE + REVIEW`.
- **Parallel_Group**: `PG-<milestone>-<letter>` | `BOTTLENECK` | `SEQUENTIAL` (default).
- **Notes**: must include `files: <comma-separated list>` for any task in a `PG-*` group.

Workstream `TASKS.md` files are authoritative **on their feature branches**. Cross-workstream progress becomes visible only after a milestone merge to `main` + `git pull`. `MASTER_TASKS.md` is the human supervisor's integration dashboard, not a live feed.

---

## 6. Interfaces and milestones

- **Frozen interfaces** (`docs/interfaces/*-interface-spec.md`) are contracts between workstreams. Never edit them to make your code compile; instead, flag a blocker and wait for human approval. Tasks that touch a frozen interface must set `Depends_on` to the interface spec version.
- **Milestone-ready** means (all of): acceptance checklist met, required validation complete, human behavioral check done. Only then does the feature branch merge.
- **Target cadence:** ~1 milestone merge per hour per workstream (~5 per workstream over the hackathon).

---

## 7. Parallel groups and file ownership

- Tasks in the **same** `PG-*` group must have **disjoint file sets**, listed in `Notes` as `files: renderer.cpp, renderer.h`.
- Before editing, the claiming agent re-checks the file set. If the task needs a file outside its declared set:
  1. **Pause.**
  2. Update the file set in `TASKS.md`.
  3. Check whether the new file is claimed by another task in the same parallel group.
  4. If yes: flag in `_coordination/overview/BLOCKERS.md` and wait for human resolution. Do **not** race.
- `BOTTLENECK` tasks block every dependent until merged. While a BOTTLENECK is open, idle agents pull work from **other** milestones or workstreams (tests, docs, aspect references, validation queue entries). Use `BOTTLENECK` sparingly.
- Multiple agents may share a worktree/branch only if their file sets are disjoint per this rule.

---

## 8. Validation, review, and queues

Risk-based per task; mandatory per milestone. Trigger validation and review via queue files, **not hidden hooks**.

- **Low/trivial task** → `SELF-CHECK` unless touching shared interfaces, build system, or milestone-critical behavior.
- **Medium task** → `SPEC-VALIDATE` or `REVIEW` for nontrivial logic or integration surfaces.
- **High/hard task** → at least one secondary check before merge.
- **Every milestone** → Spec Validator + human behavioral check + lightweight Code Review.
- **Testing** — Catch2 for math, parsers, ECS logic. Rendering correctness is verified by human behavioral check + smoke-test visuals, **not** unit tests.

Queue files (add entries, do not invoke tools directly):

- `_coordination/queues/VALIDATION_QUEUE.md`
- `_coordination/queues/REVIEW_QUEUE.md`
- `_coordination/queues/TEST_QUEUE.md`

---

## 9. Skills and large headers

1. When starting a task, read:
   - the relevant `TASKS.md` row,
   - the role `SKILL.md` for your workstream,
   - only the library/domain SKILLs needed by this task,
   - per-aspect references under `.agents/skills/<lib>-api/references/` only when the task clearly falls in that aspect.
2. **Use the SKILL first; open the actual header only if the SKILL is insufficient.** Quote only the minimal snippet needed (relevant struct + 2–3 functions).
3. Large headers (`sokol_gfx.h`, `entt.hpp`, `cgltf.h`, …) are 20–30k LOC — loading them naively wastes context.
4. If a SKILL contains an error or missing API, flag it in `_coordination/overview/BLOCKERS.md` — the human supervisor fixes and pushes so all agents benefit. Do not quietly patch around drift.

---

## 10. Global rules

1. **Read before you edit.** Read the workstream `TASKS.md` row and the spec it references before touching code.
2. **Humans claim tasks.** The human supervisor updates `TASKS.md`, commits, and pushes before triggering an agent. Agents do not self-claim.
3. **Never change frozen shared interfaces** without explicit human approval.
4. **Every implementation task must reference** its milestone unlock or dependency.
5. **Respect `Validation`** — never silently downgrade a required check.
6. **Do not merge** milestone-ready work until acceptance checklist, required validation, and human behavioral check are all complete.
7. **Validation and review are queue-triggered**, not hidden hooks.
8. **Pull before starting** new work when multiple people/machines touch the same workstream.
9. **SKILLs first, headers second.** Quote only minimal header snippets when the SKILL is insufficient.
10. **`Owner` = `<agent_name>@<machine>`.** Unknown machine → leave existing owner intact and add a note.
11. **CMakeLists.txt** is owned by Renderer / Systems Architect. Cross-workstream build changes require 2-minute notice.
12. **Agent outage.** If you stall > 90 s on a subtask, stop and log in `_coordination/overview/BLOCKERS.md` so the human can re-route. Do not silently spin.
13. **SKILL drift** is fixed upstream. Flag it in `BLOCKERS.md`; do not silently work around it.
14. **Milestone validation:** every milestone merge requires a behavioral check by a **different person from the implementer** if possible; otherwise the human supervisor verifies each acceptance criterion on the demo machine.

---

## 11. Quality gates

| Gate | Rule                                               | Enforcer                              | Timing                       |
| :--- | :------------------------------------------------- | :------------------------------------ | :--------------------------- |
| G1   | `cmake --build` returns 0                          | Implementing agent                    | Before `DONE`                |
| G2   | Relevant tests pass                                | Implementing agent / Test Author      | Before `DONE` where applicable |
| G3   | Frozen interfaces unchanged without approval       | Human + Spec Validator                | Continuous                   |
| G4   | Required validation completed                      | Spec Validator / Code Reviewer        | Before merge when required   |
| G5   | Milestone-level validation completed               | Spec Validator + human + Reviewer     | Before milestone merge       |
| G6   | `main` remains demo-safe                           | Humans                                | Continuous                   |
| G7   | Parallel-group file sets remain disjoint           | Task author + claiming agent          | Before task claim            |

---

## 12. Source control

Branches: `main` / `integration` / `feature/renderer` / `feature/engine` / `feature/game`. `main` stays demo-safe at all times; `integration` is optional staging. Multiple agents may share a worktree/branch only if their file sets are disjoint (§7).

---

## 13. If you are stuck

1. Check the role SKILL and the per-aspect reference for the subsystem.
2. If the SKILL is wrong or missing, log in `_coordination/overview/BLOCKERS.md` and stop — do not invent API behavior.
3. If a frozen-interface change is tempting, stop; flag the blocker; wait for human approval.
4. If a file outside your declared set is needed, follow the §7 pause protocol.
5. If you stall > 90 s, stop and log in `BLOCKERS.md` per Rule 12.

---

> **Tool-specific companions**
> - `CLAUDE.md` — Claude-only notes; imports this file via `@AGENTS.md`.
> - `.github/copilot-instructions.md` — short mirror of the critical rules for Copilot.
> - `.gemini/settings.json` — points Gemini at `AGENTS.md` via `contextFileName`.
