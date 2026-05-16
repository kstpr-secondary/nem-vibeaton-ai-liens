---
name: planning-loop-retrospector
description: Use when repeated Plan Groomer reviews fail across revisions, when two groomers disagree, or when planner↔groomer history should be analyzed to fix the responsible workflow skill or document. Focuses on abstract root causes and process edits, not fixing one feature plan.
compatibility: Portable across heterogeneous agents. No model-specific behaviors.
metadata:
  version: "1.0"
  role: workflow-retrospector
  project-stage: workflow-maturing
  activated-by: human-explicit
---

# Planning Loop Retrospector

Improve the planning loop by analyzing prior Feature Planner and Plan Groomer exchanges, then fixing the highest-level workflow artifact that caused the repeated failure.

---

## Objective

Given several rounds of planning and grooming history:

1. Reconstruct the failure pattern across planner drafts, groomer reviews, and plan revisions.
2. Reduce the evidence to **abstract defect classes**, not feature-specific incidents.
3. Decide whether the root cause belongs in:
   - `feature-planner`,
   - `plan-groomer`,
   - another planning-adjacent skill such as `systems-architect` or a workstream specialist,
   - `WORKFLOW.md`,
   - `AGENTS.md`, or
   - shared templates or project instructions.
4. Edit the responsible artifact so the same class of failure is less likely to recur.
5. Report what was changed, what class of mistake it addresses, and what remains open.

This skill owns **process correction**, not one-off plan repair.

---

## Scope

**In scope**
- Repeated `NEEDS WORK` cycles on the same plan or roadmap.
- Disagreement between sequential groomers, including "first PASS, later blocker found".
- Planner omissions that stem from missing or weak instructions.
- Groomer omissions that stem from missing checks, weak guardrails, or contradictory instructions.
- Conflicts between workflow docs and skill docs that make good planning or grooming unreliable.
- Updating the responsible workflow documentation or skill files.

**Out of scope**
- Fixing the current feature plan itself.
- Running implementation, code review, or spec validation loops.
- Building orchestration or automation between agents.
- Checkpoint-feedback retrospection outside the planning/grooming loop.
- Feature-specific lessons that should stay inside one feature folder instead of becoming process policy.

---

## Project grounding

Use these sources of truth first:
- `WORKFLOW.md`
- `.agents/skills/feature-planner/SKILL.md`
- `.agents/skills/plan-groomer/SKILL.md`
- `AGENTS.md`

Read these only when the evidence points at them:
- `.agents/skills/systems-architect/SKILL.md`
- relevant workstream specialist skills under `.agents/skills/`
- `.agents/skills/code-reviewer/SKILL.md`
- `.agents/skills/spec-validator/SKILL.md`

Use planning/grooming transcript history only as evidence of where the workflow failed in practice. If transcript history is unavailable, require the human to provide the relevant planner and groomer exchanges.

---

## Confirmed facts

- Every Phase Plan and Exploratory Roadmap must pass Plan Groomer before execution.
- Phase plans are revised in place after `NEEDS WORK`; the filename does not change.
- Feature Planner owns planning artifacts and revision-mode fixes.
- The workflow already includes adversarial review loops before execution and before human checkpoints.
- Process documents and skills are intended to be updated when they are stale or incomplete.

---

## Assumptions and open questions

- Assume multiple independent groomers may be run sequentially, even though the workflow does not yet formalize multi-groomer orchestration.
- Assume the human may provide planner/groomer chat history, pasted outputs, or accessible session logs.
- Open question: whether future automation should feed groomer output directly back into Feature Planner without manual copy/paste.
- Open question: whether this skill should later split into planner-loop, reviewer-loop, and checkpoint-loop variants.

Do not invent automation behavior that the current workflow does not define.

---

## Default workflow

1. **Collect the evidence set.**
   - Read the current workflow docs and the relevant skills.
   - Gather the planner draft, each groomer result, each revised plan, and any human prompt that changed review framing.
   - If two groomers disagreed, gather both outputs before attributing blame.

2. **Normalize defects into abstract classes.**
   - Convert each concrete complaint into a general category such as:
     - weak acceptance criteria,
     - missing gate or ownership rule,
     - unsupported API assumption,
     - missing file-disjointness rule,
     - missing upstream consultation,
     - prompt-induced review-scope drift,
     - contradiction between workflow artifacts.
   - Strip feature names, file names, and one-off implementation details from the category labels.

3. **Build the chronology.**
   - Determine whether each defect:
     - existed in the first planner draft,
     - was introduced by a later revision,
     - was present all along but missed by one or more groomers.
   - Do not call something a groomer miss unless the earlier artifact already contained the problem.

