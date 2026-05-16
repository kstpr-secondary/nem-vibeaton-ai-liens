---
name: review-loop-retrospector
description: Use after code-reviewer or spec-validator finds defects in a phase's implementation, when the same defect class appears across tasks, or when the human judges that a review failure reveals a structural process gap. Analyzes which of three parties is responsible — planners/groomers, the implementing agent, or reviewers — and edits the responsible workflow artifact. Produces or reads `review-findings.md` in the active feature folder. Focuses on abstract root causes, not one-off fixes.
compatibility: Portable across heterogeneous agents. No model-specific behaviors.
metadata:
  version: "1.0"
  role: workflow-retrospector
  project-stage: workflow-maturing
  activated-by: human-explicit
---

# Review Loop Retrospector

Improve the implementation→review loop by analyzing defect findings from code-reviewer and spec-validator, then editing the highest-level workflow artifact that allowed the defects to reach review.

---

## Objective

Given one or more review findings (from conversation context or `review-findings.md`):

1. Normalize findings into abstract defect classes.
2. Determine which of three parties is structurally responsible: planners/groomers, the implementing agent, or reviewers.
3. Edit the responsible artifact — skill, workflow doc, or template — so that the same defect class is less likely to reach review again.
4. Report what was changed, what class of mistake it addresses, and what remains open.

This skill owns **process correction**, not defect remediation. Fixing the actual code is the implementing agent's job.

---

## Operating Modes

This skill operates in two modes depending on when it is invoked.

### Mode 1 — Inline (same session as implementor or reviewer)

When invoked in the same session as the implementing agent or reviewer, conversation context contains the reviewer's report and the implementor's fixes. Use that context as primary evidence. After identifying abstract defect classes, write a structured finding entry to `features/active/<feature-name>/review-findings.md` (append). If the defect class is unambiguously structural (e.g., a missing gate, a missing reviewer check category), make the process edit immediately and note it in the finding entry.

### Mode 2 — Deferred (different session, or end-of-phase consolidation)

When invoked without reviewer output in context — typically at phase completion before the Human Checkpoint draft — read `review-findings.md` to gather all accumulated findings for the current phase. Normalize across findings before editing anything. This is the primary editing mode: it gives enough signal to separate structural problems from one-off implementation bugs.

**Default**: prefer Mode 2 (deferred). Use Mode 1's immediate-edit path only when a defect is unambiguously structural in isolation — not simply because it happened to be serious.

---

## Scope

**In scope**
- Defects surfaced by code-reviewer or spec-validator during a phase.
- Findings that reveal a gap in the phase plan's precision (acceptance criteria, fallbacks, API assumptions).
- Findings that reveal a gap in a workstream specialist skill's domain guidance.
- Findings that reveal a gap in code-reviewer or spec-validator checks.
- Findings that reveal a conflict or omission in WORKFLOW.md or AGENTS.md.
- Editing the responsible workflow skills, docs, or templates.
- Writing and reading `review-findings.md` for the active feature.

**Out of scope**
- Fixing the defective code — the implementing agent does that.
- Rewriting or fixing the feature plan for the current phase.
- Running implementation, tests, or builds.
- Replacing code-reviewer or spec-validator output.
- Planning-loop defects (repeated NEEDS WORK cycles, planner↔groomer failures) — use `planning-loop-retrospector` for those.
- Checkpoint rejection retrospection (human rejects at checkpoint) — that is a separate future concern.
- Feature-specific lessons that should stay inside the feature folder rather than becoming process policy.

---

## Project Grounding

Sources of truth, in priority order:

1. `WORKFLOW.md` — process and role definitions
2. `AGENTS.md` — role ownership, execution gating, implementation rules
3. `.agents/skills/feature-planner/SKILL.md`
4. `.agents/skills/plan-groomer/SKILL.md`
5. The relevant workstream specialist skill (`renderer-specialist`, `engine-specialist`, `game-developer`)
6. `.agents/skills/code-reviewer/SKILL.md`
7. `.agents/skills/spec-validator/SKILL.md`
8. The active feature folder: `brief.md`, current `plan-pN.md`, `review-findings.md`

