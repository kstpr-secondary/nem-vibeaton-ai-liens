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

**Cross-workstream features**: Before drafting the brief for any feature that touches code in two or more workstreams, or that may require changes to `docs/interfaces/*-interface-spec.md`, consult `systems-architect` first. It owns the seam definitions and the frozen interface contracts. A cross-workstream scope cannot be safely drawn without that input.

---

## Step 1: Feature Brief (`brief.md`)

Always created first. For Quick features, create the brief and `plan-p1.md` together in one step.

**Location**: `features/active/<feature-name>/brief.md`

Use the template at `templates/brief.md.template`. Key discipline:
- Success criteria must be human-verifiable — no "renders correctly" or "works as expected".
- The unknowns table is required for Phased and Exploratory, optional for Quick.
- Scope exclusions are explicit: name what is not being done.

Choose `<feature-name>` as a short kebab-case slug, e.g. `shield-vfx`, `deferred-rendering`, `enemy-steering`.

After writing the Brief, tell the human the next step based on feature type — **do not invoke Plan Groomer here; it only reviews Phase Plans and Roadmaps, not Briefs**:
- **Quick**: brief and `plan-p1.md` are written together in one step — then tell the human to invoke Plan Groomer on the plan.
- **Phased**: the Brief is the gate for Phase 1. Tell the human to invoke Feature Planner to write `plan-p1.md`.
- **Exploratory**: Tell the human to invoke Feature Planner to write `roadmap.md`.

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

**When to write it.** The Feature Planner decides. The signal is: if writing task descriptions forces entries like "determine the approach", "figure out the data structure", or "decide which algorithm to use", stop and write `design.md` first. This is especially true when vagueness spans both a data structure definition and the operations over it — that combination cannot be resolved by the implementor without design authority the plan hasn't given them.

---

## Step 5: Phase Plan (`plan-pN.md`)

**Gate**: `checkpoint-p(N-1).md` must exist in the feature directory before writing `plan-pN.md`. For Phase 1 the gate is the Feature Brief (human has described/approved it).

**Location**: `features/active/<feature-name>/plan-pN.md`

Use `templates/phase-plan.md.template`. Key discipline:

**The planner is the architect; the implementor fills in the details.** For Quick and Phased features, task descriptions should sketch the implementation path: name the key functions, methods, or data structures to add or change, specify which algorithm to use, and flag dangerous spots the implementor should watch for (e.g., "sokol_gfx finalizes the pipeline at `sg_make_pipeline` — do not mutate the descriptor after that point"). The implementor resolves call sites, writes the boilerplate, and integrates with existing code. A useful test: a capable implementor who reads the task should not be able to take the wrong architectural approach, but the actual code is theirs to write.

**Algorithm pitfall annotation.** If a task names a geometric or physical algorithm, enumerate the standard degenerate-input cases for that class in the task description before finalizing. These are not optional notes — they become acceptance requirements. A task that names an algorithm but lists no edge cases is incomplete. Common classes to consult:
- Slab intersection (ray vs. AABB, ray vs. hull): parallel ray to a slab face; ray origin exactly on a face plane.
- SAT: near-zero cross-product axes (skip threshold must be stated); full-containment case (overlap formula differs from gap test).
- Convex hull / gift-wrapping: colinear or coplanar candidate set; initial seed-edge validity (arbitrary index pick is fragile; requires a direction-based pivot).
- Euler physics: variable-dt tunnelling; zero-mass bodies in impulse math.
If you are unsure which edge cases apply to an algorithm, treat the task as requiring a spike before the plan is finalized.

**Interface/operation compatibility.** If a task defines a data structure and a later task (or the same task) describes operations over it, cross-check before finalizing: for each described operation, list the data it needs and verify it is present in the struct without per-call reconstruction. Write this check inline in the plan as a brief note ("SAT axis set 3 needs unique edge list — verify ConvexHull.unique_edges exists"). Any gap found here is a plan defect to fix before the plan goes to grooming, not a decision left to the implementor.

For Exploratory phase plans, keep "how" lighter — the approach is still being validated by spikes. The "how" sharpens progressively as unknowns are resolved and each phase plan is written in sequence.

