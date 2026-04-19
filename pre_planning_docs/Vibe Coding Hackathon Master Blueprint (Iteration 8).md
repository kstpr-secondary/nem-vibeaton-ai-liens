# Vibe Coding Hackathon — Master Blueprint (Iteration 8)

> **Document purpose:** This is the authoritative reference document for the hackathon. It is written to be read by both human team members and AI agents (Systems Architect, role specialists). It describes the project context, the agentic setup, the workflow, the source-control protocol, and the pre-hackathon preparation required to maximize delivery inside a 5–6 hour coding window. It is **not** the full technical specification of the rendering engine, game engine, or game; those live in separate documents. This revision focuses on: (a) task and milestone granularity that supports parallel multi-agent work across multiple machines, (b) SpecKit planning scope per workstream vs whole project, (c) library reference doc placement and granularity, and (d) agent ID convention refinement.

> **Status:** Living document. Approved propositions from iterations 1–7 are marked **FIXED**. New decisions made in iteration 8 are marked **FIXED (Iter 8)**.

***

## 1. Project Concept

### 1.1 What We Are Building

A team of three graphics programmers will use AI agents to build three interconnected C++ projects from scratch in a single 5–6 hour hackathon window:

1. **A custom real-time 3D rendering engine** — forward rendering, physically plausible shading, skybox, textures, line rendering. Built on top of `sokol_gfx`.
2. **A custom game engine** — ECS-based scene management, game loop, physics, input, asset loading. Built on top of the rendering engine's public API.
3. **A playable 3D space shooter game** — asteroids, player-controlled spaceship, enemy AI, weapons, shields/hit points, containment field. Built on top of the game engine's public API.

The three projects live in one repository, in separate directories, and are integrated progressively at milestone boundaries rather than continuously. Each workstream begins against frozen interface contracts and, where necessary, mock implementations. When a milestone reaches its acceptance criteria, it becomes the synchronization point for downstream workstreams.

### 1.2 The Point of the Exercise

The goal is a **live demonstration**: every line of C++ implementation code is generated during the hackathon by AI agents under human supervision, producing a playable and visually convincing game within 5 hours.

**Speed over quality.** This project is optimized for visible progress and demo reliability, not long-term maintainability. Behavioral correctness, milestone predictability, and integration discipline take precedence over code elegance, architecture purity, and post-event extensibility.

### 1.3 The Human Team

Three people, all expert Unity 3D / C\# / 3D math programmers with strong shader and rendering knowledge but rusty C++ syntax knowledge.

Each person owns one primary workstream:

- **Person A** — Rendering Engine
- **Person B** — Game Engine
- **Person C** — Game

Humans supervise sessions, approve interface changes, run behavioral checks, and manage git synchronization at milestone boundaries. Manual implementation coding is allowed only as an emergency path to protect the demo.

***

## 2. Hardware and Platform Configuration

### 2.1 Final Platform Decision

**FIXED:** The team will use a **Ubuntu-only configuration**: three laptops with Ubuntu 24.04 LTS plus one Ubuntu 24.04 LTS RTX 3090 desktop machine.


| Machine | Owner | Primary Role | Notes |
| :-- | :-- | :-- | :-- |
| Laptop A | Person A | Rendering engine workstream | Ubuntu 24.04 LTS |
| Laptop B | Person B | Game engine workstream | Ubuntu 24.04 LTS |
| Laptop C | Person C | Game workstream | Ubuntu 24.04 LTS |
| RTX 3090 PC | Shared | Local agent, validation, tests | Ubuntu 24.04 LTS |

### 2.2 Graphics and Build Baseline

**FIXED:** Primary graphics target is **desktop Linux**. The stack uses **CMake + Ninja + Clang** on all machines. The rendering abstraction remains `sokol_gfx`; implementation optimizes for Linux and avoids unnecessary platform branching.

### 2.3 Shared Local Agent Machine

The RTX 3090 machine is a shared service node, not a primary implementation machine. It handles bounded tasks such as focused review jobs, small test-generation jobs, short validation passes, and overflow support when cloud models are saturated. It must not sit on the critical path for routine implementation.

***

## 3. Technology Stack

All items below are **FIXED** unless explicitly marked otherwise.

### 3.1 Core Libraries

| Library | Role | Integration | Version |
| :-- | :-- | :-- | :-- |
| **sokol_gfx** | Graphics abstraction | FetchContent | pinned |
| **sokol_app** | Window + event loop | FetchContent | same pin as gfx |
| **sokol_time** | High-res timer | FetchContent | same pin as gfx |
| **entt** | Entity Component System | FetchContent | v3.x pinned |
| **cgltf** | glTF loading | FetchContent | pinned |
| **tinyobjloader** | OBJ fallback loader | FetchContent | pinned |
| **GLM** | 3D mathematics | FetchContent | 1.x pinned |
| **Catch2 v3** | Unit testing framework | FetchContent | v3.x pinned |
| **Dear ImGui** | HUD / debug overlay | FetchContent | pinned |

### 3.2 Language and Build

- Language: **C++17**.
- Build: **CMake + Ninja** on all machines.
- Dependencies: **FetchContent only**; no Conan or vcpkg.
- Explicit cuts: no audio, networking, skeletal animation, particle system, or editor tooling.


### 3.3 Shader Pipeline

