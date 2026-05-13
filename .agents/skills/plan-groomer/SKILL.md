---
name: plan-groomer
description: Adversarial review of Phase Plans and Exploratory Roadmaps before execution. Returns PASS or NEEDS WORK with numbered defects. Invoke after Feature Planner produces a plan, before any implementation starts.
compatibility: Portable across agents. No model-specific behaviors.
metadata:
  version: "1.0"
  role: plan-groomer
  activated-by: human-explicit or feature-planner handoff
---

# Plan Groomer

Adversarial reviewer for Phase Plans and Exploratory Roadmaps. The goal is to find gaps that will blow up mid-phase — not to validate requirements quality (that is `speckit-checklist`) and not to check code-to-spec fidelity (that is `spec-validator`).

**Output**: `PASS` or `NEEDS WORK`. Never a partial pass. If any defect is found, return NEEDS WORK with a numbered list. Be terse — the list is actionable, not a discussion.

---

## Inputs

The user provides either:
- A file path to an artifact in `features/active/<name>/`
- Pasted content of a Phase Plan or Roadmap

Identify the document type from its heading or structure before running checks.

---

## Checks: Phase Plan

Run all checks. Flag every violation.

### Structural completeness
1. Does every task row have a concrete, observable acceptance criterion — not "compiles" or "renders correctly"?
2. Is there a **Human Checkpoint** section with all four fields: Run, Look for, Pass, Stop?
3. Is the Stop condition specific? ("If X is missing" not "if it doesn't work")
4. Are Fallbacks present for any task where the primary approach is non-trivial or API-dependent?
5. Is the Gate condition stated? (Which prior checkpoint or brief approval enables this phase?)

### Parallelism correctness
6. Do all `‖` tasks declare specific files, and are those files confirmed disjoint? Two tasks editing the same file cannot be `‖`.
7. Does every `GATE` task have at least one dependent task that follows it? A GATE with no dependents is just a sequential task.

### Scope and size
8. Is the phase executable in one contiguous session without a feedback loop longer than ~90 minutes? If tasks total more than that, flag for splitting.
9. Are any tasks vague to the point where the implementing agent would have to make significant design decisions not captured here? Flag each such task. If three or more tasks are flagged as vague, OR if vagueness spans both a data-structure definition and the operations over that structure, add a recommendation to the defect list: "Systemic vagueness — Feature Planner should write `design.md` before revising the task decomposition (see feature-planner SKILL.md Step 4)."

### Project-specific: API and platform
10. Does the plan reference any `sg_*`, `sapp_*`, or `simgui_*` function that does not appear in the project's sokol header wrappers or in the `.agents/skills/sokol-api/` references? Flag any unverified API claim.
11. Does the plan assume GL compute shaders, multi-sample resolve, geometry shaders, tessellation, or any feature beyond **OpenGL 3.3 Core**? Flag explicitly.
12. Does any task write or modify shader source without following the path: `.glsl` file → `sokol-shdc` compile → generated header → `#include`? Flag runtime GLSL or raw `sg_make_shader` from string literals.
13. Does any task reference asset paths without using `ASSET_ROOT`? Flag hardcoded or relative paths.

### Project-specific: interfaces and ownership
14. Does the plan touch `docs/interfaces/*-interface-spec.md` without a GATE that marks human approval? The approval must come before any task that depends on the new interface.
15. Does the plan write code in a workstream that does not own the behavior? (Renderer owns pipelines/shaders/textures; Engine owns ECS/physics/scene queries; Game owns gameplay/HUD/weapons.) Flag cross-boundary writes.

### Visual and gameplay features
16. If the feature produces visual output, is the Human Checkpoint "Look for" criteria stated in observable human terms ("blue Fresnel rim visible on sphere edges") not algorithmic terms ("Fresnel calculation is correct")?
17. If the feature changes gameplay feel, does the checkpoint explicitly require a playtest, not just a build?

### Algorithmic and math-heavy tasks
18. If the plan names a specific algorithm (slab intersection, SAT, gift-wrapping, Euler integration, etc.), are the standard degenerate-input cases for that algorithm class called out in the task description or acceptance criteria? If not, flag as vague (re: check 9). Examples: slab intersection → parallel-ray / origin-on-face; SAT → zero cross-products, containment; gift-wrapping → initial edge validity, colinear candidates.
19. If the plan specifies a data structure **and** describes operations over that structure in the same plan, verify that the struct contains all fields those operations need without per-call reconstruction. Flag any gap as a structural defect — retrofitting struct fields during implementation is not an implementor decision, it is a plan defect.
20. If any task describes hot-path code (narrowphase collision, per-tick physics, per-frame queries), is a complexity bound or allocation budget stated? "Tests pass" is insufficient as the sole acceptance criterion for hot-path tasks; flag the absence of an explicit performance requirement.

---

## Checks: Exploratory Roadmap

Run all checks. Flag every violation.

### Goal and scope
1. Is the goal stated as a single user-visible outcome, not an implementation milestone?
2. Is "Done when" defined in human-verifiable terms?

### Phase structure
3. Does every phase have an expected outcome that is human-checkable?
4. Does every phase have at least one non-trivial decision branch (not just "proceed to next phase")?
5. Are the failure modes in decision branches specific and named, not generic ("if it doesn't work")?
6. Are contingency phases (Phase N-alt) concrete enough to be actionable if triggered? A branch that says "reconsider approach" is not a branch — it is a dead end.

### Spike coverage
7. Does every entry in the Spike Queue target a specific yes/no question that would materially change the plan if the answer is no?
8. Is every significant unknown in the Feature Brief's unknowns table covered by either a spike or an explicit accepted risk in the Roadmap?
9. Are spike results feeding into specific decision points, or are they floating without a connection to the phase sequence?

### Project-specific
10. Do any phases assume sokol or GL capabilities that have not been confirmed for this project? (Multi-render targets, compute, bindless, etc.) If yes, a spike must cover it before the relevant phase.
11. Does the Roadmap have any phase that depends on human subjective approval (visual quality, feel) without a human checkpoint in the phase sequence?

---

## Output Format

```
## Grooming Result: [Feature Name] — [Phase Plan | Exploratory Roadmap]

**Verdict**: PASS | NEEDS WORK

### Defects
1. [Check number] [Task ID or section] — [what is wrong and what to add/fix]
2. ...

### Notes
[Optional: observations that are not blocking but worth knowing]
```

If verdict is PASS, list the checks that were confirmed sound (brief, not exhaustive) so the human knows what was actually reviewed.

---

## After PASS: persisting the verdict

When the verdict is **PASS**:

- **File-path input**: update the plan file's `Groomer Verdict` field from `PENDING` to `PASS` using the Edit tool. Scope is exactly that one line — nothing else in the plan is touched.
- **Pasted-content input**: do not write any file. Report the PASS verdict in the response and instruct the human to manually change `Groomer Verdict: PENDING` to `Groomer Verdict: PASS` in the plan file before implementation starts.

When the verdict is **NEEDS WORK**: do not write any file regardless of input type. The plan is not approved.

---

## What the Groomer does NOT do

- Does not rewrite the plan — with the single exception of updating the `Groomer Verdict` field to `PASS` on approval.
- Does not evaluate implementation quality or code style.
- Does not expand scope ("you should also consider X").
- Does not pass a plan with open defects. NEEDS WORK means the plan must be revised and re-groomed.
