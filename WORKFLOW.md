# Feature Development Workflow

Canonical process for planning, executing, and closing features in this project. Replaces the hackathon-era coordination structure.

---

## Feature Types

Classify the feature before planning. The type determines the artifact set and process.

| Type | Characteristics | Examples |
|------|----------------|---------|
| **Quick** | ~4–8 tasks, known API surface, deterministic outcome. Build correctness is sufficient proof — the Human Checkpoint may be a build-only check when no behavioral verification is needed. The checkpoint file is still written. | Gameplay constant, HUD element, shader uniform, spawning variant |
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
| Review Findings | `review-findings.md` | Append-mode per task when a reviewer defect has process implications; read by retrospector at phase end | Phased, Exploratory (optional for Quick) |

Templates for all artifacts: `.agents/skills/feature-planner/templates/`

---

## Process Flows

> **Optional pre-checkpoint validation.** Before any Human Checkpoint, `spec-validator` and `code-reviewer` are available as pre-verification steps. Invoke them when the phase includes: (a) a frozen-interface touch, (b) a named algorithm where a brute-force substitute would pass the same tests, or (c) hot-path or math-heavy code. For Quick features, skip unless one of those criteria applies. For Phased and Exploratory, the criteria apply often enough that invoking both before the Human Checkpoint is the recommended default. These tools surface spec gaps and risk before the human spends time testing. **If either tool reports blockers, the implementing agent fixes them before the Human Checkpoint.** A re-run after fixes is optional but recommended for changed code.
>
> **Review findings and retrospection.** When a reviewer finds defects, the implementing agent records an abstract finding entry in `features/active/<feature-name>/review-findings.md` after applying the fix (see `review-loop-retrospector` skill for entry format). At phase completion — after all tasks are done and all review cycles are closed, before the Human Checkpoint draft — invoke `review-loop-retrospector` to consolidate the phase's findings and edit any responsible workflow docs or skills. This step is optional for Quick features (too few tasks to establish a pattern) and recommended for Phased and Exploratory features. The retrospector never edits code; it edits only process artifacts.

### Quick Feature

```
1. Human describes feature
2. Feature Planner creates brief.md + plan-p1.md (together, same session)
3. Plan Groomer reviews plan-p1.md → PASS | NEEDS WORK → fix → re-groom
4. Execute
5. Human verifies → Feature Planner drafts checkpoint-p1.md from human feedback → human confirms → Feature Planner writes checkpoint-p1.md
6. If feature complete: invoke Doc Updater → human confirms report → human moves to features/archive/
```

### Phased Feature

```
1. Human writes or describes the feature
2. Feature Planner creates brief.md
3. Feature Planner creates plan-p1.md
4. Plan Groomer reviews plan-p1.md → PASS | NEEDS WORK
5. Execute Phase 1
6. Human verifies → Feature Planner drafts checkpoint-p1.md from human feedback → human confirms → Feature Planner writes checkpoint-p1.md
7. Feature Planner creates plan-p2.md (informed by checkpoint-p1 learnings)
8. Plan Groomer reviews plan-p2.md → PASS | NEEDS WORK
9. Execute Phase 2 → (same checkpoint process as step 6)
   ... repeat per phase ...
10. All phases done: invoke Doc Updater → human confirms report → human moves to features/archive/
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
8. Human verifies → Feature Planner drafts checkpoint-p1.md from human feedback → human confirms → Feature Planner writes checkpoint-p1.md (also annotates the Roadmap decision point)
9. Feature Planner creates plan-p2.md — only now, informed by checkpoint-p1
   ... repeat per phase, always oriented toward the Roadmap goal ...
10. All phases done: invoke Doc Updater → human confirms report → human moves to features/archive/
```

---

## Key Rules

**One-phase-at-a-time.** An agent must not generate `plan-pN.md` without `checkpoint-p(N-1).md` present in the feature directory. Phase 1 is gated by the Feature Brief itself (human approval of the brief = green light). This rule prevents downstream plans from inheriting assumptions from unverified phases.

