---
name: spec-validator
description: Use when the current feature docs need checking against code, or when a task/phase asks whether implementation matches the written expectations.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: project
  version: "2.0"
  project-stage: post-hackathon features
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
2. Turn each concrete requirement into a short checklist.
3. Inspect the code only enough to map each requirement to `file:line`, or note `MISSING`, `PARTIAL`, or `UNCLEAR`.
4. Return a concise verdict with the gaps.
5. If the writing is ambiguous, stop and ask for clarification instead of guessing.

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
- Do not patch code or expand scope.