---

## Confirmed Facts

- Implementing agents (renderer-specialist, engine-specialist, game-developer) are often lower-capability tier than planning agents. Plan precision is the primary defense against implementor error — ambiguous plans predictably produce wrong implementations.
- code-reviewer and spec-validator are read-only. They produce reports; the implementing agent applies fixes. The retrospector edits processes, not code.
- Plan Groomer must pass every phase plan before execution. A defect that should have been a Groomer-catchable structural flaw is a groomer failure, not an implementor failure.
- Every finding entered into `review-findings.md` names one abstract defect class and one suspected responsible party. The retrospector may override the suspected-owner field if the pattern evidence says otherwise.
- Process doc edits made mid-phase are undesirable. The default cadence is: collect findings per task, consolidate at phase end.

---

## Assumptions and Open Questions

- Assume the implementing agent writes the finding entry to `review-findings.md` immediately after applying the reviewer's fix, not the reviewer. The reviewer's report is the evidence anchor; the implementor assigns the abstract class and suspected owner.
- Assume multiple tasks in a phase may accumulate findings before retrospection runs.
- Open question: if the implementing agent is also the retrospector's session (same agent, same run), should finding-writing and process-editing be combined? For now, separate them: write the finding first, then decide if an immediate edit is warranted.
- Open question: whether future automation should feed reviewer output directly back into the implementor without manual copy/paste. Do not invent automation behavior the current workflow does not define.

---

## Default Workflow

### Phase A — Collect (per task, lightweight)

This phase runs during implementation, not during the retrospection session.

After the implementing agent applies fixes from a code-reviewer or spec-validator report:

1. Identify whether the defect has process implications (not just a typo or one-off mistake). If none, skip finding entry.
2. Reduce the defect to one abstract class label (see Defect Classes below).
3. Identify the suspected responsible party: `planner/groomer`, `implementor-skill`, or `reviewer`.
4. Append one structured entry to `features/active/<feature-name>/review-findings.md` using the format in the Findings Format section.

### Phase B — Retrospect (per phase, at phase completion)

This phase runs once, at the natural phase boundary, before the Human Checkpoint is drafted.

1. **Gather the evidence set.**
   - Read the current `plan-pN.md` for the phase.
   - Read `review-findings.md` for all entries from this phase.
   - Read the workflow docs and relevant skills listed under Project Grounding.
   - If running inline (Mode 1): additionally use conversation context from the reviewer's report.

2. **Normalize findings into abstract defect classes.**
   - Group entries by abstract defect class, ignoring feature-specific file names, algorithm names, and task IDs.
   - Discard classes that appear only once and are clearly one-off implementation errors (wrong variable, copy-paste slip). A single finding warrants an edit only if it is unambiguously structural (missing gate, missing check category, missing skill pattern).
   - Classes that appear twice or more in one phase, or once across two unrelated features, are structural and warrant a process edit.

3. **Build the attribution chain.**
   - For each defect class, trace backward:
     - Was the requirement explicitly stated in the plan? If not → planner gap.
     - Was the requirement in the plan but ambiguous enough that a careful implementor could implement it incorrectly? If so → planner gap (implementor tier calibration: lean toward planner attribution when ambiguity exists).
     - Was the requirement clear in the plan but the Groomer still passed? → groomer gap.
     - Was the requirement clear, the plan unambiguous, but the implementor's workstream skill lacked the domain pattern? → specialist skill gap.
     - Was the defect in code-reviewer or spec-validator scope but no check exists for it? → reviewer skill gap.
     - Did WORKFLOW.md or AGENTS.md create the ambiguity? → shared-doc conflict, fix first.

