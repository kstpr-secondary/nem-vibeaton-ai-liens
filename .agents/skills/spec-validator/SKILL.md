---
name: spec-validator
description: Use when a task in `TASKS.md` carries `Validation: SPEC-VALIDATE` or `SPEC-VALIDATE + REVIEW`, when an entry lands in `_coordination/queues/VALIDATION_QUEUE.md`, when a milestone is ready to merge and its `Expected Outcome` / acceptance criteria must be confirmed, or when a human asks "did this implement the spec?" / "does the code match the interface?" / "is anything missing from the spec?". Also use when a task touches a frozen interface (`docs/interfaces/*-interface-spec.md`) and the change must be audited before merge. The role answers one question — **did this code implement what the referenced spec actually says, no more and no less** — and writes a verdict to `_coordination/<workstream>/VALIDATION/<task-id>.md`. Do NOT use for risk / bug / style review (that is `code-reviewer`), for writing tests (`test-author`), for rendering correctness / visual output (human behavioral check per Blueprint §8.6), or for deciding whether the spec itself is right (that is Systems Architect). Domain knowledge is NOT part of this skill — load `renderer-specialist`, `engine-specialist`, `game-developer`, or the relevant library SKILL alongside when the spec uses domain terms you need to interpret.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: spec-validator
  activated-by: human-or-queue
---

# Spec Validator

Checks **spec → code fidelity**, fast. One question only: every requirement in the referenced spec must be either implemented in the declared files, explicitly deferred on the task, or flagged as a gap. Nothing else is in scope — not code quality, not performance, not visual output.

Expert in: reading `TASKS.md` rows, locating the referenced spec (interface / architecture / milestone acceptance), mechanically mapping spec bullets to code symbols, and writing a compact verdict file. **Not** expert in rendering, ECS, physics, or any library — load the specialist SKILL alongside when the spec wording needs domain interpretation.

---

## 1. Objective

Given a task whose `Validation` column includes `SPEC-VALIDATE` (or a milestone-level validation request):

1. Extract the spec's requirements as a **numbered checklist**.
2. For each item, locate the implementing symbol (`file:line`) or mark it **MISSING** / **PARTIAL** / **DEFERRED**.
3. Check frozen-interface invariants (`docs/interfaces/*`) and the task's declared `files:` set (AGENTS.md §7).
4. Write the verdict to `_coordination/<workstream>/VALIDATION/<task-id>.md` (template in §6).
5. Transition `Status` per AGENTS.md — `READY_FOR_REVIEW` on PASS, `BLOCKED` on FAIL, leave `READY_FOR_VALIDATION` untouched on NEEDS-CLARIFICATION and log in `BLOCKERS.md`.

Time budget: **~10 min per task, ~20 min per milestone**. If you pass that, stop and log in `_coordination/overview/BLOCKERS.md` — validation must not block the critical path.

---

## 2. Scope

**In scope**
- Task-level spec checks: every acceptance bullet on the task's `Depends_on` spec is present in the diff.
- Milestone-level checks: every bullet under the milestone's `Expected Outcome` has at least one implementing task marked `DONE` and verifiable in code.
- Frozen-interface diffing: signatures, struct layouts, enum values in `docs/interfaces/*-interface-spec.md` match the headers under `src/<workstream>/include/` (or equivalent public-API location).
- Parallel-group discipline: edited files ⊆ task's declared `files:` set.
- Writing the verdict file and updating `Status`.

**Out of scope**
- Bug hunting, memory safety, threading, naming, style — route to `code-reviewer`.
- Build / compile status (G1) and tests (G2) — implementing agent's gate.
- Rendering correctness, shader visuals, gameplay feel — human behavioral check (Blueprint §8.6).
- Editing code to fix gaps. Spec Validator **reports**, never patches.
- Editing the spec to match the code. If the spec is wrong, escalate in `BLOCKERS.md` — Systems Architect decides.
- Deciding whether a scope cut (Blueprint §13.1) was legitimate — flag it, human approves.

---

## 3. What counts as "the spec" for this task

Resolve in this order; stop at the first match:

1. `Depends_on` in the task row — if it points at `docs/interfaces/<ws>-interface-spec.md#<anchor>` or a SpecKit section, **that** is the spec.
2. The milestone's `Expected Outcome` line in the workstream `TASKS.md` (for milestone-level runs).
3. The workstream architecture doc (`docs/architecture/<ws>-architecture.md`) — only for the subsystem the task touches.
4. The Master Blueprint — only §3.3 (shaders), §3.5 (build targets), §3.6 (asset paths), §4–§6 (workstream scopes), §13 (quality gates). Do **not** pull in §7–§11 as requirements; those govern process, not code.

If none of the above names a concrete acceptance criterion, the task is **NEEDS-CLARIFICATION** — do not invent one.

---

## 4. Default workflow

1. **Read the task row.** Capture `ID`, `Milestone`, `Depends_on`, `Validation`, `Notes.files`, and expected `Status` transition.
2. **Resolve the spec** per §3. Quote only the relevant section (typically 10–40 lines).
3. **Extract acceptance as a numbered list.** One bullet = one check. Rewrite vague prose into testable statements before validating (e.g., "supports directional lighting" → "`sg_directional_light_t` struct defined in `renderer.h` with `direction`, `color`, `intensity`").
4. **Diff the code** against the list. For each item, find the implementing symbol via `grep` / file read and record `file:line`, or mark `MISSING` / `PARTIAL` / `DEFERRED (<task-id>)`.
5. **Check the invariants** in §5.
6. **Write the verdict file** per §6.
7. **Transition status** per §1 step 5. Never self-claim; only move forward / back.
8. If a spec line is ambiguous, **do not guess** — NEEDS-CLARIFICATION + blocker entry naming the exact sentence.

---

## 5. Invariants (always check)

- **Frozen interfaces.** Any change to `docs/interfaces/*-interface-spec.md` or to a public header it governs, without an approved `Depends_on` to a new spec version, is an automatic FAIL (AGENTS.md §10 rule 3, gate G3).
- **File-set discipline.** `git diff --name-only` ⊆ the task's declared `files:`. Violations are a FAIL unless `BLOCKERS.md` already records the human-approved expansion (AGENTS.md §7).
- **Asset paths.** No relative asset paths introduced — all runtime file loads compose from `ASSET_ROOT` (Blueprint §3.6). Shaders are compiled into headers and need no path.
- **Shader robustness.** If the task touches shader creation, a failure-log + magenta fallback path exists (Blueprint §3.3 / gate wording in Iter 9, item 18). No unconditional `assert` / `abort` on shader creation.
- **Build-target scope.** The task only adds sources to the target its workstream owns. Top-level `CMakeLists.txt` changes require Renderer / Systems Architect ownership (AGENTS.md §10 rule 11) — otherwise FAIL.
- **Validation downgrade.** The task's `Validation` column was not silently weakened vs. the spec's risk level (AGENTS.md §10 rule 5).

An invariant failure is always enough to return FAIL, even if every numbered acceptance item passed.

---

## 6. Verdict file template

Write to `_coordination/<workstream>/VALIDATION/<task-id>.md` (create the dir if missing):

```markdown
# Spec Validation — <TASK-ID>

- Verdict: PASS | FAIL | NEEDS-CLARIFICATION
- Spec source: <path-with-anchor>
- Validator: <agent_name>@<machine>
- Date: <YYYY-MM-DD>

## Acceptance checklist
1. <requirement> — PASS (src/x.cpp:42)
2. <requirement> — MISSING
3. <requirement> — PARTIAL (src/y.cpp:17 — returns vec3 but spec says quat)
4. <requirement> — DEFERRED (R-27, approved in TASKS.md notes)

## Invariants (§5)
- Frozen interfaces unchanged: YES
- File-set discipline (files: ⊇ diff): YES
- ASSET_ROOT discipline: N/A
- Shader fallback present: N/A
- CMake scope: YES
- Validation level preserved: YES

## Gaps / blockers
- <one line per issue, cite file:line or spec line>

## Recommendation
- <one line: merge / fix gaps / escalate / clarify spec>
```

Keep the file under ~60 lines. If you need more, you are probably doing review, not validation — stop and hand off.

---

## 7. Decision rules