**FIXED:** Shader code is generated during the hackathon in `.glsl` files under `/shaders/`. Runtime file loading is the default path; `sokol-shdc` is a stretch goal only if tooling friction is negligible.

### 3.4 Assets and Format Policy

- Assets are pre-verified for integrity, format, licensing, and suitability.
- Prefer **glTF/GLB** for main meshes and use OBJ only as a fallback via tiny loader.
- Keep MVP to at most two mesh formats.

***

## 4. Rendering Engine — High-Level Scope

> Full milestone details live in the separate Rendering Engine Spec document.

**MVP (To be distributed into milestones):** perspective camera, forward rendering, unlit → Lambertian → Blinn-Phong shading, diffuse textures, static skybox, line renderer for laser beams, simple procedural geometry, runtime GLSL loading, minimal shader-error reporting.

**Desirable (To be distributed into milestones):** alpha blending queues, normal maps, simple GPU instancing if easy, limited material hooks for gameplay effects.

**Stretch/Cut:** shadow mapping, PBR, deferred rendering, post-processing, complex material graphs.

***

## 5. Game Engine — High-Level Scope

> Full milestone details live in the separate Game Engine Spec document.

**MVP (To be distributed into milestones):** ECS scene graph using `entt`, asset loading and mesh upload bridge, game loop synchronized with renderer, input handling, Euler physics, simple collision, raycasting, thin gameplay-facing API.

**Desirable:** simple enemy steering, multiple lights, prefab-style spawning helpers.

**Stretch/Cut:** pathfinding, animation system, audio, networking, editor tooling.

***

## 6. Game — High-Level Scope

> Full MVP and design details live in `GAME_DESIGN.md` and workstream-local game design notes.

**MVP:** procedural asteroid field, player spaceship, enemy ships, plasma gun, laser beam, shields and hit points, containment boundary, Dear ImGui HUD.

**Stretch/Cut from MVP:** rocket launcher, power-ups, advanced VFX, mobile controls, station regeneration mechanics.

The game workstream starts from frozen engine and renderer interfaces plus mocks and must not block waiting for full engines.

***

## 7. AI Agent Stack

### 7.1 Agent Roles

Roles separate **implementation**, **spec validation**, **test generation**, and **code review**.


| Role | SKILL.md File | Activated By | Primary Responsibility |
| :-- | :-- | :-- | :-- |
| Systems Architect | `systems-architect/SKILL.md` | Human explicitly | Architecture, cross-workstream plans |
| Renderer Specialist | `renderer-specialist/SKILL.md` | Renderer worktree session | Rendering engine implementation |
| Engine Specialist | `engine-specialist/SKILL.md` | Engine worktree session | Game engine implementation |
| Game Developer | `game-developer/SKILL.md` | Game worktree session | Game implementation |
| Test Author | `test-author/SKILL.md` | Human or queue | Catch2 tests, smoke tests |
| Spec Validator | `spec-validator/SKILL.md` | Human or queue | Spec adherence checks |
| Code Reviewer | `code-reviewer/SKILL.md` | Human on diff / PR | Risk and bug review |

### 7.2 Role Responsibility Boundaries

| Role | Scope | Output | When Used |
| :-- | :-- | :-- | :-- |
| Test Author | Create/update tests | Test files | Before / during implementation |
| Spec Validator | Check code vs spec/task/interface | Note | Before milestone merge, risky tasks |
| Code Reviewer | Review diff for risk and obvious bugs | Note | Before PR or milestone merge |

The validator answers **"Did this implement the right thing?"**; the reviewer answers **"Is this obviously risky or broken?"**.

### 7.3 Trigger Model

Workflow is **task-queue driven**, not hook-driven. Any coding tool can implement tasks; task state lives in coordination files.

### 7.4 AI Tool Priority and Escalation

**FIXED priority stack:**

1. Claude Code Sonnet — primary implementation agent for hard or ambiguous tasks.
2. GitHub Copilot agent mode / CLI — primary backup implementation agent and medium-task worker.
3. Gemini CLI — validation, review, research, overflow implementation.
4. z.ai GLM 5.1 — backup for Claude-heavy work and parallel hard-task executor in second half of hackathon.
5. Local GLM-4.7-Flash on RTX 3090 — auxiliary worker for tests, spec checks, shader review, small-file analysis.
6. Claude Opus — escalation-only for mathematically or architecturally thorny tasks.

### 7.5 Local Agent Role and Limits

The RTX 3090 model is a **shared burst-capacity assistant**. Use it for:

- Tests for one file or subsystem slice.
- One shader or small diff review.
- One task or milestone checklist with trimmed prompt.
- Small interface consistency checks.

Treat it as a **small-context, bounded-output assistant** even if configured for 64K context; prompts should remain narrow.

### 7.6 Local Agent Worktree Strategy

The RTX 3090 PC keeps one clone plus worktrees mirroring the laptops. Validation, review, and test tasks target a **specific worktree/branch** named in the task entry. It mostly inspects committed or locally saved state, not live in-progress edits.

***

## 8. Instruction Files, Skills, and Large-Header Handling

### 8.1 Design Principles

Instruction and skills design must satisfy:

1. **Cross-tool compatibility** — a single root for behavior across tools.
2. **Conciseness** — small root instructions to protect context budgets.
3. **Progressive disclosure** — specialized knowledge loaded only on demand.
4. **Stable locations** — humans and agents always know where to find rules, specs, tasks, validation notes, and skills.