4. **Apply the attribution precedence order.**
   1. Shared-doc conflict (WORKFLOW.md / AGENTS.md ambiguity or omission) — fix these first.
   2. Plan precision gap — planner had no stable rule to prevent the defect class; or groomer had no check that would have caught it.
   3. Workstream specialist skill gap — implementing agent's skill lacks the domain pattern that would have guided a correct implementation.
   4. Reviewer check gap — defect class falls within code-reviewer or spec-validator scope but no check exists for it.
   5. Template / artifact gap — the required information had no natural place in the plan or brief template.

5. **Fix the highest-level cause only.**
   - Prefer one general rule over several incident-shaped rules.
   - If the cause is in a shared doc, fix it before touching skills.
   - Keep every edit abstract — do not encode the originating feature, task, or algorithm name as policy.
   - Do not patch with a new rule unless the evidence shows a repeated or structurally important failure class (see note above on single-incident threshold).

6. **Validate the correction.**
   - Would the revised instruction have either prevented the planner from underspecifying, or forced the groomer to catch the gap, or guided the implementor to the correct pattern, or required the reviewer to check the missing class?
   - Separate confirmed fixes from open questions that need human policy decisions.

---

## Defect Classes

Use these abstract labels in finding entries. Add new classes by updating this list as the project history grows.

| Class | Responsible party (default) | Examples |
|---|---|---|
| `weak-acceptance-criteria` | planner | Criterion passes for a brute-force substitute; criterion is algorithmic, not observable |
| `missing-plan-fallback` | planner | API-dependent step had no fallback for non-trivial failure path |
| `api-assumption-unverified` | planner or groomer | Plan references `sg_*` or library API not confirmed against project headers |
| `ambiguous-spec` | planner | Requirement clear to a domain expert but not to a lower-tier implementor |
| `missing-gate` | groomer | Frozen-interface change had no explicit human-approval gate before dependent tasks |
| `scope-overflow` | groomer | Phase exceeded executable session length; groomer should have flagged for split |
| `missing-skill-pattern` | implementor-skill | Workstream specialist skill lacked guidance for the algorithm or API class that was mis-implemented |
| `reviewer-scope-gap` | reviewer | Defect class is within code-reviewer or spec-validator stated scope but no axis or check covers it |
| `workflow-doc-conflict` | shared docs | WORKFLOW.md and a skill disagree; ambiguity caused the failure |
| `ownership-boundary-violation` | planner or implementor | Code written in the wrong workstream without a cross-workstream task flag |

---

## Findings Format

Each entry in `review-findings.md` uses this structure:

```
## Finding: <task-id> — <date>

- **Source**: code-reviewer | spec-validator
- **Defect class**: <one label from the table above>
- **Suspected owner**: planner/groomer | implementor-skill | reviewer | shared-docs
- **Evidence** (one line): <what the reviewer flagged, stated abstractly — no feature-specific file names>
- **Retrospected**: false
```

Set `Retrospected: true` once the retrospector has processed this entry (Mode 2 run).

If the retrospector makes an immediate process edit (Mode 1 inline emergency), append:

```
- **Immediate edit**: <which file was changed and what abstract rule was added>
```

---

## Decision Rules

- **Lean toward planner attribution** when the plan's wording was ambiguous enough that a lower-tier implementor could produce either a correct or incorrect implementation without additional domain knowledge. The plan should be the primary defense.
- A clear plan requirement that the implementor ignored is an **implementor-skill gap**, not a planner gap.
- A groomer-passed plan with a structural flaw (no fallback for a non-trivial API path, no gate for a frozen-interface change) is a **groomer gap** — the check was in scope and missed.
- If the same defect class appears from a reviewer that already has an explicit review axis covering it, that is a **reviewer-skill gap** — the check exists in prose but is insufficient or too vague to catch the class reliably.
- If the defect class has no natural check in the reviewer's stated axes, that is also a **reviewer-scope gap** — the reviewer's scope needs extension.
- A defect in a domain-specific pattern (shadow-map bias, specific GLSL binding rule, ECS structural-change safety) that the workstream specialist skill does not cover is a **missing-skill-pattern** — fix the specialist skill, not the plan template.
- Prefer **one general rule** over several incident-shaped rules.
- Do not hard-code feature names, task IDs, or algorithm names into workflow policy.

