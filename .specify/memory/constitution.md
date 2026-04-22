<!--
SYNC IMPACT REPORT
==================
Version change:    (none → 1.0.0) — initial population of all placeholders
Modified principles: N/A — first constitution fill
Added sections:
  - Core Principles (5 principles)
  - Technology Constraints
  - Workstream Discipline & Quality Gates
  - Governance
Removed sections: N/A
Templates requiring updates:
  - .specify/templates/plan-template.md ✅ Constitution Check section already present; principles now concrete
  - .specify/templates/spec-template.md ✅ No constitution-specific sections required
  - .specify/templates/tasks-template.md ✅ Task types align with principles
Deferred TODOs: None — all placeholders resolved
-->

# NEM Vibeaton Constitution

## Core Principles

### I. Behavioral Correctness (NON-NEGOTIABLE)

Correctness is the single highest priority. A live demo with working behavior beats polished,
extensible, or fast code that crashes or produces wrong output. Every feature MUST satisfy its
acceptance criteria before being considered done. Rendering, physics, and gameplay behavior MUST
be human-verified on the demo machine before a milestone merges to `main`.

**Rationale:** This is a 5–6 hour hackathon with a live demo goal. A broken demo has zero value
regardless of code quality.

### II. Milestone-Driven Integration

Workstreams (renderer, engine, game) MUST integrate only at milestone boundaries. Each workstream
MUST start from frozen interface contracts and mocks; upstream delivery MUST NOT block downstream
iteration. Cross-workstream changes to a frozen interface MUST receive explicit human approval
before implementation begins.

**Rationale:** Parallel development on three machines requires strict interface isolation.
Uncoordinated cross-workstream changes are the primary risk to timeline and demo stability.

### III. Speed Over Long-Term Maintainability

Implementation decisions MUST bias toward getting a working demo, not toward future extensibility.
YAGNI applies strictly: features not required for the demo MUST be cut. Complexity introduced
beyond the minimum MUST be explicitly justified. Elegance, refactoring, and architectural purity
are explicitly deprioritized within the hackathon window.

**Rationale:** The project lifespan is one hackathon session. Decisions optimized for a six-month
codebase are actively harmful here.

### IV. AI-Generated Under Human Supervision

Every line of C++ MUST be AI-generated. Humans MUST supervise: approve interface changes, run
behavioral checks, resolve parallel-group file conflicts, and perform milestone-level validation.
Manual human coding is an emergency-only path and MUST be logged. AI agents MUST NOT self-claim
tasks; human supervisors commit task claims before triggering agents.

**Rationale:** The demo goal is a live showcase of AI-generated code under human direction.
Undermining this by humans writing code silently defeats the project's premise.

### V. Minimal Fixed Stack — No Additions Without Approval

The technology stack is fixed (see Technology Constraints). Agents MUST NOT introduce new
libraries, build systems, package managers, or tooling without explicit human approval. All
dependencies MUST use CMake FetchContent with pinned versions. System packages, Conan, and vcpkg
are prohibited. If a library is insufficient, a blocker MUST be logged in
`_coordination/overview/BLOCKERS.md` — agents MUST NOT silently work around constraints.

**Rationale:** Uncontrolled dependency additions during a hackathon break build reproducibility
across the four machines and introduce unacceptable integration risk.

## Technology Constraints

The following technology choices are **FIXED** and MUST NOT be changed without human approval.
Agents that encounter a reason to deviate MUST log a blocker and stop, not improvise.