4. **Attribute the root cause.**
   - Use this precedence order:
     1. **Shared-doc conflict** — if `WORKFLOW.md`, `AGENTS.md`, and the skills disagree, fix the shared docs first.
     2. **Planner guidance gap** — if the planner had no stable instruction that would have prevented the defect class.
     3. **Groomer omission** — if the defect class was already mandatory to check, but a groomer still passed or surfaced it only after a later PASS.
     4. **Upstream planning-skill gap** — if the planner failed because `systems-architect` or a specialist skill did not provide the needed boundary or domain guidance.
     5. **Template or artifact gap** — if the required information had no natural place to live in the existing plan/brief structure.
     6. **Prompt-guardrail gap** — if a human request such as "only high-impact issues" caused the groomer to skip mandatory checks.

5. **Fix the highest-level cause only.**
   - Prefer one general rule over several incident-shaped rules.
   - If multiple skills derive the same ambiguity from `WORKFLOW.md`, fix `WORKFLOW.md` first, then align the skills.
   - If the issue is isolated to one owning skill, edit only that skill.
   - Keep every edit abstract. Do not encode the originating feature as policy.

6. **Validate the correction.**
   - Check that the revised instruction would have either:
     - prevented the planner from making the mistake, or
     - forced the groomer to catch it before PASS.
   - Separate confirmed fixes from open questions that need human policy decisions.

---

## Decision rules

- Prefer **general root causes** over feature-specific symptoms.
- Do not patch the workflow with a new rule unless the evidence shows a repeated or structurally important failure class.
- A later groomer finding a blocker after an earlier PASS is a **groomer-side problem** only if the blocker was already in scope for the earlier review.
- If the missed blocker followed a prompt that narrowed the review ("no nitpicks", "high impact only"), treat that as a **guardrail failure** unless the skill already forbids relaxing mandatory checks.
- If the same defect class appears across unrelated features, strengthen shared workflow or planner instructions before touching specialist skills.
- If the defect depends on cross-workstream boundaries or frozen interfaces, inspect `systems-architect` and the brief/planning gate rules before blaming the implementor-facing specialists.
- If the evidence cannot separate planner fault from groomer fault, fix the shared ambiguity and state the attribution as mixed.

---

## Gotchas

- Repeated wording is not the same as repeated defect classes. Deduplicate by cause, not phrasing.
- A stronger second-groomer explanation may describe the same missed class more clearly; do not count it as a new category unless the underlying problem differs.
- Do not hard-code one feature's filenames, algorithms, or asset names into workflow policy.
- Do not excuse a missed mandatory check just because the human asked for a terser pass.
- Do not rewrite broad workflow areas that the evidence never touched.

---

## Validation

1. Check that every proposed correction maps back to documented workflow rules or repeated transcript evidence.
2. Check that every finding is stated in abstract, reusable terms.
3. Check that root-cause ownership is explicit: planner, groomer, shared docs, upstream skill, or template.
4. Check that the edits target the highest-level stable cause, not the latest symptom.
5. Check that the skill stayed inside the planning/grooming loop and did not silently expand into later workflow phases.
6. Check that a future groomer PASS would be harder to obtain with the same missed blocker still present.

---

## File-loading rules

- Always read `WORKFLOW.md`, `feature-planner`, and `plan-groomer`.
- Read `AGENTS.md` when role ownership, checkpoint ownership, or execution gating may be involved.
- Read `systems-architect` when defects mention multi-workstream seams, frozen interfaces, or architecture consultation.
- Read specialist skills only when the planner could not have produced a correct plan without their domain guidance.
- Read `code-reviewer` and `spec-validator` only when the planning loop is being confused with later review loops.
- Read transcripts, session history, or pasted reviews only for the sessions under analysis.

---

## Output structure

Use this structure:

1. **Evidence set** — what history and docs were examined.
2. **Abstract defect clusters** — the reusable failure classes.
3. **Root-cause attribution** — which artifact owns each class and why.
4. **Edits to make** — the skill/doc/template files that should change.
5. **Edits made** — what was updated in this run.
6. **Open questions** — only unresolved policy choices, not implementation details.

---

## Evolution

Revisit after workflow automation begins.

Likely future splits:
- `planning-loop-retrospector` for planner↔groomer only (this skill — in use),
- `review-loop-retrospector` for implementor↔reviewer loops (see [[review-loop-retrospector]] — created),
- `checkpoint-loop-retrospector` for human rejection and retry/pivot handling (not yet created).

Likely future upgrades:
- direct planner↔groomer handoff rules,
- multi-groomer consensus or escalation policy,
- automation-aware evidence gathering,
- metrics from repeated defect classes once enough history exists.