### 8.2 Canonical Root Files

**FIXED:** `AGENTS.md` is the canonical cross-tool root behavior file.

- `AGENTS.md`: short global rules about task claiming, milestones, validation, interface changes, and queue usage.
- `CLAUDE.md`: imports `AGENTS.md` (via tool-specific include syntax, `@AGENTS.md`) and adds only Claude-specific notes.
- `.github/copilot-instructions.md`: short mirror of the critical rules so Copilot sees them without depending on linked-file auto-loading.
- `.gemini/settings.json`: configured to point at `AGENTS.md` as context file — `"contextFileName": "AGENTS.md"`.

Key simplifications:

- `AGENTS.md` contains only rules that **must** be globally consistent across tools and workstreams.
- Anything workstream-specific or library-specific moves into SKILL files instead of bloating `AGENTS.md` or `.github/copilot-instructions.md`.


### 8.3 Repository Structure (High Level)

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
│   └── planning/
│       └── speckit/
│           ├── renderer/
│           ├── engine/
│           └── game/
├── _coordination/
│   ├── overview/
│   │   ├── PROGRESS.md
│   │   ├── BLOCKERS.md
│   │   ├── MERGE_QUEUE.md
│   │   └── MASTER_TASKS.md
│   ├── renderer/
│   │   ├── TASKS.md
│   │   ├── PROGRESS.md
│   │   ├── VALIDATION/
│   │   └── REVIEWS/
│   ├── engine/
│   │   ├── TASKS.md
│   │   ├── PROGRESS.md
│   │   ├── VALIDATION/
│   │   └── REVIEWS/
│   ├── game/
│   │   ├── TASKS.md
│   │   ├── PROGRESS.md
│   │   ├── VALIDATION/
│   │   └── REVIEWS/
│   └── queues/
│       ├── VALIDATION_QUEUE.md
│       ├── REVIEW_QUEUE.md
│       └── TEST_QUEUE.md
├── shaders/
└── src/...
```

- `docs/` holds **stable reference**: architecture, interfaces, design, planning artifacts.
- `_coordination/` holds **live operational state**: tasks, progress, blockers, merge queue, validation and review notes.
- `.agents/skills/` holds reusable **skills and distilled APIs** to avoid constantly loading large headers.


### 8.4 `_coordination` Contents and Task Files

- `_coordination/overview/`: `PROGRESS.md`, `BLOCKERS.md`, `MERGE_QUEUE.md`, `MASTER_TASKS.md`.
- `_coordination/{renderer,engine,game}/`: each has `TASKS.md`, `PROGRESS.md`, and `VALIDATION/`, `REVIEWS/` directories.
- Queue files under `_coordination/queues/`: `VALIDATION_QUEUE.md`, `REVIEW_QUEUE.md`, `TEST_QUEUE.md`.

Per-workstream `TASKS.md` is the **operational source of truth** for that stream; `MASTER_TASKS.md` is a compact integration dashboard listing only milestones and cross-stream dependencies.

A task row keeps the existing schema:

```markdown
| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
```

`Validation` is one of `NONE`, `SELF-CHECK`, `SPEC-VALIDATE`, `REVIEW`, `SPEC-VALIDATE + REVIEW`.

`Parallel_Group` is explained in section 10.

### 8.5 AGENTS.md Hard Rules (Compacted)

`AGENTS.md` is kept deliberately short. It should contain at most:

1. Before editing code, read the relevant workstream `TASKS.md` and any referenced spec for that task.
2. Claiming a task means **editing `TASKS.md`, committing that change, and pushing it** when any other human or machine may touch that workstream.
3. Do not change frozen shared interfaces without explicit human approval.
4. Every implementation task should reference which milestone it unlocks or depends on.
5. Respect the `Validation` column; do not silently downgrade required checks.
6. Milestone-ready work is not merged until (a) acceptance checklist is met, (b) required validation is complete, and (c) a human has performed a behavioral check.
7. Review and validation are triggered via queue files, not hidden hooks.
8. When multiple people or machines touch the same workstream, pull before starting new work and after any remote task-claim or milestone-sync commit.
9. When working with a major library that has a dedicated SKILL, load and follow the SKILL first, and only read minimal header snippets when the SKILL's distilled API is insufficient.
10. Claim a task by setting `Owner = <agent_name>@<machine>` (e.g., `claude@laptopA`, `gemini@laptopC`). If you cannot determine the machine name, do not overwrite a populated owner; add a note in the `Notes` column instead.

Everything more specific (library usage, workstream details, agent examples) lives in SKILLs or architecture/interface docs.

### 8.6 Validation and Review Granularity (Risk-Based)

- **Low / trivial tasks:** self-check unless they touch shared interfaces, build system, or milestone-critical behavior.
- **Medium tasks:** spec validation or review when they modify nontrivial logic or integration surfaces.
- **High / hard tasks:** at least one secondary check before merge.
- **Every milestone:** always gets milestone-level validation (spec validator + human behavior check + lightweight review) before merge.

The **milestone** is the mandatory backstop; task checks are risk-based.

### 8.7 Architecture and Spec Documents

- `docs/architecture/ARCHITECTURE.md`: integrated view of renderer, engine, game, and their dependencies.
- `renderer-architecture.md`, `engine-architecture.md`, `game-architecture.md`: workstream-local architecture.
- `docs/interfaces/INTERFACE_SPEC.md`: shared contracts.
- Workstream-local interface specs as needed.

Agents working within a workstream prefer local architecture/spec files plus the relevant excerpt from the integrated documents.

### 8.8 SpecKit Planning Scope — Per Workstream

**FIXED (Iter 8):** SpecKit planning is executed **once per workstream**, not once for the whole project.

**Rationale:** Running a single SpecKit pass over all three workstreams at once risks scope explosion that derails the planning phase. Each workstream (renderer, engine, game) is a genuinely distinct technical domain with its own task graph, dependency surface, and milestone cadence. Three separate passes keep each plan focused and tractable.

**Maintaining the big picture:** The risk of losing cross-workstream coherence is real and must be explicitly managed:

- The **Systems Architect agent** runs a short cross-workstream synthesis pass *after* all three individual SpecKit plans are complete. This pass reads the three `tasks.md` outputs side-by-side and produces or updates `MASTER_TASKS.md` with cross-stream dependencies and integration milestones.
- The cross-workstream interface contracts (`docs/interfaces/INTERFACE_SPEC.md`) must be frozen *before* individual SpecKit passes begin, so that each plan is already aware of boundary conditions.
- Any task or milestone in one workstream that touches a frozen interface must be flagged with `Depends_on` referencing the relevant interface spec version.

**SpecKit outputs per workstream** are stored under `docs/planning/speckit/{renderer,engine,game}/` and translated into the stable coordination schema as per section 8.9 below.

### 8.9 SpecKit Outputs and Synthesis

SpecKit-style planning remains **precursor**; outputs are mapped into the stable schema in a short synthesis step:

- `plan.md` / `research.md` → architecture and planning notes under `docs/architecture/` or `docs/planning/speckit/<workstream>/`.
- `contracts/` → `docs/interfaces/INTERFACE_SPEC.md` and workstream-local interface specs.
- `tasks.md` → per-workstream `_coordination/.../TASKS.md` plus a compact `MASTER_TASKS.md`.
- `quickstart.md` → optional setup/runbook doc.

SpecKit's directory layout is not used directly for live coordination; it is translated once and then treated as read-only planning context.

### 8.10 Skills and Large-Header Libraries (sokol, entt, cgltf, etc.)

**Problem:** single-header or header-heavy libraries (e.g., `sokol_gfx.h`, `entt.hpp`, `cgltf.h`) can be 20–30k LOC. Naively loading them in every session wastes context and can degrade model behavior.

**Strategy:** use a **Skill + per-aspect reference files** pattern:

1. **Library Skill (`.agents/skills/<lib>-api/SKILL.md`):**
    - Contains a short description of what the library does in this project, project-specific rules of use (e.g., "prefer these functions", "do not touch these macros"), and a distilled API cheat-sheet (key public types and function signatures needed for this project only, each with a short description).
    - Does **not** embed the full header; it is curated by humans or pre-hackathon agent passes and kept under a strict size budget.
2. **Per-Aspect Reference Files (`.agents/skills/<lib>-api/references/<lib>-<aspect>.md`):**
    - **FIXED (Iter 8):** Library reference material lives under `.agents/skills/<lib>-api/references/`, **not** under `docs/interfaces/`. The rationale is that these files are agent-consumption artifacts (usage guides + API howtos for a specific library), not project interface contracts. Keeping them co-located with the SKILL they extend makes navigation unambiguous.
    - Each file covers one narrow operational concern. The granularity is intentionally fine so agents load only what they need. Example breakdown for `sokol-api`:
        - `sokol-api-work-with-descriptors.md` — struct descriptor patterns (`sg_buffer_desc`, `sg_pipeline_desc`, etc.)
        - `sokol-api-work-with-buffers.md` — vertex/index buffer creation, update, destruction lifecycle
        - `sokol-api-render-passes.md` — `sg_begin_pass`, `sg_end_pass`, attachments, `sg_pass_action`
        - `sokol-api-draw-calls.md` — `sg_apply_pipeline`, `sg_apply_bindings`, `sg_apply_uniforms`, `sg_draw`
        - `sokol-api-compute-dispatch.md` — compute pipeline setup and dispatch (stretch goal path)
    - Each reference file contains: a one-paragraph "when to use this file" header, the minimal relevant API signatures, and 1–2 short concrete usage patterns. It deliberately avoids duplicating material from SKILL.md.
    - The SKILL.md file references these aspect files explicitly in a "Deeper references" section so agents know they exist and when to reach for them.
3. **Header Usage Rules:**
    - For implementation tasks, the default is: **use the SKILL first**, then load the relevant aspect reference if the task clearly falls in that domain.
    - Only when the task explicitly mentions new or advanced functionality not covered by any aspect reference, or an error message references a symbol not in the skill, may the agent open the actual header and quote the **minimal necessary portion** (the relevant struct and 2–3 functions) into context.
4. **Pre-Hackathon Distillation:**
    - The Systems Architect (or a dedicated prep session) generates distilled API lists and aspect reference files for `sokol_gfx`, `entt`, `cgltf`, and other heavy headers.
    - The result is checked by humans for correctness and kept under a strict token budget suitable for frequent loading.
5. **Skill-First Policy:** See `AGENTS.md` rule 9 in section 8.5.

### 8.11 Lazy / Progressive Skill Loading

- Role skills (`systems-architect`, `renderer-specialist`, etc.) are **role-specific** and can assume the agent is working in that worktree.
- Library/domain skills (`sokol-api`, `entt-ecs`, `cgltf-loading`, `glsl-patterns`, `physics-euler`) are **loaded only when the task requires that domain**.
- When starting a task:

1. Read the relevant `TASKS.md` row.
2. Load the role SKILL.
3. Load only the necessary library/domain SKILLs for the subsystems mentioned in the task.
4. Load the relevant per-aspect reference files from `.agents/skills/<lib>/references/` if the task clearly falls in that aspect.
- This keeps prompts tight while still giving the agent enough guidance.


### 8.12 Task Claiming and Agent ID Convention

**FIXED (Iter 8):** Task ownership uses the `agent_name@machine` convention, not `person@machine`.

**Rationale:** A human supervising a session may run multiple different agents (Claude Code, Gemini CLI, Copilot agent mode) on the same machine. The previous `person@machine` scheme could not distinguish which agent holds a task claim, making it impossible to know whether another agent on the same machine can safely start work in the same files.

**Convention:**

- `Owner` column in `TASKS.md` is a string of the form `<agent_name>@<machine>`.
- `agent_name` is the lowercase tool/model name, e.g. `claude`, `gemini`, `copilot`, `glm`, `local-glm`.
- `machine` is a short stable hostname agreed by the team before the hackathon (see below).

**Examples:**

```
claude@laptopA
gemini@laptopC
copilot@laptopB
glm@laptopA
local-glm@rtx3090
```

- It is assumed, and is good practice, that no team member runs two instances of the **same** agent on one machine simultaneously. If this changes, append a numeric suffix: `claude-2@laptopA`.

- For human-initiated tasks where a person is typing directly (emergency manual path), use `<FirstName>@<machine>`, e.g. `Alice@laptopA`.


**Machine names:**

- **FIXED (Iter 8):** Machine hostnames are set once during pre-hackathon infrastructure setup and recorded in `AGENTS.md` in a short table. The table maps each hostname to its human owner and primary role. All machines must have their `/etc/hostname` set to the agreed name before the hackathon.
- Agents that genuinely cannot determine the machine name (e.g., a sandboxed environment) must **not** overwrite a populated `Owner` field. They should add a note in `Notes` such as `"assisted by <agent_name>, machine unknown"` and leave the current owner unchanged.

**Machine hostname table (to be completed during pre-hackathon setup):**


| Hostname | Owner | Primary Role |
| :-- | :-- | :-- |
| `laptopA` | Person A | Rendering engine workstream |
| `laptopB` | Person B | Game engine workstream |
| `laptopC` | Person C | Game workstream |
| `rtx3090` | Shared | Validation, tests, local model |


***

## 9. Source Control Structure

### 9.1 Repository Layout and Branching

One repository with:

```text
main
integration
feature/renderer
feature/engine
feature/game
```

Worktrees:

```text
hackathon/
hackathon-renderer/
hackathon-engine/
hackathon-game/
```

`main` remains always demoable; `integration` is optional as a short-lived integration branch when time allows.

### 9.2 Milestones as Sync Points and Protocol

Milestones are explicit merge-and-sync boundaries between workstreams:

1. Owning workstream marks all milestone tasks done.
2. Build passes and smoke tests pass.
3. Spec Validator confirms acceptance criteria.
4. Human performs behavioral check.
5. Code Reviewer performs a lightweight review.
6. Human merges feature branch to `integration` or `main`.
7. Other workstreams fetch and sync.
8. Mocks are swapped for real implementations where applicable.

Workstream reassignment can happen after stable milestone merge or explicit human override.

***

## 10. Task and Milestone Structure for Parallel Multi-Agent Work

**FIXED (Iter 8).** This section supersedes the "one cpp file/shader per task" guidance from iteration 6 and the flat milestone structure from earlier iterations.

### 10.1 The Parallelism Problem

With up to four machines and multiple agents per machine, multiple agents may be active in the same workstream simultaneously. File-level collisions are a real risk. The previous flat milestone list and single-priority-tier task schema are insufficient because:

- Priority tiers (HIGH/MEDIUM/LOW) communicate urgency but say nothing about which tasks can run concurrently.
- The "one file per task" rule is often violated by real tasks that span a header, a source file, and a shader.
- Milestone boundaries are sync points; too many of them create a constant integration tax, too few allow divergence to build up.


### 10.2 Two-Level Task Structure: Milestone Groups and Milestones

**FIXED (Iter 8):** The task structure uses **Milestone Groups** as broad thematic phases and **Milestones** as sync points within them.

- A **Milestone Group** is a logical cluster of related milestones (e.g., "Shading", "Scene Management", "Gameplay Core"). It carries no mandatory integration event; it is purely an organizational label that makes the task file readable.
- A **Milestone** is a formal, tested sync point. Its tasks must all be complete and their acceptance criteria met before a milestone merge is triggered. Milestones carry an `Expected Outcome` statement describing the observable result.
- A **Parallel Group** is a set of tasks *within a milestone* that touch non-overlapping files and can therefore be claimed and implemented concurrently by different agents. Parallel groups are labeled `PG-<milestone_id>-<letter>` (e.g., `PG-M3-A`, `PG-M3-B`).
- A **Bottleneck Task** is a task that must be completed exclusively by one agent before any task in a dependent milestone or parallel group may start. It is marked `BOTTLENECK` in the task row and acts as a gate.


### 10.3 Task Schema (Extended)

```markdown
| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
```

`Parallel_Group` is either:

- A parallel group label such as `PG-M3-A` — this task may run concurrently with other tasks in the same group.
- `BOTTLENECK` — this task must complete and be merged before tasks that depend on it may start.
- `SEQUENTIAL` — this task must run after its `Depends_on` tasks but has no concurrency constraint with tasks outside its dependency chain (default when not parallel-ready).


### 10.4 File-Ownership Rule for Parallel Groups

**FIXED (Iter 8):** When defining a parallel group, the task author (Systems Architect or human) must enumerate the **file set** touched by each task in that group. Two tasks may only be placed in the same parallel group if their file sets are **disjoint** (no shared files).

- File sets are listed in the task's `Notes` column: `files: renderer.cpp, renderer.h`.
- If during implementation an agent discovers it must touch a file outside the declared file set, it must:

1. Pause.
2. Update the task's file set in `TASKS.md`.
3. Check whether the new file is claimed by another task in the same parallel group.
4. If yes, flag a blocker in `BLOCKERS.md` and wait for human resolution before proceeding.


### 10.5 Bottleneck Tasks and Agent Idling

When one agent holds a BOTTLENECK task and others cannot proceed:

- Idle agents should pull work from **other milestones or workstreams** that have no dependency on the bottleneck. Cross-workstream overflow is acceptable.
- Secondary tasks include: writing tests for already-completed tasks, adding validation queue entries, updating documentation, or preparing aspect reference files.
- The BOTTLENECK convention is used sparingly; most tasks should be parallelizable or sequential rather than true bottlenecks.


### 10.6 Worked Example — Renderer Shading Milestone Group

```markdown
## Milestone Group — Shading