**Plan Groomer is not optional.** Every Phase Plan and every Exploratory Roadmap must pass grooming before execution begins. Quick features are not exempt — the groomer catches glaring gaps that short plans often miss.

**The Roadmap is annotated, not rewritten.** As spikes complete and phase checkpoints are recorded, add dated lines to the Roadmap's annotation section. The document becomes a record of how the theory held up. Do not rewrite phase descriptions retroactively.

**The "how" belongs to the implementing agent.** Phase Plans specify what each task must achieve and what observable result proves it. They do not prescribe specific function names, data structures, or implementation strategies unless those are forced by the interface contract. The agent implementing the task decides the how.

**Acceptance criteria for named algorithms must discriminate.** If a task names a specific algorithm (gift-wrapping, SAT, slab intersection, etc.), at least one acceptance criterion must be observable that would *fail* for the plausible wrong alternative (e.g., a brute-force substitute). "Tests pass" is insufficient if a brute-force implementation would also pass the same tests. The Feature Planner is responsible for writing this discriminating criterion; the Plan Groomer must flag its absence (see groomer check 20).

**Frozen interfaces have two distinct moments.** (1) Pre-implementation gate: if a Phase Plan requires a change to `docs/interfaces/*-interface-spec.md`, human approval must be explicitly noted in the plan as a GATE before any dependent tasks. Do not implement dependents speculatively. (2) Post-feature doc sync: after the final checkpoint passes, Doc Updater updates interface specs to reflect what was actually built. These are separate steps — do not conflate them.

**Checkpoint ownership is Feature Planner, not the human or the implementing agent.** The human runs the verification and provides free-form feedback. Feature Planner drafts `checkpoint-pN.md` from that feedback, shows the draft for confirmation, and writes the file only after the human confirms it accurately reflects what was observed. The implementing agent does not write the checkpoint. The human does not write the checkpoint file directly — their role is to verify, provide feedback, and confirm the draft. Feature Planner then writes the next phase plan.

**Implementing agents stop at phase boundaries.** When all tasks in a phase plan are complete, the implementing agent outputs a handoff (summary + quoted Human Checkpoint criteria) and stops. It does not write `checkpoint-pN.md` and does not begin or sketch the next phase plan.

**Stop means Evaluate, not Fail Forward.** When a Human Checkpoint returns Stop, the human chooses between two paths: **(a) Retry** — Feature Planner produces a revised `plan-pN.md` (same filename, revision note in the header, Groomer Verdict reset to PENDING), Plan Groomer reviews before re-execution, and a new `checkpoint-pN.md` replaces the prior one after re-verification; or **(b) Pivot** — if the failing behavior is a bounded gap that a subsequent phase will address, the human records the gap in `checkpoint-pN.md` and Feature Planner writes `plan-p(N+1).md` scoped to fix it. The human decides: if the unmet criteria would make the next phase's assumptions unsound, retry; if the gap is isolated and bounded, pivot.

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
| Planning Loop Retrospector | `.agents/skills/planning-loop-retrospector/SKILL.md` | Analyzes repeated planner↔groomer failures and fixes the responsible workflow docs or skills | After repeated grooming failures, groomer disagreement, or a missed blocker after PASS |
| Review Loop Retrospector | `.agents/skills/review-loop-retrospector/SKILL.md` | Analyzes defects surfaced by code-reviewer or spec-validator, attributes them to planners/groomers, implementors, or reviewers, and fixes the responsible workflow artifact | At phase completion after review cycles, when a defect class appears across tasks, or when a single finding is unambiguously structural |
| Doc Updater | `.agents/skills/doc-updater/SKILL.md` | Updates `docs/` to reflect what was built | Once, after final checkpoint, before archive |

Existing skills that remain active:
- `spec-validator` — verifies code matches written spec after implementation *(read-only: produces a gap report, never edits code)*
- `code-reviewer` — reviews diffs for risk and correctness *(read-only: produces a defect report, never edits code)*
- `systems-architect` — cross-workstream seams and frozen interface changes
