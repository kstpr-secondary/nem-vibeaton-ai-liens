# Human Supervisor Runbook

> Extracted from AGENTS.md and the Hackathon Master Blueprint. Contains all process, coordination, and escalation content that belongs with human supervisors — not implementing agents. Keep this document accessible to the human team during the hackathon.

---

## 1. Task Claiming Protocol

All task management flows through TASKS.md files in `_coordination/{renderer,engine,game}/`.

### How it works

1. **Human claims a task** by editing TASKS.md: sets `Owner = <FirstName>@<machine>`, updates `Status` to `CLAIMED`, and commits + pushes.
2. The human then triggers the assigned agent (via Claude Code, Copilot CLI, Gemini CLI, etc.) on the appropriate worktree.
3. Agent reads the claimed row and the spec it references before editing code.
4. When done, agent sets `Status = READY_FOR_VALIDATION` (or `READY_FOR_REVIEW` / `DONE` per the task's `Validation` column).

### Owner tag format

| Format | When to use | Example |
|---|---|---|
| `<agent_name>@<machine>` | AI agent implementation | `claude@laptopA`, `copilot-2@rtx3090` |
| `FirstName@machine` | Human manual path (emergency) | `Alice@laptopA` |

- Agent names are lowercase: `claude`, `gemini`, `copilot`, `glm`, `local-qwen`.
- Two instances of the same agent on one machine: append `-2`, `-3` → `claude-2@laptopA`.
- If an agent cannot determine the machine name, leave existing owner intact and add `"assisted by <agent_name>, machine unknown"` to `Notes`.

### Task row schema

| Column | Values | Notes |
|---|---|---|
| `ID` | e.g. `R-14`, `E-07`, `G-03` | Workstream-prefixed unique ID |
| `Task` | Free text description | |
| `Tier` | `LOW` \| `MED` \| `HIGH` | Implementation risk / difficulty |
| `Status` | `TODO` → `CLAIMED` → `IN_PROGRESS` → `READY_FOR_VALIDATION` → `READY_FOR_REVIEW` → `DONE` \| `BLOCKED` | Progress state machine |
| `Owner` | `<agent_name>@<machine>` or `FirstName@machine` | Who is doing the work |
| `Depends_on` | Task ID(s) or empty | Dependencies within/between milestones |
| `Milestone` | Which milestone this belongs to | |
| `Parallel_Group` | `PG-<milestone>-<letter>`, `BOTTLENECK`, or `SEQUENTIAL` | Concurrency classification |
| `Validation` | `NONE` \| `SELF-CHECK` \| `SPEC-VALIDATE` \| `REVIEW` \| `SPEC-VALIDATE + REVIEW` | Required checks before DONE |
| `Notes` | Free text; must include `files: <comma-separated list>` for PG-group tasks | File ownership tracking |

---

## 2. Milestone Merge Protocol

Run this full protocol whenever a milestone includes tasks that touch frozen shared interfaces (`docs/interfaces/`). For intra-workstream milestones only, a lightweight variant suffices.

### Full merge protocol (interface-touching milestones)

1. All milestone tasks marked `DONE` in TASKS.md.
2. `cmake --build build` passes with exit code 0 on the demo machine (`rtx3090`).
3. Smoke tests pass (game launches, basic interaction works).
4. Spec Validator confirms acceptance criteria are met against the spec documents.
5. Human performer a behavioral check — runs the feature interactively and verifies it behaves as described.
6. Code Reviewer performs a lightweight review — scans for obvious risks, bugs, or regressions.
7. Human merges the feature branch → `integration` or `main`.
8. Other workstreams fetch and sync (`git pull`).
9. Mocks are swapped for real implementations where applicable (human flips CMake toggle, verifies build, deletes mock once stable).

### Lightweight merge protocol (intra-workstream, no interface changes)

1. All milestone tasks marked `DONE`.
2. Per-workstream build passes: `cmake --build build --target <target>`.
3. Human spot-checks the feature visually or via smoke test.
4. Push to feature branch; merge to main/integration.

### Milestone-ready definition

A milestone is ready for merge only when **all three** are true:
- Acceptance checklist is fully met (all tasks DONE).
- Required validation is complete per the `Validation` column on every task.
- Human behavioral check is done (interactive verification on demo machine).

### Cadence target

~1 milestone merge per hour per workstream (~5 total per workstream over the hackathon).

---

## 3. Validation and Review Queues

Queues are human-managed artifacts. Agents do not invoke queue tools directly. Humans add entries; agents respect the `Validation` column on their tasks.

### Queue files (all under `_coordination/queues/`)

| File | Purpose | Who manages |
|---|---|---|
| `VALIDATION_QUEUE.md` | Tasks awaiting spec validation | Spec Validator / human |
| `REVIEW_QUEUE.md` | Tasks awaiting code review | Code Reviewer / human |
| `TEST_QUEUE.md` | Tests to be written or run | Test Author / human |

### Risk-based validation levels

| Task Tier | Required Validation | Who performs it |
|---|---|---|
| LOW / trivial | `SELF-CHECK` (unless touching shared interfaces, build system, or milestone-critical behavior) | Implementing agent |
| MEDIUM | `SPEC-VALIDATE` or `REVIEW` for nontrivial logic or integration surfaces | Spec Validator / Code Reviewer |
| HIGH / hard | At least one secondary check before merge | Spec Validator + Code Reviewer |
| Every milestone | Spec Validator + human behavioral check + lightweight Code Review | All three roles |

### Testing scope

- **Catch2** for math, parsers, ECS logic — unit-testable behavior.
- **Rendering correctness** verified via human behavioral check + smoke-test visuals, NOT unit tests. Don't waste time writing unit tests for rendering pipeline correctness.

---

## 4. AI Tool Priority and Escalation

This is a **human supervisor decision framework**. Agents do not orchestrate other agents.

### Tool priority (use highest-priority available tool first)

1. **Claude Code Sonnet** — primary for hard/ambiguous tasks
2. **GitHub Copilot** (agent mode / CLI) — primary backup; medium-task worker
3. **Gemini CLI** — validation, review, research, overflow
4. **z.ai GLM 5.1** — backup for Claude-heavy work; parallel hard-task executor in second half
5. **Local Qwen3.6-35B-A3B on RTX 3090** — tests for one file, one shader/diff review, one checklist with trimmed prompt, small interface checks. Treat as small-context, bounded-output even if configured for 64K.
6. **Claude Opus** — escalation-only for thorny math/architecture

### Escalation triggers

| Trigger | Action |
|---|---|
| Agent stalls >90 seconds | Switch to next priority tier; log in `_coordination/overview/BLOCKERS.md` |
| Agent fails a task twice | Switch to emergency manual path: `FirstName@machine` |
| Agent is >30 min behind milestone | Switch to emergency manual path: `FirstName@machine` |

### Secondary-agent dispatch (RTX 3090 and overflow)

- Hard graphics/math/systems → Claude Sonnet first; GLM 5.1 or Claude Opus as needed.
- Standard implementation → Copilot first; Claude or Gemini as backup.
- Tests / narrow spec check / shader check / short diff review → local RTX 3090 first if idle; Gemini if prompt is large or local box busy.
- Milestone validation across several files → Gemini, GLM 5.1, or Claude Sonnet; local model only with aggressively trimmed prompts.

---

## 5. Scope-Cut Contingency Order

If time runs short during the hackathon, cut features in this order (do not improvise):

1. Blinn-Phong shading (fallback to Lambertian only)
2. Diffuse textures (solid colors only)
3. Custom shader hook / shader-based explosion VFX
4. Enemy AI steering (game-local seek only; no engine E-M5 swap)
5. Multiple enemy ships (keep 1)
6. Asteroid-asteroid collisions
7. Capsule mesh builder

### Finalization timeline

| Time | Action |
|---|---|
| **T-30 min** | Feature Freeze — no new feature merges to `main`; only bugfixes, asset/shader tweaks, demo stabilization |
| **T-10 min** | Branch Freeze — no further merges; all focus on `rtx3090` |
| **Final 60 min** | Demo machine (`rtx3090`) kept synced with `main` |

### Fallback strategy

If a workstream is broken at T-30, re-enable its mocks on `main` so the game still launches for demo.

---

## 6. Source Control Protocol

### Branches

```
main  /  integration  /  feature/renderer  /  feature/engine  /  feature/game
```

### Worktrees per machine

```
hackathon/  hackathon-renderer/  hackathon-engine/  hackathon-game/
```

### Rules

- `main` stays demo-safe at all times.
- `integration` is optional staging.
- Multiple agents may work in the same worktree/branch simultaneously only if their file sets are disjoint (per parallel group declarations).
- Always `git pull` before starting new work when multiple people/machines touch the same workstream.

---

## 7. Parallel Group File Ownership

When tasks share a `Parallel_Group` (e.g., `PG-M3-A`), all agents must verify their file sets are disjoint before starting.

### Violation resolution

If an agent discovers it needs a file outside its declared set:

1. **Pause.** Do not edit the file yet.
2. Update the file set in TASKS.md (edit the `Notes` column).
3. Check whether the new file is claimed by another task in the same parallel group.
4. If yes: flag in `_coordination/overview/BLOCKERS.md` and wait for human resolution. Do NOT race.

### Bottleneck tasks

`BOTTLENECK` tasks block every dependent until merged. While a BOTTLENECK is open, idle agents should pull work from other milestones or workstreams (write tests, add validation queue entries, update docs). Use `BOTTLENECK` sparingly.

---

## 8. Quality Gates

| Gate | Rule | Enforcer | Timing |
|---|---|---|---|
| **G1** | `cmake --build` returns 0 | Implementing agent | Before DONE |
| **G2** | Relevant tests pass | Implementing agent / Test Author | Before DONE where applicable |
| **G3** | Frozen interfaces unchanged without approval | Human + Spec Validator | Continuous |
| **G4** | Required validation completed | Spec Validator / Code Reviewer | Before merge when required |
| **G5** | Milestone-level validation completed | Spec Validator + human + reviewer | Before milestone merge |
| **G6** | `main` remains demo-safe | Humans | Continuous |
| **G7** | Parallel group file sets remain disjoint | Task author (pre-claim) + claiming agent (pre-edit) | Before task claim |

---

## 9. Mock-to-Real Swap Protocol

Mock implementations live in `src/<workstream>/mocks/` and are toggled via CMake options (e.g., `-DUSE_RENDERER_MOCKS=ON`).

### When to swap

After a milestone merge that delivers the real implementation, downstream workstreams swap mocks for real code.

### Steps

1. Human flips the CMake toggle (`-DUSE_<WORKSTREAM>_MOCKS=OFF`).
2. Human verifies the full build passes: `cmake --build build`.
3. Human runs smoke tests on the game executable.
4. Once stable, human deletes the mock files from `src/<workstream>/mocks/`.

---

## 10. Agent Outage and Emergency Manual Path

### Detection

An agent is considered "outaged" when:
- It stalls for >90 seconds on a task (detectable by wall-clock timing).
- It fails to produce working code after two attempts on the same task.
- It falls >30 minutes behind its milestone deadline.

### Actions

1. **Stall >90s:** Log in `_coordination/overview/BLOCKERS.md`, switch to next priority tool tier, and re-trigger.
2. **Two failures or >30 min behind:** Take over manually — `Owner` becomes `FirstName@machine`. The human writes the code directly in emergency-only mode.

---

## 11. Pre-Hackathon Checklist (for Human Team)

### Infrastructure
- [ ] Ubuntu 24.04 LTS verified on all machines
- [ ] OpenGL 3.3 Core backend verified on all laptops
- [ ] Agreed hostnames set in `/etc/hostname`
- [ ] Clang, CMake, Ninja installed and tested
- [ ] Repo and worktrees created on all machines including RTX 3090
- [ ] FetchContent cache warmed on all machines
- [ ] Asset and shader runtime paths configured and verified
- [ ] Build baseline opens a Linux window and clears the screen
- [ ] `quickstart.md` / runbook created and tested

### Agent Configuration
- [ ] AGENTS.md finalized with trimmed content
- [ ] CLAUDE.md imports AGENTS.md; contains only Claude-specific notes
- [ ] `.github/copilot-instructions.md` mirrors critical rules
- [ ] `.gemini/settings.json` context file points to AGENTS.md
- [ ] Role and domain skills generated and reviewed
- [ ] Copilot, Claude, Gemini, and z.ai workflows verified on real repo structure

### Coordination Files
- [ ] `docs/` vs `_coordination/` split implemented
- [ ] Per-workstream TASKS.md with parallel groups and file sets
- [ ] MASTER_TASKS.md populated after cross-workstream synthesis
- [ ] Per-workstream VALIDATION/ and REVIEWS/ directories created
- [ ] Milestone Groups and Milestones with Expected Outcome statements

---

## 12. Fixed Decisions (Cumulative)

These are project-level decisions that the human supervisor must enforce. Agents should not question them; if an agent encounters a conflict, flag it as a blocker.

1. OpenGL 3.3 Core only — Vulkan removed.
2. Shaders precompiled via `sokol-shdc` at build time — no runtime GLSL loading or hot-reload.
3. Asset paths use `ASSET_ROOT` macro from generated `paths.h` — never hard-code relative paths.
4. Alpha blending (basic) is MVP in renderer.
5. Capsule mesh builder is Desirable, not MVP. Engine uses sphere + cube only for MVP.
6. CMakeLists.txt ownership: Renderer workstream / Systems Architect. Cross-workstream build changes require 2-minute notice to other workstreams.
7. `_coordination/` (operational state) vs `docs/` (reference material) split is fixed — never edit docs/ during hackathon except for bugfixes.
