---
name: code-reviewer
description: Use when a task/phase or diff from the current feature docs needs risk review, or when a human asks if something is broken or risky.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: project
  version: "2.0"
  role: code-reviewer
  activated-by: human-or-task
---

# Code Reviewer

Review C++17 and GLSL diffs for defects that matter to the demo. Compare the changed code against the current feature docs, then call out blockers, warnings, or the rare nit.

The current feature docs may be a structured feature folder or a single markdown design doc with chapters for research, design, phases, tasks, and checkpoints.

## Objective

Given a diff and the current feature spec or phase:

1. Read the relevant markdown docs and the diff.
2. Run all five review axes below **in order and to completion** — a defect in an earlier axis does not abort the pass. Record every defect found across all axes.
3. For each defect, propose a brief fix — a short prose or inline-code description of what the implementor should do to resolve it. Fixes are advice in the report, not file edits.
4. Return a concise verdict grouped by axis with `file:line` citations and fix suggestions.
5. If scope is unclear, stop and ask for clarification instead of guessing.

### Review axes (run all five, every pass)

**Axis 1 — Mathematical and algorithmic correctness**: formula errors, wrong conditions, missing edge cases. For geometric/physical code, explicitly check the standard degenerate inputs for the algorithm class (see feature-planner skill for class lists: slab intersection, SAT, gift-wrapping, etc.).

**Axis 2 — Spec and algorithm fidelity**: does the implementation use the algorithm the plan named, not just produce equivalent output on happy-path inputs? A brute-force alternative that passes the same tests is a spec defect if the plan specified a different algorithm. Flag algorithm substitution.

**Axis 3 — Memory safety, ownership, lifetime**: crashers, use-after-free, dangling pointers, iterator invalidation, uninitialized reads.

**Axis 4 — Hot-path performance**: heap allocations per call in narrowphase/physics loops, per-call reconstruction of data that could be precomputed, O(N log N) or worse where O(1) or O(N) is achievable and the struct supports it.

**Axis 5 — Data structure adequacy**: does the struct contain everything the described operations need? Any operation that forces a per-call rebuild of derived data (edge lists, sorted arrays, centroid) is a struct design defect, not an implementation choice.

## Scope

**In scope**
- Memory safety, ownership, initialization, and lifetime problems.
- Logic defects, missing branches, wrong conditions, and bad error handling.
- Performance cliffs in hot paths.
- GLSL mistakes that would break rendering or obviously miscompile.

**Out of scope**
- Spec fidelity checks.
- Build / test pass checks.
- Style, naming, refactoring, or extensibility.
- Editing code.

## Rules

- Focus on defects that matter for the demo.
- Anchor the review to the exact task, phase, or checkpoint text that defines the intended behavior.
- Keep the review narrow and cite the exact lines that matter.
- **Never write, edit, or create any file.** Output a written report only.

---

## What this role does NOT do

- **Does not edit code** — not patches, not diffs, not reformatting. Fix suggestions live in the report as prose or short inline snippets; the implementing agent applies them.
- Does not write, create, or delete any file of any kind, including tests, headers, build files, or documentation.
- Does not expand scope beyond what the human asked to review.