### Milestone M2 — Frustum Culling
Bottleneck: NO  
Parallel Groups: PG-M2-A (all tasks are in one coherent unit for this milestone)

Tasks:
| ID   | Task                                                  | Tier | PG       | files                                      |
|------|-------------------------------------------------------|------|----------|--------------------------------------------|
| R-10 | Compute frustum planes from camera/projection matrix  | MED  | PG-M2-A  | frustum.cpp, frustum.h                     |
| R-11 | AABB vs frustum test for all scene objects            | MED  | PG-M2-A  | frustum.cpp (extends R-10 — SEQUENTIAL)    |
| R-12 | Filter draw list to culled objects only               | MED  | PG-M2-A  | renderer.cpp (post R-11 — SEQUENTIAL)      |

Expected outcome: In a procedural scene, disabling frustum culling noticeably drops FPS; enabling it recovers it.  
Integration event: merge feature/renderer → integration; downstream workstreams sync.

---

### Milestone M3 — Directional Light and Lambertian Shading
Bottleneck: R-13 (light uniform struct — shared between renderer.h and shader)  
Parallel Groups: PG-M3-A (cpp side), PG-M3-B (shader side, starts after R-13)  
Depends on: M2 complete for renderer.cpp stability; M3 shader tasks depend only on R-13 (BOTTLENECK)