- **Unit of work = one numbered acceptance item.** If you cannot state a spec bullet as a testable sentence, it is NEEDS-CLARIFICATION, not PASS.
- **`grep` + a single targeted read** beats loading whole files. Look for the symbol the spec names; don't re-read the whole translation unit.
- **Absence is evidence.** A missing symbol is MISSING, even if "the rest of the diff looks fine."
- **"Desirable" ≠ required.** Blueprint §4/§5/§6 split MVP / Desirable / Stretch. Only MVP bullets are mandatory at milestone validation unless the task row explicitly promotes a Desirable item.
- **One bullet, one location.** If a requirement is half in `renderer.cpp` and half in `renderer.h`, cite both — PARTIAL is reserved for "the behavior is incomplete," not "it's split across files."
- **Never rewrite the spec to fit the code.** Log a BLOCKER and let the human decide.
- **Never expand scope.** If you notice an unrelated gap, note it in `Gaps / blockers` with a `follow-up:` prefix — do not block the current task on it.

---

## 8. Gotchas

- **`Depends_on` can be stale after a milestone merge.** If the referenced spec section was renamed, follow the git-log trail one hop, then flag as NEEDS-CLARIFICATION if still unresolved.
- **Mocks pass trivially.** If the diff only touches `src/<ws>/mocks/` (Blueprint §11.2), acceptance items about real behavior stay MISSING — validation against the mock file is meaningless. Mark `DEFERRED (mock phase)` only if the task row says so.
- **SpecKit artifacts under `docs/planning/speckit/` are precursors, not live specs** (Blueprint §8.9). Validate against `docs/interfaces/` + `_coordination/.../TASKS.md`, not the raw speckit `tasks.md`.
- **Interface specs are frozen at version level.** A patch version change may be allowed; a field removal is not. If the spec does not version itself, treat any signature change as frozen-breaking.
- **OpenGL 3.3 Core only** (Blueprint Iter 8 item 20). A task that introduces a GL 4.x feature or a Vulkan path FAILs even if acceptance items pass.
- **`sokol-shdc`-generated headers live under `${CMAKE_BINARY_DIR}/generated/shaders/`.** Do not flag missing shader files there as MISSING — they are build artifacts.
- **Parallel-group file sets are in `Notes`, not in the task title.** Always read `Notes: files: ...` before §5's file-set check.
- **"Smoke test passes" is not a spec item.** That is gate G2. Do not substitute it for a behavioral acceptance bullet.

---

## 9. Validation of the validator

Before marking the verdict file done:

1. Every numbered spec bullet has exactly one status (`PASS` / `MISSING` / `PARTIAL` / `DEFERRED`) with a citation where applicable.
2. Every §5 invariant is answered (`YES` / `NO` / `N/A`).
3. The verdict on line 3 matches the checklist: any `MISSING`, `PARTIAL`, or invariant `NO` ⇒ FAIL; any unlabeled bullet ⇒ NEEDS-CLARIFICATION.
4. No edits were made to code, specs, or `TASKS.md` schema — only the verdict file and the task's `Status` cell.
5. Citations resolve (`file:line` exists in the current tree; spec anchors exist in the current spec).

---

## 10. File loading (lazy)

- **Always:** `AGENTS.md`, this `SKILL.md`, the task row, and the specific spec section resolved in §3.
- **Domain context (load alongside when the spec uses domain terms):** `renderer-specialist/SKILL.md`, `engine-specialist/SKILL.md`, `game-developer/SKILL.md`, or a library SKILL — **one** of them, matching the workstream.
- **Code:** only the files listed in the task's `Notes: files:` and the public header they implement against.
- **Do not load:** full `sokol_gfx.h` / `entt.hpp` / `cgltf.h`, the full Blueprint, unrelated workstream `TASKS.md`, or SpecKit precursor folders unless §3 sent you there.

---

## 11. Evolution

Revisit when:
- The first frozen-interface change lands — capture the real diff pattern in §5 so future validators spot it faster.
- A real false PASS is found — add the missed check as a new §5 invariant or §8 gotcha.
- Milestone-level validation stabilizes — split §6 into per-milestone and per-task templates if they meaningfully diverge.
- A machine-readable spec format appears — replace §4 step 3 ("extract acceptance as numbered list") with a parse step.
