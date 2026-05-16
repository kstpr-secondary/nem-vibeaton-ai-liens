# AGENTS.md — Repository operating guide

> Authoritative behavior rules for every AI agent working in this repo. Tool-specific notes live in `CLAUDE.md`, `.github/copilot-instructions.md`, and `.gemini/settings.json`. Workstream-specific knowledge lives in `.agents/skills/`.

## 1. Project state

This repo contains three C++17 workstreams:

1. **Renderer** (`src/renderer/`) — OpenGL 3.3 Core renderer using `sokol_gfx`/`sokol_app`; owns window/app lifecycle, frame callback, pipelines, shaders, mesh upload, and draw submission.
2. **Engine** (`src/engine/`) — ECS (`entt`), asset import (`cgltf`, `tinyobjloader`), Euler physics, AABB/raycast queries, camera computation, renderer bridge.
3. **Game** (`src/game/`) — the space-shooter game executable: controls, combat, enemy AI, HUD, match flow, and game-layer ECS state.

Work is organized per-feature.

## 2. Active documentation model

Active feature work lives under `features/active/<feature-name>/`. See `WORKFLOW.md` for the full process.

Key artifacts per feature:
- `brief.md` — what and why; always present
- `roadmap.md` — exploratory features only; full journey sketch with decision branches
- `spike-<name>.md` — one per unknown resolved by experiment
- `design.md` — technical design when needed
- `plan-pN.md` — phase plan, one per phase, written before execution
- `checkpoint-pN.md` — human-written after verification; gates the next phase plan
- `review-findings.md` — append-mode log of reviewer defects with process implications; read by `review-loop-retrospector` at phase end; archived with the feature

**Before starting implementation**: read `brief.md`, the current `plan-pN.md`, and any `checkpoint` files for context on prior phases. Do not read phase plans that have not yet been reached.

Completed features are archived under `features/archive/<feature-name>/`. Older work lives under `specs/` (original SpecKit outputs) and `pre_planning_docs/` (hackathon artifacts) — treat those as historical reference only.

Stable reference docs:
- `docs/interfaces/` — frozen public contracts
- `docs/architecture/` — subsystem architecture
- `docs/game-design/` — gameplay intent
- `.agents/skills/` — role and library guidance

## 3. Build and platform constraints

- C++17, CMake, Ninja, Clang, warnings `-Wall -Wextra -Wpedantic`, `-Werror` off.
- Dependencies via **FetchContent only**.
- **OpenGL 3.3 Core only**.
- Shaders live under `shaders/{renderer,game}/` and are compiled by `sokol-shdc` into generated headers.
- Shader creation failures must log and fall back to the **magenta error pipeline**.
- Runtime asset paths must be composed from `ASSET_ROOT`.

Target-scoped builds:

```bash
cmake --build build --target renderer_app renderer_tests
cmake --build build --target engine_app engine_tests
cmake --build build --target game
cmake --build build
```

Top-level `CMakeLists.txt` is co-owned by Renderer and Systems Architect.

## 4. Ownership boundaries

- **Renderer owns** `sokol_app`, frame lifecycle, pipelines, shaders, textures, mesh upload, skybox, line quads.
- **Engine owns** ECS lifecycle, component schema, physics, scene queries, asset import, camera matrices, renderer-facing scene submission.
- **Game owns** gameplay systems, tuning, HUD, weapons, enemy behavior, match state, game-local components.

Do not move behavior across workstreams unless the feature explicitly requires it.

## 5. Frozen interfaces

`docs/interfaces/*-interface-spec.md` are cross-workstream contracts. Do not edit a frozen interface to make code compile. If a feature requires an interface change, stop and surface that clearly for human approval.

## 6. Working rules