Tasks:
| ID   | Task                                                        | Tier | PG         | files                          |
|------|-------------------------------------------------------------|------|------------|--------------------------------|
| R-13 | Define sg_directional_light_t uniform struct; add to header | HIGH | BOTTLENECK | renderer.h                     |
| R-14 | Add light uniform block upload in renderer frame loop       | MED  | PG-M3-A    | renderer.cpp                   |
| R-15 | Define Lambertian material param struct                     | MED  | PG-M3-A    | material.h, material.cpp       |
| R-16 | Write Lambertian fragment shader                            | MED  | PG-M3-B    | shaders/lambertian.frag.glsl   |
| R-17 | Wire shader + material + uniform in pipeline creation       | MED  | SEQUENTIAL | renderer.cpp (after R-14..R-16)|

Expected outcome: Scene objects are lit from a directional source with Lambertian diffuse shading.  
Integration event: merge → integration; game engine workstream may swap mock lighting.

---

### Milestone M4 — Blinn-Phong Shading
Bottleneck: NO  
Parallel Groups: PG-M4-A (material/cpp), PG-M4-B (shader)  
Depends on: M3 complete (R-13 struct frozen)  
Note: M4 and M2 are independent — can run in parallel across agents if M3 bottleneck is cleared.

Tasks:
| ID   | Task                                                       | Tier | PG      | files                              |
|------|-----------------------------------------------------------|------|---------|------------------------------------|
| R-18 | Extend material param struct for Blinn-Phong (specular)   | MED  | PG-M4-A | material.h, material.cpp           |
| R-19 | Write Blinn-Phong fragment shader                         | MED  | PG-M4-B | shaders/blinn-phong.frag.glsl      |
| R-20 | Add toggle between Lambertian and Blinn-Phong pipelines   | MED  | SEQUENTIAL | renderer.cpp (after R-18, R-19) |