- **Language**: C++17. Warnings: `-Wall -Wextra -Wpedantic`. `-Werror` is off.
- **Build**: CMake + Ninja + Clang. Configure once; iterate per-target.
- **Dependencies**: FetchContent only. All versions pinned. No system packages.
- **Graphics**: OpenGL 3.3 Core via `sokol_gfx`. Vulkan is removed.
- **Window/Input**: `sokol_app`, owned exclusively by the renderer workstream.
- **ECS**: `entt` v3.x (pinned).
- **Asset import**: `cgltf` (glTF/GLB primary), `tinyobjloader` (OBJ fallback). Max two formats.
- **Math**: GLM 1.x (pinned).
- **Testing**: Catch2 v3 (pinned). Rendering correctness: human behavioral check only.
- **UI/HUD**: Dear ImGui via renderer-owned `util/sokol_imgui.h`. No GLFW/SDL backends.
- **Shaders**: Annotated `.glsl` under `shaders/{renderer,game}/`, precompiled by `sokol-shdc`
  into `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. No runtime GLSL loading.
- **Asset paths**: Composed from the `ASSET_ROOT` macro generated in `paths.h`.
  Hard-coded relative paths are prohibited.
- **Shader errors**: MUST log and fall back to a magenta error pipeline. Never crash.
- **Explicit cuts**: audio, networking, skeletal animation, particles, editor tooling, Vulkan,
  shader hot-reload, frustum culling, shadow mapping, PBR, deferred rendering.

Per-workstream iteration build commands (agents MUST use these; do not rebuild unrelated targets):

```bash
cmake --build build --target renderer_app renderer_tests  # renderer
cmake --build build --target engine_app engine_tests      # engine
cmake --build build --target game                         # game
cmake --build build                                       # full milestone-sync only
```

## Workstream Discipline & Quality Gates

**Workstreams**: `renderer` (Laptop A), `engine` (Laptop B), `game` (Laptop C).
**Validation/demo machine**: RTX 3090 PC.

### Parallel Group File Ownership

Tasks in the same `PG-*` parallel group MUST declare disjoint file sets in `Notes: files: …`.
Before editing, the agent MUST verify the file set. If a file outside the declared set is needed:
1. Pause.
2. Update the file set in `TASKS.md`.
3. Check for conflicts with other tasks in the same group.
4. If conflict exists: log in `_coordination/overview/BLOCKERS.md` and wait. Do not race.

### Quality Gates

| Gate | Rule | Enforcer | Timing |
|:-----|:-----|:---------|:-------|
| G1 | `cmake --build` returns 0 | Implementing agent | Before `DONE` |
| G2 | Relevant Catch2 tests pass | Implementing agent / Test Author | Before `DONE` where applicable |
| G3 | Frozen interfaces unchanged without approval | Human + Spec Validator | Continuous |
| G4 | Required validation completed | Spec Validator / Code Reviewer | Before merge when required |
| G5 | Milestone-level validation + human behavioral check | Spec Validator + human + Reviewer | Before milestone merge |
| G6 | `main` remains demo-safe | Humans | Continuous |
| G7 | Parallel-group file sets remain disjoint | Task author + claiming agent | Before task claim |

### Validation and Review

Validation and review are triggered via queue files, not hidden hooks:
- `_coordination/queues/VALIDATION_QUEUE.md` — spec adherence checks
- `_coordination/queues/REVIEW_QUEUE.md` — risk and bug reviews
- `_coordination/queues/TEST_QUEUE.md` — test authoring requests

Risk tiers: `SELF-CHECK` (low/trivial) → `SPEC-VALIDATE` or `REVIEW` (medium) →
`SPEC-VALIDATE + REVIEW` (high/milestone-critical). Every milestone merge requires a human
behavioral check plus lightweight code review.

### Agent Stall Rule

If an agent stalls > 90 s on any subtask, it MUST stop and log in
`_coordination/overview/BLOCKERS.md` so the human supervisor can re-route. Silent spinning is
prohibited.

## Governance

This constitution supersedes all other practices. In the event of conflict, this document wins.
Amendments require:
1. Human supervisor approval.
2. Update to this file with version bump per semantic versioning:
   - **MAJOR**: backward-incompatible principle removals or redefinitions.
   - **MINOR**: new principle or section added or materially expanded.
   - **PATCH**: clarifications, wording, typo fixes.
3. Propagation to `AGENTS.md` and any affected template files.
4. Announcement to all active agents before they begin new tasks.

All task claims, validations, reviews, and blockers MUST be logged in `_coordination/` as the
live operational record. `MASTER_TASKS.md` is the human supervisor's integration dashboard;
workstream `TASKS.md` files are authoritative on their feature branches.

Runtime development guidance lives in `AGENTS.md` (authoritative for all agents) and
tool-specific companions (`CLAUDE.md`, `.github/copilot-instructions.md`, `.gemini/settings.json`).

**Version**: 1.0.0 | **Ratified**: 2026-04-20 | **Last Amended**: 2026-04-23