- Read the active feature docs before editing code.
- Use the relevant skill first; open large headers only if the skill is insufficient.
- Keep changes within the owning workstream unless the feature explicitly spans multiple workstreams.
- Prefer precise, feature-scoped changes over cleanup outside the task.
- Keep `main` demo-safe.
- Every Phase Plan and every Exploratory Roadmap must pass Plan Groomer review before implementation starts (`.agents/skills/plan-groomer/SKILL.md`).
- Do not generate `plan-pN.md` without `checkpoint-p(N-1).md` present; for Phase 1, the Feature Brief serves as the gate.
- **Before executing any task in a phase plan**: verify the plan's `Groomer Verdict` field reads `PASS`. If it reads `PENDING` or the field is absent, stop and tell the human to invoke Plan Groomer before any implementation begins.
- **As you complete each task in a phase plan**: update that task's `Status` field from `[ ]` to `[x]` in the plan document before moving to the next task.
- **When all tasks in a phase plan are `[x]`**: stop. Do not write `checkpoint-pN.md`, do not write or sketch `plan-p(N+1).md`, and do not begin designing the next phase. Output the phase-completion handoff instead (see below).
- **Implementing agents never write checkpoints.** The checkpoint records human-observed behavior. Feature Planner may draft it from the human's free-form feedback, but only after the human has run the verification. The next phase plan cannot exist without the checkpoint file.

**Phase-completion handoff** (output when all tasks are `[x]`):
1. Briefly summarize what was implemented in this phase.
2. Quote the Human Checkpoint section verbatim from the plan (Run / Look for / Pass / Stop).
3. Tell the human to run the verification. When they report results back, ask them to provide: **Result** (PASS or STOP), **What they observed** (specific, not "it works"), and **Anything unexpected or that should inform the next phase**. These map directly to the checkpoint fields and prevent ambiguity when Feature Planner drafts the checkpoint. Tell them to write `features/active/<feature-name>/checkpoint-pN.md` using the template at `.agents/skills/feature-planner/templates/checkpoint.md.template`.
4. If `features/active/<feature-name>/review-findings.md` exists and contains entries from this phase, tell the human to invoke `review-loop-retrospector` before the Human Checkpoint draft. The retrospector reads the accumulated findings, corrects any responsible workflow docs or skills, and marks entries as retrospected. This step completes the review loop before the next phase begins.
5. Tell the human to invoke Feature Planner once the checkpoint is written (for the next phase), or Doc Updater if this was the final phase.
6. State explicitly: next-phase planning belongs to Feature Planner, not to this agent.

## 7. Validation expectations

- Use tests for deterministic math, ECS, parser, and helper logic.
- Use human behavioral checks for rendering correctness, gameplay feel, and live demo behavior.
- Use spec validation when the question is “did this match the feature docs?”
- Use code review when the question is “is this diff risky or obviously broken?”

Implementing agents (`renderer-specialist`, `engine-specialist`, `game-developer`) invoke `test-author` during the Execute phase to write any tests required by the phase plan. `test-author` is not a separate pipeline role that follows implementation — it is a supporting skill owned by the workstream specialist.

**Validation roles are read-only.** plan-groomer, code-reviewer, and spec-validator produce reports, never edits. Implementing agents fix what those reports name.

**Finding entries after reviewer fixes.** When an implementing agent applies a fix from code-reviewer or spec-validator output, it must assess whether the defect has process implications (not a one-off typo). If yes, append one structured entry to `features/active/<feature-name>/review-findings.md` using the format in `.agents/skills/review-loop-retrospector/SKILL.md`. Assign: the abstract defect class, the suspected responsible party (planner/groomer | implementor-skill | reviewer | shared-docs), and a one-line abstract evidence note. This file is the input for `review-loop-retrospector` at phase completion.

## 8. If stuck

1. Re-read the active feature doc and the relevant skill.
2. Open only the minimal supporting reference needed.
3. If the task appears to require a frozen interface change, stop and flag it.
4. If a skill is stale or missing key project facts, update the skill instead of inventing policy.

## 9. Excluded directories

- **Do not** perform file searches in `.agents-ignore` and `.specify` directories.