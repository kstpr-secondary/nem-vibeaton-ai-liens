# Code Reviewer SKILL.md — Compaction Opportunities

_Analysis of gaps and refinements for `.agents/skills/code-reviewer/SKILL.md`._

---

## 1. Milestone Outcome Cross-Reference

**Location:** §1 (Objective) or §2 (Scope)

**Issue:** The skill says "missing functionality visible from the diff" but has no guidance on determining what *should* be there beyond the diff itself. A reviewer checking a milestone merge needs to know what the milestone was supposed to deliver.

**Suggested addition (§1, after current step 4):**

> Before starting: read the task's `Expected Outcome` from `TASKS.md`. Use it as a checklist — if the diff doesn't deliver something listed in the outcome, flag it as a WARNING ("possible missing functionality").

---

## 2. ECS Pattern Checklist

**Location:** §7.1 (C++ defect checklist)

**Issue:** The skill mentions "`entt` entity stored after `registry.destroy" but doesn't cover common ECS anti-patterns relevant to this project:

- Iterating over a registry while modifying it (adding/removing entities or components mid-iteration)
- Using stale component/entity handles across function boundaries
- Misusing views/iterators (e.g., iterating a view that becomes invalid after a component assignment)
- Storing entity IDs without verifying they still exist before use

**Suggested addition (§7.1, new bullet):**

> **ECS lifecycle.** Iterating a registry or view while adding/removing entities or components mid-iteration; storing entity/component handles across ticks without validation; `entt` iterator invalidated by structural change; using `entity == entt::null` after a destroy but before the next tick.

---

## 3. Shader Fallback — Refactored-Code Coverage

**Location:** §5 (Invariants)

**Issue:** The shader fallback check says to verify it's present on *new* shader/pipeline creation, but doesn't address the case where existing pipeline code is refactored and the fallback path is accidentally removed.

**Suggested edit to §5, shader fallback line:**

> **Shader fallback.** Any new or refactored shader / pipeline creation has the magenta-error fallback path; no `assert` / `abort` / unchecked return on `sg_make_shader` / `sg_make_pipeline`; existing fallback paths preserved when surrounding code changes (Blueprint §3.3). → BLOCKER.

---

## 4. sokol-shdc Annotation Validity

**Location:** §7.2 (GLSL) or §9 (Gotchas)

**Issue:** The skill covers uniform/attribute mismatches but doesn't explicitly flag missing or incorrect `@module`, `@program`, `@block` annotations that would cause sokol-shdc build failure or silent wrong rendering. When the `sokol-shdc` SKILL is loaded alongside, this is covered — but a standalone reviewer won't catch it.

**Suggested addition (§7.2, new bullet):**

> **Annotation validity.** Missing or incorrect `@module`, `@program`, `@block` (or `@layout`) annotations that would cause `sokol-shdc` build failure; `@block` member order or type mismatch with C++ struct (sokol-shdc reflects C side from annotations — divergence = silent wrong rendering).

**Or as a gotcha (§9, new entry):**

> **Standalone reviewer blind spot.** If the `sokol-shdc` SKILL is not loaded alongside, do not attempt to validate annotation syntax (`@module`, `@program`, `@block`). Only flag obvious mismatches (e.g., no annotations at all on a shader that should have them). Defer full annotation validation to the specialist.

---

## 5. File Loading — Skill Dependency Clarification

**Location:** §10 (File loading)

**Issue:** The skill says "at most one library SKILL the diff exercises" but doesn't clarify what happens when a diff touches multiple libraries (e.g., both sokol and entt). In practice, the reviewer may need two library SKILLs loaded.

**Suggested edit (§10, domain context bullet):**

> **Domain context (load alongside when judging API usage):** `renderer-specialist` / `engine-specialist` / `game-developer` SKILL — one matching the workstream — and one or more library SKILLs the diff exercises (e.g., both `sokol-api` and `entt-ecs` if the diff touches GPU buffers and ECS together).