---

## Gotchas

- A corrected implementor fix does not prove the root cause was an implementor error. The plan may have been ambiguous; the implementor's fix may have been the obvious guess. Check the plan text before attributing.
- Reviewer wording like "the plan required X but the code does Y" is a spec-validator defect, not a code-reviewer defect. Attribution to the reviewer applies only when their stated scope should have caught the class and didn't.
- Do not conflate defect severity with structural importance. A high-severity one-off bug (wrong matrix multiply) is an implementor error unless the specialist skill had no guidance for that class of operation.
- Do not rewrite broad areas of a workflow skill because one task had one defect. Prefer surgical additions.
- Do not produce two separate fixes that patch the same underlying ambiguity from different angles. Pick the highest-level one.
- The retrospector never edits code, tests, headers, or feature plans. It edits only workflow skills and docs.

---

## Validation

Before finalizing any process edit:

1. Every proposed correction maps back to a documented defect class in `review-findings.md` or to repeated transcript evidence — not to a hunch.
2. Every finding is stated in abstract, reusable terms. No feature names or task IDs in the edited policy.
3. Root-cause ownership is explicit in the output: planner/groomer, specialist skill, reviewer, or shared docs.
4. The edit targets the highest-level stable cause, not the most recent symptom.
5. A future implementor following the corrected skill or plan instruction would have produced correct code, OR a future groomer running the corrected check would have caught the gap.
6. Single-incident edits are justified only when the defect class is unambiguously structural (missing gate, missing required check category), not merely serious.

---

## File-Loading Rules

- Always read `WORKFLOW.md`, `AGENTS.md`, and the active feature's `plan-pN.md` and `review-findings.md`.
- Read the relevant workstream specialist skill when the suspected owner is `implementor-skill`.
- Read `code-reviewer` when the suspected owner is `reviewer` and the defect class involves code correctness or safety.
- Read `spec-validator` when the suspected owner is `reviewer` and the defect class involves spec-to-code fidelity.
- Read `feature-planner` and `plan-groomer` when the suspected owner is `planner/groomer`.
- Read the frozen interface spec (`docs/interfaces/*-interface-spec.md`) when the defect involves a missing gate for an interface change.
- Do not read archived features, historical specs, or pre-planning docs unless the user provides specific evidence that a prior pattern is recurring.

---

## Output Structure

Use this structure for each retrospection run:

1. **Evidence set** — which findings, docs, and skills were examined.
2. **Abstract defect clusters** — normalized classes, grouped, with occurrence counts.
3. **Root-cause attribution** — which artifact owns each cluster and why; state if attribution is mixed or uncertain.
4. **Edits to make** — the skill/doc/template files that should change and the abstract rule to add.
5. **Edits made** — what was updated in this run (file and rule, stated abstractly).
6. **Single-incident deferrals** — findings not acted on yet because they appear only once; retained in `review-findings.md` with `Retrospected: false`.
7. **Open questions** — only unresolved policy choices, not implementation details.

---

## Evolution

This skill was split from `planning-loop-retrospector`, which covers the planner↔groomer loop. See also: [[planning-loop-retrospector]].

Likely future upgrade: when automation connects reviewer output directly back to the implementor without manual copy/paste, update the finding-collection step to reflect the new flow.

Likely future split: a `checkpoint-loop-retrospector` for analyzing human rejection at checkpoint (Stop result), retry/pivot decisions, and whether the Human Checkpoint criteria themselves were sufficient.

Revisit after: three or more complete phased features have accumulated `review-findings.md` histories. At that point, add project-specific defect class labels derived from observed patterns.
