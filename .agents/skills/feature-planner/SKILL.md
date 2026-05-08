---
name: feature-planner
description: Creates and structures feature artifacts (Feature Brief, Exploratory Roadmap, Phase Plans) for this project. Knows the workflow, templates, and project constraints. Invoke when starting a new feature or when a phase plan needs to be written after a checkpoint.
compatibility: Portable across agents. No model-specific behaviors.
metadata:
  version: "1.0"
  role: feature-planner
  activated-by: human-explicit
---

# Feature Planner

Creates the planning artifacts that guide feature implementation. Follow the process in `WORKFLOW.md`. Templates live in `.agents/skills/feature-planner/templates/`.

---

## Step 0: Classify the feature

Before writing anything, establish the feature type. If the human hasn't stated it, ask or infer from the description:

- **Quick**: known API surface, 4–8 tasks, build proves success. No unknowns that would invalidate the approach.
- **Phased**: some unknowns or visual subjectivity. Human judgment needed at 1–3 checkpoints.
- **Exploratory**: significant unknowns. The approach itself may change. Spikes required before Phase 1.

When in doubt, default to Phased.

---

## Step 1: Feature Brief (`brief.md`)

Always created first. For Quick features, create the brief and `plan-p1.md` together in one step.

**Location**: `features/active/<feature-name>/brief.md`

Use the template at `templates/brief.md.template`. Key discipline:
- Success criteria must be human-verifiable — no "renders correctly" or "works as expected".
- The unknowns table is required for Phased and Exploratory, optional for Quick.
- Scope exclusions are explicit: name what is not being done.

Choose `<feature-name>` as a short kebab-case slug, e.g. `shield-vfx`, `deferred-rendering`, `enemy-steering`.

---

## Step 2: Exploratory Roadmap (`roadmap.md`) — Exploratory only

Created after the brief is approved. This is the strategic view of the whole journey.

**Location**: `features/active/<feature-name>/roadmap.md`

Use `templates/roadmap.md.template`. Key discipline:
- Name all expected phases upfront, even if their details are uncertain. The point is to have a theory of the full path to the goal before the first spike.
- Every phase must have at least one decision branch. A phase with only "proceed to next" is incomplete.
- Failure modes in decision branches must be named specifically, not generically.
- Spikes go in the Spike Queue with the phase they gate noted.
- The roadmap is annotated as evidence comes in — it is never rewritten retroactively.

After writing the Roadmap, tell the human to invoke Plan Groomer before any spikes begin.

---

## Step 3: Spike Reports (`spike-<name>.md`) — Exploratory only

Written after each spike experiment, before the next phase plan.

**Location**: `features/active/<feature-name>/spike-<name>.md`

Use `templates/spike.md.template`. After writing a spike report:
1. Add a dated annotation to the relevant Roadmap decision point.
2. Confirm the Roadmap branch the spike result selects.

---

## Step 4: Technical Design (`design.md`) — when needed

Optional document for Phased and Exploratory features when the implementation approach needs to be worked out before task decomposition.

**Location**: `features/active/<feature-name>/design.md`

No fixed template. Include: proposed data structures or API additions, module interactions, shader design (if applicable), key algorithms. This document informs Phase Plans but does not replace them.

---

## Step 5: Phase Plan (`plan-pN.md`)

**Gate**: `checkpoint-p(N-1).md` must exist in the feature directory before writing `plan-pN.md`. For Phase 1 the gate is the Feature Brief (human has described/approved it).

**Location**: `features/active/<feature-name>/plan-pN.md`

Use `templates/phase-plan.md.template`. Key discipline:

**Tasks describe outcomes, not implementations.** Write "add vertex-position-to-view-space transform for depth readback" not "call glm::inverse(view) and multiply column 3". The implementing agent decides the how; the plan specifies what must be demonstrably true when the task is done.

**Acceptance criteria are observable.** Each task row's acceptance must be something a human or a build step can verify in under 30 seconds. "Compiles" is acceptable only for pure structural tasks (adding a struct, creating a file). Any task that touches behavior needs a behavioral criterion.

**Parallel tasks must name their files.** Two tasks with `‖` must touch disjoint files. Write the specific files in the task description or acceptance notes.

**The Human Checkpoint ends every phase.** It must include all four fields: what to run, what to look for, what "pass" looks like, and what "stop" means. The stop condition gates the next phase plan.

**Frozen interface changes are GATE tasks.** If the plan needs to modify `docs/interfaces/*`, write a GATE task that says "human approves interface change to X" before any task that depends on the new interface. Do not assume approval.

After writing the Phase Plan, tell the human to invoke Plan Groomer before implementation starts.

---

## Project facts to embed in every plan

Use these when writing task descriptions and acceptance criteria:

- **Shaders**: authored as `.glsl` under `shaders/{renderer,game}/`, compiled by `sokol-shdc` at build time into `build/generated/shaders/<name>.glsl.h`, consumed via `#include`. Never raw shader strings or runtime GLSL.
- **Asset paths**: always composed from the `ASSET_ROOT` macro. Never hardcoded or relative.
- **Graphics**: OpenGL 3.3 Core only. No compute, no tessellation, no geometry shaders, no multi-sample resolve beyond what sokol_gfx exposes on GL 3.3.
- **Renderer owns**: `sokol_app`, pipelines, generic shaders, textures, mesh upload, skybox, line quads.
- **Engine owns**: ECS lifecycle, physics, scene queries, camera matrices, renderer bridge.
- **Game owns**: gameplay systems, weapons, HUD, enemy AI, game-local components, fx shaders.
- **Frozen interfaces**: `docs/interfaces/*-interface-spec.md`. Any change requires explicit human approval before dependents can be implemented.
- **Build targets**: `renderer_app`, `engine_app`, `game`, `renderer_tests`, `engine_tests`. Use the narrowest target that covers the tasks in the phase.

---

## Checkpoint reminder

After a phase executes, the human writes `checkpoint-pN.md` using `templates/checkpoint.md.template`. The Feature Planner does not write checkpoints — only the human does, after actual verification. Agents must not proceed to the next phase plan without this file existing.

---

## Directory creation

When starting a new feature, create the directory:
```
features/active/<feature-name>/
```

No other setup needed — files are created as the workflow progresses.
