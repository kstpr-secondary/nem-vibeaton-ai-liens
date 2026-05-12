---
name: spec-validator
description: Use when the current feature docs need checking against code, or when a task/phase asks whether implementation matches the written expectations.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: project
  version: "2.0"
  role: spec-validator
  activated-by: human-or-task
---

# Spec Validator

Check spec-to-code fidelity for the current feature docs. Read the active feature document, task/phase notes, and any linked architecture or interface docs, then compare the written expectations to the implementation.

This skill answers one question: what is implemented, what is missing, what is partial, and what is still unclear?

The current feature docs may be a structured folder under `specs/<feature>/` or a single combined markdown document with sections for diagnostics/research, design, phases, tasks, and checkpoints.

## Objective

Given a feature spec, phase, or task:

1. Read the relevant markdown docs.
2. Turn each concrete requirement into a short checklist. For tasks that name a specific algorithm, add two implicit requirements to the checklist even if the spec doesn't state them explicitly: (a) the implementation uses the named algorithm, not a functionally-equivalent substitute that would fail on the algorithm's known degenerate inputs; (b) the standard edge cases for that algorithm class are handled.
3. Inspect the code only enough to map each requirement to `file:line`, or note `MISSING`, `PARTIAL`, or `UNCLEAR`.
4. For each gap, propose a brief fix — a short prose or inline-code description of what the implementor should add or change to close it. Fixes are advice in the report, not file edits.
5. **Complete the full checklist in a single pass** — do not stop after finding the first defect. All gaps across all requirements must appear in one report. Incremental passes that each surface a subset of the defects multiply the review cycle without improving coverage.
6. Return a concise verdict with the gaps and fix suggestions.
7. If the writing is ambiguous, stop and ask for clarification instead of guessing.

## Scope

**In scope**
- Current feature markdown docs.
- Phase descriptions, task descriptions, checkpoints, and acceptance notes.
- Linked architecture or interface docs when the task points to them.

**Out of scope**
- Bug hunting, memory safety, style, or performance review.
- Editing code.
- Rewriting the spec to match the code.

## Rules

- Prefer the exact task/phase/checkpoint text that defines the behavior.
- If the feature is documented in one combined markdown file, read only the sections relevant to the validation target.
- One requirement should map to one concrete code location or a clear gap.
- If a requirement cannot be stated clearly, mark it ambiguous and stop.
- **Never write, edit, or create any file.** Output a written report only.

---

## What this role does NOT do

- **Does not edit code** — not patches, not diffs, not corrections. Fix suggestions live in the report as prose or short inline snippets; the implementing agent applies them.
- Does not write, create, or delete any file of any kind, including tests, headers, build files, or documentation.
- Does not rewrite or update the spec to match the code.
- Does not expand scope beyond the spec and tasks the human pointed to.