**Acceptance criteria are observable.** Each task row's acceptance must be something a human or a build step can verify in under 30 seconds. "Compiles" is acceptable only for pure structural tasks (adding a struct, creating a file). Any task that touches behavior needs a behavioral criterion.

**The Files column must be exhaustive.** List every file the task modifies — not only files where new code is written, but also existing files that receive call-site additions (new function calls, new includes, new field assignments). A task that says "call X() from renderer_init()" must list renderer.cpp in its Files column even if renderer.cpp is primarily touched by a different task. If the Files column is incomplete, the parallelism check is unreliable and the implementing agent will discover the missing edit without a plan reference.

**Parallel tasks must have strictly disjoint Files.** Two tasks with `‖` must touch no file in common. Apply this after confirming the Files columns are exhaustive — an incomplete Files column can make a conflict invisible.

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

## Checkpoint drafting

After a phase executes, `checkpoint-pN.md` must exist before the next phase plan can be written. The checkpoint records human-observed behavior — the human must have actually run the verification before a checkpoint is produced. The Feature Planner may draft the checkpoint from free-form human feedback, then the human confirms before it is written.

**When the human provides feedback after a phase** (e.g., "all pass but the shield bar flickers on low HP" or "physics task fails — asteroids clip the boundary"):

1. Read the current phase plan's **Human Checkpoint** section (Run / Look for / Pass / Stop).
2. If the human's feedback is ambiguous about outcome (e.g., "it's janky", "mostly works", "not quite right"), ask one explicit follow-up before drafting: "Should this be **PASS** (you want to proceed, with these observations noted) or **STOP** (you want this fixed before the next phase)?" Do not infer Result from tone.
3. Read the human's feedback and map it to the checkpoint fields:
   - **Result**: PASS if all Pass criteria met (minor observations don't block); STOP if any Stop condition triggered.
   - **What was tested**: infer from the plan's "Run" field plus any specifics the human mentioned.
   - **Observations**: the human's verbatim feedback, lightly structured. Include surprises, deviations, and anything that should inform the next phase.
   - **Roadmap impact**: fill only for Exploratory features; leave blank or omit otherwise.
   - **Next step**: infer from Result — PASS → `generate plan-p(N+1).md`; STOP → `stop and re-examine`; final phase → `feature complete → update docs/ and archive`.
4. Draft the full `checkpoint-pN.md` and show it to the human.
5. Ask: *"Does this accurately reflect what you observed? I'll write the file once you confirm."*
6. On confirmation, write `features/active/<feature-name>/checkpoint-pN.md`.

**Do not draft a checkpoint if the human has not run the verification.** If the human asks for the checkpoint without having tested, prompt them to run the Human Checkpoint steps from the plan first.

---

## Revision Mode (NEEDS WORK from Plan Groomer)

When invoked to fix a plan that Plan Groomer returned as NEEDS WORK:

1. Read the existing `plan-pN.md` in full.
2. Read the Groomer's numbered defect list.
3. Address **each numbered defect** in sequence. Edit only what the defect names — do not rewrite sections the Groomer left uncontested.
   - **If Feature Planner judges that a defect should not be addressed** (e.g., the suggested fix requires disproportionate restructuring), **stop**. Do not add a revision note, do not reset the Groomer Verdict, and do not tell the human to re-groom. Instead, present the defect and the rejection rationale to the human and wait for their explicit decision: either implement the fix as the Groomer suggested, or accept the risk and provide a note to embed in the plan. Resume revision only after the human has responded. Feature Planner may not unilaterally decline a groomer defect — doing so silently creates unchecked risk and will cause the same defect to be raised by the next Groomer.
4. Add a revision note below the `Groomer Verdict` field, e.g.: `**Revision**: r2 — addressed groomer defects 1, 3, 5`.
5. Reset `Groomer Verdict` to `PENDING` — the revised plan requires a fresh grooming pass.
6. The plan filename does not change (`plan-pN.md`, not `plan-pN-r2.md`). Git carries the revision history.
7. Tell the human to invoke Plan Groomer again before implementation starts.

Do not rewrite the whole plan. The Groomer's silence on a section is implicit approval of that section.

---

## Directory creation

When starting a new feature, create the directory:
```
features/active/<feature-name>/
```

No other setup needed — files are created as the workflow progresses.
