# Spec Validator SKILL — Compaction Opportunities

Identified during review of `.agents/skills/spec-validator/SKILL.md` against AGENTS.md and the Hackathon Master Blueprint. These are opportunities to inline external references so the skill is self-sufficient during implementation without requiring a second document load.

---

## 1. Blueprint cross-references in §5 (Invariants) — HIGH PRIORITY

**Current:** The invariant section cites Blueprint sections for asset paths, shader robustness, and build-target scope.

**Why it matters:** During validation, both AGENTS.md and the skill are loaded, but the Blueprint is explicitly *not* loaded (§10 lazy loading). An agent following §10 will not have the Blueprint context needed to resolve these citations.

**What to inline:**

- **Asset paths** (currently cites Blueprint §3.6): *"All runtime file loads must compose from `ASSET_ROOT`; no relative asset paths. `ASSET_ROOT` is a `#define` in `${CMAKE_BINARY_DIR}/generated/paths.h`, set at configure time to `<project-source-root>/assets`."*
- **Shader robustness** (currently cites Blueprint §3.3 / Iter 9 item 18): *"Shader creation failures must log a message and fall back to a magenta error pipeline. No unconditional `assert` or `abort` on shader creation."*
- **Build-target scope** (currently cites AGENTS.md §10 rule 11, which is fine — keep that citation).

---

## 2. OpenGL 3.3 Core only — HIGH PRIORITY

**Current location:** §8 Gotcha, item citing "Blueprint Iter 8 item 20".

**What to inline:** *"OpenGL 3.3 Core only. A task that introduces GL 4.x features or a Vulkan path is an automatic FAIL."*

This is a hard code constraint, not a process rule, and the validator needs it readily available.

---

## 3. MVP vs Desirable vs Stretch — MEDIUM PRIORITY

**Current location:** §7 Decision rules, item citing "Blueprint §4/§5/§6".

**What to inline:** *"MVP items are mandatory at milestone validation. Desirable and Stretch items are not, unless the task row explicitly promotes them."*

This is core to the validator's decision of what counts as PASS vs acceptable deferment.

---

## 4. ASSET_ROOT macro — MEDIUM PRIORITY

**Current location:** §5 mentions `ASSET_ROOT` by name but never defines it or tells the validator how to verify its presence.

**What to add:** A brief definition and verification step:

- `ASSET_ROOT` is defined in `${CMAKE_BINARY_DIR}/generated/paths.h` as `PROJECT_SOURCE_ROOT "/assets"`.
- To verify: check that `paths.h` exists under the build directory and the macro is defined. Source files should not hard-code relative paths; they must compose from `ASSET_ROOT`.

---

## 5. sokol-shdc generated headers — MEDIUM PRIORITY

**Current location:** §8 Gotcha mentions `${CMAKE_BINARY_DIR}/generated/shaders/` but only to say "don't flag them as missing."

**What to add for tasks touching shaders:** *"When a task touches shader creation, verify the CMake custom command for `sokol-shdc` exists in the relevant target's build rule and that the generated header is `#include`d by the source. Do not flag missing `.glsl.h` files under `${CMAKE_BINARY_DIR}/generated/shaders/` as MISSING — they are build artifacts."*

---

## 6. Driver executables context — LOW PRIORITY

**Current location:** Not mentioned.

**What to add to §2 Scope (in scope) or §8 Gotcha:** *"Driver executables (`renderer_app`, `engine_app`) are standalone demo harnesses with hardcoded procedural scenes. They are expected to be small and are not shipped with the game. Validation of driver code should check for correctness against the workstream's public API, not production-quality patterns."*

---

## 7. Mock-to-real transition handling — MEDIUM PRIORITY

**Current location:** §8 Gotcha has one line about mocks passing trivially, but lacks a concrete verification step.

**What to expand into:** When validating a task that replaces mocks with real implementations:

1. Verify that mock files under `src/<ws>/mocks/` are deleted or disabled via CMake toggle.
2. Verify the real implementation exists in the declared source files.
3. If the diff still contains mock files alongside real ones and the task is about full replacement, mark as PARTIAL.

---

## 8. Blueprint §3 spec-resolution step — LOW PRIORITY

**Current location:** §3 step 4 says "Do **not** pull in §7–§11 as requirements; those govern process, not code."

**What to tighten:** Replace the vague exclusion list with an explicit inclusion list:

*"The Blueprint is a code requirement only for: shader annotation format (§3.3), build target topology (§3.5), and asset path policy (§3.6). Sections §7–§11 govern process, agent behavior, and coordination — not code."*

---

## Summary table

| # | Issue | Location | Priority | Effort |
|---|-------|----------|----------|--------|
| 1 | Blueprint refs in §5 invariants | §5 | HIGH | ~3 lines inline |
| 2 | OpenGL 3.3 Core only | §8 gotcha | HIGH | ~2 lines inline |
| 3 | MVP/Desirable/Stretch | §7 decision rules | MED | ~2 lines inline |
| 4 | ASSET_ROOT undefined | §5 invariant | MED | ~3 lines add |
| 5 | sokol-shdc build verification | §8 gotcha | MED | ~3 lines expand |
| 6 | Driver executables context | missing | LOW | ~2 lines add |
| 7 | Mock-to-real transition | §8 gotcha | MED | ~4 lines expand |
| 8 | Blueprint scope tightening | §3 step 4 | LOW | ~2 lines rewrite |

Total: ~21 lines of inlining/expansion. No structural changes needed — the skill is already well-organized.