Expected outcome: Shading can be toggled between Lambertian and Blinn-Phong at runtime; specular highlights visible.  
Integration event: merge → integration.

---

### Milestone M5 — Background Skybox
Bottleneck: NO  
Parallel Groups: PG-M5-A (independent of M3/M4 cpp, only touches skybox files)  
Note: M5 is fully parallel with M4 since file sets are disjoint.

Tasks:
| ID   | Task                                           | Tier | PG      | files                                   |
|------|------------------------------------------------|------|---------|-----------------------------------------|
| R-21 | Load equirectangular or cubemap texture        | MED  | PG-M5-A | skybox.cpp, skybox.h                    |
| R-22 | Write skybox vertex + fragment shaders         | MED  | PG-M5-A | shaders/skybox.vert.glsl, skybox.frag.glsl |
| R-23 | Integrate skybox pass into renderer frame loop | MED  | SEQUENTIAL | renderer.cpp (after R-21, R-22)      |

Expected outcome: A skybox is rendered behind all scene geometry with no z-fight or depth artifacts.  
Integration event: merge → integration.
```

This example illustrates that M4 and M5 can proceed in parallel with each other (disjoint file sets) once M3's bottleneck task (R-13) is cleared, without requiring a dedicated integration sync between M4 and M5 themselves.

### 10.7 Milestone Integration Cadence

**Guideline:** Aim for **one milestone merge per 25–35 minutes** per workstream. With three workstreams operating in parallel the total integration rate across all streams may be higher, but each stream's individual merge cadence should be predictable enough that humans are not context-switching between integration events every few minutes.

- Milestones that are fully contained in one workstream (no cross-stream impact) are merged with a lightweight protocol: build passes, smoke test passes, human spot-check, push.
- Milestones that unlock or update a shared interface require the full sync protocol from section 9.2, including downstream workstream sync and mock-to-real swap where applicable.

***

## 11. Hackathon Workflow

### 11.1 Before the Hackathon

Pre-hackathon tasks include:

- Environment setup on all machines, including agreed hostnames set in `/etc/hostname`.
- Repo and worktree creation.
- Systems Architect planning and per-workstream SpecKit planning (three separate runs).
- Systems Architect cross-workstream synthesis pass producing initial `MASTER_TASKS.md`.
- Human review and freeze of interfaces, milestones, and queue formats.
- Asset manifest preparation and integrity checks.
- Local-model runtime validation on RTX 3090.
- Copilot and Gemini context-loading verification.
- Per-aspect library reference files generated and reviewed.
- Optional tooling checks for `sokol-shdc`.


### 11.2 Hackathon Start Sequence

At T+0:00:

1. Freeze docs and task lists.
2. Generate shared API headers from interface spec.
3. Generate mock implementations to enable parallelism.
4. Start three main workstreams in parallel.
5. Start queue-driven secondary workflow on RTX 3090 and overflow channels.

### 11.3 Secondary-Agent Dispatch Rules

- Hard graphics/math/systems implementation → Claude Sonnet first; GLM 5.1 or Claude Opus as needed.
- Standard implementation → Copilot first; Claude or Gemini as backup.
- Tests / narrow spec check / shader check / short diff review → local RTX 3090 model first if idle; Gemini if prompt is large or local box busy.
- Milestone validation across several files → Gemini, GLM 5.1, or Claude Sonnet; local model only with aggressively trimmed prompts.

***

## 12. Spec-Driven Planning Phase

The blueprint keeps the spec-driven planning approach but emphasizes:

- **FIXED (Iter 8):** SpecKit is run **once per workstream** (renderer, engine, game), not once across the whole project. See section 8.8 for full rationale and cross-workstream coherence protocol.
- Clear milestone unlocks and mock-to-real transitions.
- Validation level per task and mapping of tasks to agent routing (cloud vs local, Sonnet vs GLM 5.1 vs Copilot).
- Explicit mapping from SpecKit outputs into the stable doc and coordination schema.

***

## 13. Quality Gates

| Gate | Rule | Enforcer | Timing |
| :-- | :-- | :-- | :-- |
| G1 | `cmake --build` returns 0 | Implementing agent | Before marking DONE |
| G2 | Relevant tests pass | Implementing agent/Test Author | Before DONE where applicable |
| G3 | Frozen interfaces unchanged without approval | Human + validator | Continuous |
| G4 | Required validation completed | Spec Validator / Reviewer | Before merge when required |
| G5 | Milestone-level validation completed | Spec Validator + human + reviewer | Before milestone merge |
| G6 | `main` remains demo-safe | Humans | Continuous |
| G7 | Parallel group file sets remain disjoint | Task author (pre-claim) + claiming agent (pre-edit) | Before task claim |

Validation is risk-based per task and mandatory per milestone.

***

## 14. Pre-Hackathon Checklist (Updated for Iteration 8)

### Infrastructure

- [ ] Ubuntu 24.04 LTS verified on all machines.
- [ ] Agreed hostnames set in `/etc/hostname` on all machines and recorded in `AGENTS.md` hostname table.
- [ ] Clang, CMake, Ninja installed and tested.
- [ ] Repo and worktrees created on all machines, including RTX 3090.
- [ ] Queue and helper scripts tested.
- [ ] Build baseline opens a Linux window and clears the screen.


### Agent Configuration

- [ ] `AGENTS.md` finalized and shortened to global rules only, including hostname table and `agent_name@machine` convention.
- [ ] `CLAUDE.md` imports `AGENTS.md` and contains only Claude-specific notes.
- [ ] `.github/copilot-instructions.md` mirrors critical rules directly.
- [ ] `.gemini/settings.json` points its context file to `AGENTS.md`.
- [ ] Role and domain skills generated and reviewed, including `sokol-api`, `entt-ecs`, `cgltf-loading`, `glsl-patterns`, `physics-euler`.
- [ ] Copilot, Claude, Gemini, and z.ai workflows verified on the real repo structure.


### Local Agent

- [ ] Local context configured, with documented operational prompt budget smaller than the max.
- [ ] Real tests performed for narrow validation prompts and medium-sized test-file generation.
- [ ] Output-cap-aware prompting strategy documented.
- [ ] `validate_task.sh` or equivalent helper created.


### Coordination Files

- [ ] `docs/` vs `_coordination/` split implemented.
- [ ] Per-workstream `TASKS.md` files created with extended schema including `Parallel_Group` column.
- [ ] File sets populated in `Notes` for all tasks belonging to a parallel group.
- [ ] `MASTER_TASKS.md` created as compact integration dashboard after cross-workstream synthesis pass.
- [ ] Per-workstream `VALIDATION/` and `REVIEWS/` directories created.
- [ ] Milestone Groups and Milestones defined with `Expected Outcome` statements for all MVP work.


### Skills and Large Headers

- [ ] Distilled API SKILL.md files for `sokol_gfx`, `entt`, `cgltf`, and other heavy headers generated.
- [ ] Per-aspect reference files generated under `.agents/skills/<lib>-api/references/` for `sokol_gfx` at minimum (descriptors, buffers, render passes, draw calls).
- [ ] SKILL.md files contain a "Deeper references" section listing available aspect files.
- [ ] Rule in `AGENTS.md`: "Use library SKILLs before opening large headers; when needed, quote only minimal header snippets."
- [ ] One dry run where an agent uses only SKILL docs to implement a small feature involving each major library.


### Planning

- [ ] Per-workstream SpecKit runs completed (renderer, engine, game — three separate runs).
- [ ] Systems Architect cross-workstream synthesis pass run; `MASTER_TASKS.md` populated.
- [ ] Interface contracts frozen before individual SpecKit passes begin.
- [ ] Milestone sync rules documented.
- [ ] Task claim semantics ("edit + commit + push" when concurrency exists) confirmed.
- [ ] Milestone acceptance, unlock, and merge conditions written for every MVP milestone.
- [ ] GLM 5.1 backup and parallel-hard-task policy documented.
- [ ] Local validator fallback path to Gemini documented.

***

## 15. Fixed Decisions — Cumulative Through Iteration 8

1. Sections 1–7 remain stable and are treated as the high-level project and agentic baseline.
2. `AGENTS.md` is slimmed down to global guardrails; tool-specific behaviors and rich domain knowledge move into SKILL files and reference docs.
3. Copilot and Gemini continue to anchor on `AGENTS.md` via their respective configuration mechanisms; only minimal duplication lives in `.github/copilot-instructions.md`.
4. `_coordination/` vs `docs/` split remains: operational state vs reference material.
5. Per-workstream task files and a compact `MASTER_TASKS.md` remain the task management structure.
6. Validation and code review stay risk-based at task granularity and mandatory at milestone granularity.
7. Large headers such as `sokol_gfx.h`, `entt.hpp`, and `cgltf.h` are handled via a **Skill + per-aspect reference files** pattern with a "skills first, header snippets second" rule.
8. Per-aspect reference files live under `.agents/skills/<lib>-api/references/`, **not** under `docs/interfaces/`. Each file covers one narrow operational concern (descriptors, buffers, render passes, draw calls, etc.) at howto + minimal API signature granularity.
9. Task claiming uses `agent_name@machine` convention (e.g., `claude@laptopA`, `gemini@laptopC`). Human-initiated manual tasks use `FirstName@machine`. Agreed machine hostnames are set before the hackathon and recorded in `AGENTS.md`.
10. Agents that cannot determine machine name must not overwrite a populated `Owner`; they add a note in `Notes` instead.
```
11. Tasks have an extended schema with a `Parallel_Group` column: `PG-<milestone>-<letter>` for concurrent-ready tasks, `BOTTLENECK` for exclusive gates, `SEQUENTIAL` for ordered-but-not-exclusive tasks.
```

12. Tasks in the same parallel group must have **disjoint file sets**, declared in `Notes`. An agent discovering a file set violation must pause, update the task, and check for conflicts before proceeding.
13. Milestones are organized into **Milestone Groups** (thematic clusters, no mandatory integration event) and individual **Milestones** (formal sync points with `Expected Outcome` statements). Target cadence: ~1 milestone merge per 25–35 minutes per workstream.
14. SpecKit planning is executed **once per workstream** (three separate runs: renderer, engine, game), followed by a Systems Architect cross-workstream synthesis pass that produces or updates `MASTER_TASKS.md`. Interface contracts must be frozen before individual SpecKit passes begin.
15. Role and domain skills are designed for lazy loading: agents only load the skills relevant to the current task and workstream, including relevant per-aspect reference files.
16. SpecKit outputs remain precursor artifacts that must be synthesized into the stable documentation and coordination schema before the hackathon.

***

> **Suggested focus for Iteration 9:** Produce a worked example of a complete `sokol-api/SKILL.md` with its per-aspect reference files (`sokol-api-work-with-buffers.md`, `sokol-api-render-passes.md`, `sokol-api-draw-calls.md`), and a worked example of a fully populated renderer `TASKS.md` fragment covering Milestone Groups "Bootstrap" and "Shading" with parallel groups, file sets, and bottleneck markers filled in.[^1]

[^1]: Vibe-Coding-Hackathon-Master-Blueprint-Iteration-7.md

