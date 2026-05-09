# Feature Development Workflow

Canonical process for planning, executing, and closing features in this project. Replaces the hackathon-era coordination structure.

---

## Feature Types

Classify the feature before planning. The type determines the artifact set and process.

| Type | Characteristics | Examples |
|------|----------------|---------|
| **Quick** | ~4–8 tasks, known API surface, deterministic outcome. Build correctness is sufficient proof. | Gameplay constant, HUD element, shader uniform, spawning variant |
| **Phased** | 1–3 phases, some unknowns or visual subjectivity. Human judgment required at checkpoints. | New visual effect, weapon behavior rework, new physics interaction |
| **Exploratory** | Significant unknowns, subjective quality gates, approach may change. Spikes required. | Deferred rendering, GPU-driven culling, advanced post-processing |

When uncertain, default to Phased and refine after the Feature Brief is written.

---

## Artifacts

All feature work lives under `features/active/<feature-name>/` while in progress.

| Artifact | Filename | Created when | Feature types |
|----------|----------|--------------|---------------|
| Feature Brief | `brief.md` | First, always | All |
| Exploratory Roadmap | `roadmap.md` | After Brief | Exploratory |
| Spike Report | `spike-<name>.md` | One per unknown | Exploratory |
| Technical Design | `design.md` | When needed | Phased, Exploratory |
| Phase Plan | `plan-p1.md`, `plan-p2.md`, … | One per phase, before execution | All |
| Phase Checkpoint | `checkpoint-p1.md`, `checkpoint-p2.md`, … | After human verification | All |

Templates for all artifacts: `.agents/skills/feature-planner/templates/`

---

## Process Flows

### Quick Feature

```
1. Human describes feature
2. Feature Planner creates brief.md + plan-p1.md (together, same session)
3. Plan Groomer reviews plan-p1.md → PASS | NEEDS WORK → fix → re-groom
4. Execute
5. Human verifies → writes checkpoint-p1.md
6. If feature complete: update docs/ → move to features/archive/
```

### Phased Feature

```
1. Human writes or describes the feature
2. Feature Planner creates brief.md
3. Feature Planner creates plan-p1.md
4. Plan Groomer reviews plan-p1.md → PASS | NEEDS WORK
5. Execute Phase 1
6. Human verifies → checkpoint-p1.md
7. Feature Planner creates plan-p2.md (informed by checkpoint-p1 learnings)
8. Plan Groomer reviews plan-p2.md → PASS | NEEDS WORK
9. Execute Phase 2 → checkpoint-p2.md
   ... repeat per phase ...
10. All phases done: update docs/ → move to features/archive/
```

### Exploratory Feature

```
1. Human writes brief.md (with explicit unknowns table)
2. Feature Planner creates roadmap.md — full journey with decision branches
3. Plan Groomer reviews roadmap.md → PASS | NEEDS WORK
4. Execute Spikes:
   - One focused spike per significant unknown
   - Write spike-<name>.md with result
   - Annotate roadmap.md decision points with spike findings
5. Feature Planner creates plan-p1.md (constrained by spike results, aligned with Roadmap)
6. Plan Groomer reviews plan-p1.md → PASS | NEEDS WORK
7. Execute Phase 1
8. Human verifies → checkpoint-p1.md (also annotates the Roadmap decision point)
9. Feature Planner creates plan-p2.md — only now, informed by checkpoint-p1
   ... repeat per phase, always oriented toward the Roadmap goal ...
10. All phases done: update docs/ → move to features/archive/
```

---

## Key Rules

**One-phase-at-a-time.** An agent must not generate `plan-pN.md` without `checkpoint-p(N-1).md` present in the feature directory. Phase 1 is gated by the Feature Brief itself (human approval of the brief = green light). This rule prevents downstream plans from inheriting assumptions from unverified phases.

**Plan Groomer is not optional.** Every Phase Plan and every Exploratory Roadmap must pass grooming before execution begins. Quick features are not exempt — the groomer catches glaring gaps that short plans often miss.

**The Roadmap is annotated, not rewritten.** As spikes complete and phase checkpoints are recorded, add dated lines to the Roadmap's annotation section. The document becomes a record of how the theory held up. Do not rewrite phase descriptions retroactively.

**The "how" belongs to the implementing agent.** Phase Plans specify what each task must achieve and what observable result proves it. They do not prescribe specific function names, data structures, or implementation strategies unless those are forced by the interface contract. The agent implementing the task decides the how.

**Frozen interfaces are approval gates.** If a Phase Plan requires a change to `docs/interfaces/*-interface-spec.md`, human approval must be explicitly noted in the plan as a GATE before any dependent tasks. Do not implement dependents speculatively.

**Implementing agents stop at phase boundaries.** When all tasks in a phase plan are complete, the implementing agent outputs a handoff (summary + quoted Human Checkpoint criteria) and stops. It does not write `checkpoint-pN.md` and does not begin or sketch the next phase plan. The human runs the verification and provides feedback; Feature Planner drafts the checkpoint from that feedback and writes the file once the human confirms. Feature Planner then writes the next phase plan. The checkpoint requires human-observed behavior — an agent can format it, but cannot substitute for the human having actually run the check.

---

## Feature Lifecycle

When all phase checkpoints pass and the human signs off:

1. **Update live reference docs** — invoke Doc Updater (`.agents/skills/doc-updater/SKILL.md`). It reads the completed feature artifacts, determines which docs changed, and updates them. Covers:
   - `docs/interfaces/` — if a public API changed
   - `docs/architecture/` — if a subsystem's design changed
   - `docs/game-design/` — if gameplay intent changed
2. **Move** `features/active/<name>/` → `features/archive/<name>/`

`features/archive/` is institutional memory. The brief, roadmap, spike reports, phase plans, and checkpoints together tell the story of how the feature was built and what was learned.

---

## Workflow Skills

| Skill | File | Purpose | When |
|-------|------|---------|------|
| Feature Planner | `.agents/skills/feature-planner/SKILL.md` | Creates and structures feature artifacts | Start of feature / after each checkpoint |
| Plan Groomer | `.agents/skills/plan-groomer/SKILL.md` | Adversarial review before execution | After every Phase Plan or Roadmap |
| Doc Updater | `.agents/skills/doc-updater/SKILL.md` | Updates `docs/` to reflect what was built | Once, after final checkpoint, before archive |

Existing skills that remain active:
- `spec-validator` — verifies code matches written spec after implementation
- `code-reviewer` — reviews diffs for risk and correctness
- `systems-architect` — cross-workstream seams and frozen interface changes
