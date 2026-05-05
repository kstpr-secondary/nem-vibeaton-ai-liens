# AGENTS.md — Repository operating guide

> Authoritative behavior rules for every AI agent working in this repo. Tool-specific notes live in `CLAUDE.md`, `.github/copilot-instructions.md`, and `.gemini/settings.json`. Workstream-specific knowledge lives in `.agents/skills/`.

## 1. Project state

This repo contains three C++17 workstreams:

1. **Renderer** (`src/renderer/`) — OpenGL 3.3 Core renderer using `sokol_gfx`/`sokol_app`; owns window/app lifecycle, frame callback, pipelines, shaders, mesh upload, and draw submission.
2. **Engine** (`src/engine/`) — ECS (`entt`), asset import (`cgltf`, `tinyobjloader`), Euler physics, AABB/raycast queries, camera computation, renderer bridge.
3. **Game** (`src/game/`) — the space-shooter game executable: controls, combat, enemy AI, HUD, match flow, and game-layer ECS state.

The hackathon coordination workflow is retired. Current work is feature-based.

## 2. Active documentation model

Current feature work may be documented in either of these shapes:

- **Structured feature folder** under `specs/<feature>/` with files such as `spec.md`, `plan.md`, `tasks.md`, `research.md`, `data-model.md`, and `contracts/*`.
- **Single combined markdown design doc** such as `pre_planning_docs/next-gen-tasks/visual-improvements.md` that may contain research, diagnostics, design, phases, tasks, and acceptance notes in one file.

When starting work, read the feature document the user points at. If it is a combined markdown file, use the sections that matter to the task:

- problem statement / diagnostics
- design or proposed changes
- phases
- task list
- checkpoints / acceptance notes

Use stable reference docs only when the feature doc points at them or when the task clearly depends on them:

- `docs/interfaces/` — frozen public contracts
- `docs/architecture/` — subsystem architecture
- `docs/game-design/` — gameplay intent
- `.agents/skills/` — role and library guidance

`_coordination/` is archived and should not drive new work.

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

## 7. Validation expectations

- Use tests for deterministic math, ECS, parser, and helper logic.
- Use human behavioral checks for rendering correctness, gameplay feel, and live demo behavior.
- Use spec validation when the question is “did this match the feature docs?”
- Use code review when the question is “is this diff risky or obviously broken?”

## 8. If stuck

1. Re-read the active feature doc and the relevant skill.
2. Open only the minimal supporting reference needed.
3. If the task appears to require a frozen interface change, stop and flag it.
4. If a skill is stale or missing key project facts, update the skill instead of inventing policy.
