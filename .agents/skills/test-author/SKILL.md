---
name: test-author
description: Use when a class, function, or task/phase from the current feature docs needs small Catch2 coverage.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: project
  version: "2.0"
  project-stage: post-hackathon features
  role: test-author
  activated-by: human-or-task
---

# Test Author

Write small, fast Catch2 tests for deterministic code that is worth testing. Focus on pure math, physics, ECS logic, mesh builders, parsers, and other helper code that can be checked without a live renderer or game loop.

Rendering correctness, gameplay feel, shader output, and live GL behavior stay with human checks.

The current feature docs may be:

- a structured feature folder under `specs/<feature>/`, or
- a combined markdown design doc with chapters like research, design, phases, and tasks.

## Objective

Given a class or a task/phase:

1. Find the exact symbol(s) and file(s) in the current feature docs.
2. Decide whether the behavior is worth testing.
3. If yes, write one focused `TEST_CASE` with a few `SECTION`s.
4. Wire it into the existing `renderer_tests` or `engine_tests` layout only if needed.
5. Build and run the targeted test binary.

## Scope

**In scope**
- `src/renderer/tests/` and `src/engine/tests/` Catch2 tests.
- Pure math, ECS lifecycle, collision helpers, mesh-builder invariants, parser helpers.
- `Catch::Approx` for float comparisons.

**Out of scope**
- `game_tests` (there is no game test target).
- Rendering correctness, shader validation, live `sokol_app` integration.
- Re-architecting `CMakeLists.txt`.

## Rules

- Read the task/phase text that actually defines the behavior before writing tests.
- Keep tests short and focused.
- Prefer one `TEST_CASE` with a few `SECTION`s.
- Use the narrowest headers possible.
- Do not edit production code just to make a test pass.
- If the behavior is not worth testing, say so and stop.

## Validation

- The targeted `_tests` binary builds.
- The new tests pass.
- Float compares use `Catch::Approx`.
- Only the files needed for the test were touched.
