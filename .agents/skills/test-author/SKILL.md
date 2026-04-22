---
name: test-author
description: Use when a task or queue entry asks for Catch2 unit tests covering engine math, physics integration, collision primitives (AABB-vs-AABB, ray-vs-AABB), impulse response, quaternion / view-projection composition, ECS component lifecycle, renderer mesh-builder vertex/index counts, frustum-plane math, or any other pure-function logic in `src/renderer/` or `src/engine/`. Also use when populating `_coordination/queues/TEST_QUEUE.md` entries, wiring a new test file into the `renderer_tests` or `engine_tests` Catch2 executables, or a milestone's `Validation` column requests a testable math/physics surface. Indirect triggers include "write a quick test for X", "cover the physics step with Catch2", "add a raycast test", "smoke-test the Euler integrator". Do NOT use for rendering correctness (human behavioral check only — Blueprint §8.6), game-layer gameplay (no `game_tests` target exists), shader validation, integration tests that need a live GL context, or cross-workstream planning. Domain knowledge is NOT part of this skill — load `engine-specialist`, `renderer-specialist`, or a library SKILL alongside when test targets require domain understanding.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: test-author
  activated-by: human-or-queue
---

# Test Author

Writes **small, fast Catch2 tests for the few functions that genuinely matter**. The hackathon does not lean on tests; this role exists to cheaply prevent regressions in math, physics, and ECS logic that are hard to eyeball. Rendering, shaders, asset I/O, and gameplay are covered by human behavioral check — **not** by this role.

Expert in: Catch2 v3 usage (TEST_CASE, SECTION, REQUIRE, CHECK, `Catch::Approx`), float-compare hygiene, compact test files. **Not** expert in rendering, physics derivation, or ECS design — those belong to the specialist SKILL loaded alongside.

---

## 1. Objective

Given a task or `TEST_QUEUE.md` entry pointing at a function:

1. Decide whether it is worth testing at all (§3). If not, say so and stop.
2. If yes, write **just enough** Catch2 tests — typically one `TEST_CASE` with 2–4 `SECTION`s, under ~40 lines.
3. Wire the file into `renderer_tests` or `engine_tests` only if the CMake setup doesn't pick it up automatically.
4. Build and run the targeted test binary; confirm the new tests pass.
5. Update `Status` per `AGENTS.md` (tasks are human-claimed; this role transitions to `READY_FOR_REVIEW` or `DONE` based on the task's `Validation`).

Time budget: **~10 min per test task**. If you pass 15 min, stop and log a blocker — tests must not be the critical path.

---

## 2. Scope

**In scope**
- Unit tests under whichever `src/<workstream>/tests/` (or equivalent) layout the `_tests` target already uses.
- Catch2 v3: `TEST_CASE`, `SECTION`, `REQUIRE`, `CHECK`, `Catch::Approx`, `[tag]` filters, `Catch2::Catch2WithMain`.
- Minimal smoke tests for parsers, math helpers, and pure-function API surfaces.
- Appending a single source line to an existing `target_sources(... PRIVATE ...)` list when needed.
- Entries in `_coordination/queues/TEST_QUEUE.md` and status updates for test tasks.

**Out of scope**
- Rendering correctness, shader validation, pipeline tests (magenta fallback already guards failure).
- Game-layer gameplay and AI (no `game_tests` target).
- Integration tests that require a live `sokol_app` window or GL context.
- Re-architecting `CMakeLists.txt` — co-owned by Renderer / Systems Architect with 2-minute notice (`AGENTS.md` §10 rule 11). Flag, don't edit.
- Benchmarks, property-based generators, fuzzing, coverage tooling.
- Mocking `entt`, `cgltf`, or `sokol_gfx` internals.

---

## 3. What to test (decision rule)

**Worth testing:**
- Pure math: view/projection composition, quaternion → matrix, AABB-vs-AABB overlap, ray-vs-AABB intersection, impulse-based collision response, frustum-plane extraction, containment reflection, energy-cap rule.
- Euler integration step (inputs → expected position/velocity after dt).
- ECS slices with a clean pure-function view: component add/get/remove counts, view iteration yields the expected set.
- Mesh-builder invariants: `make_sphere_mesh` / `make_cube_mesh` vertex and index counts for given subdivisions; consistent winding.
- glTF/OBJ tiny-fixture parse: vertex count on a known small model.

**Not worth testing (skip; note on task):**
- Anything requiring a live `sokol_app` / GL context or real GPU resources.
- Pipeline creation, shader binding, draw calls.
- Game-layer scripts, AI, weapon cooldowns, HUD rendering.
- Trivial getters, one-line pass-throughs, config structs.
- Integration paths that depend on multiple merged workstreams at once — wait for the behavioral check at the merge.

Borderline cases: prefer **skipping** over writing a shaky test. One missing test costs less than a flaky one.

---

## 4. Default workflow

1. **Read** the task row and `TEST_QUEUE.md` entry (if any). Identify the exact symbols and source file.
2. **Apply §3.** If not worth testing, comment on the task and stop.
3. **Check existing tests** under `src/<workstream>/tests/` (open the workstream `CMakeLists.txt` once to confirm layout and whether sources are globbed). Follow the existing file-naming and tag convention.
4. **Write the test file** (skeleton in §6). Keep it under ~40 lines.
5. **Wire into the `_tests` target** only if the CMake setup does not use a glob.
6. **Build and run** target-scoped per Blueprint §3.5:
   ```bash
   cmake --build build --target renderer_tests   # or engine_tests
   ./build/<path>/renderer_tests "[your-tag]"
   ```
7. **Update task status.** Do not self-claim; only transition status.
8. If a test reveals a real bug, **do not silently fix production code** — log on the task or in `_coordination/overview/BLOCKERS.md` and hand back to the specialist.

---

## 5. Catch2 v3 essentials

Headers (v3 layout — **not** the v2 `catch.hpp`):
```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>   // only when comparing floats
using Catch::Approx;                  // Approx is namespaced in v3
```
`Catch2::Catch2WithMain` is linked by the `_tests` target → no manual `main`.

- `REQUIRE(cond)` — abort on failure (invariants).
- `CHECK(cond)` — record failure, keep going (multiple expectations).
- Floats: `REQUIRE(x == Approx(1.0f).margin(1e-5f))`. Use `margin` near zero, `epsilon` elsewhere. Never compare floats with `==`.
- `SECTION`s re-run the enclosing `TEST_CASE` body from the top — don't share mutable state across sections via statics or lambdas.
- Tags: at minimum `[<workstream>][<subsystem>]`, e.g. `[engine][physics]`, `[renderer][mesh]`.

---

## 6. Default skeleton

```cpp
// src/engine/tests/test_raycast.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/raycast.h"
using Catch::Approx;

TEST_CASE("ray-vs-AABB", "[engine][collision]") {
    const AABB box{ {0,0,0}, {1,1,1} };

    SECTION("ray hits +X face at t=1") {
        auto hit = ray_vs_aabb({-2,0,0}, {1,0,0}, box, 10.0f);
        REQUIRE(hit.has_value());
        CHECK(hit->distance == Approx(1.0f).margin(1e-5f));
    }
    SECTION("parallel ray misses") {
        auto hit = ray_vs_aabb({-2,2,0}, {1,0,0}, box, 10.0f);
        REQUIRE_FALSE(hit.has_value());
    }
}
```
Four to six `SECTION`s per `TEST_CASE` is a **ceiling**, not a target.

---

## 7. Decision rules

- **One focused `TEST_CASE` with a few `SECTION`s** beats many flat `TEST_CASE`s.
- **Hand-picked inputs** beat generators — generators are overkill at this time budget.
- **`Catch::Approx` for every float compare.** Equality on floats is a bug.
- **Skip over flaky coverage.** A sometimes-failing test is net negative this hackathon.
- **Never edit production code to make a test pass** beyond the task's scope. Exposed defect → report it.
- **Never add a top-level CMake target or a FetchContent dep** — escalate to Renderer / Systems Architect.
- **Escalate** any request to unit-test rendering correctness, shader output, or functions needing a GL context — Blueprint §8.6 routes those to behavioral check.

---

## 8. Gotchas

- **Catch2 v3 headers**: `catch2/catch_test_macros.hpp`, never `catch2/catch.hpp` (v2).
- **`Catch::Approx` is namespaced** — `using Catch::Approx;` or fully qualify.
- **Test-target layout varies** — check `renderer_tests` / `engine_tests` in the workstream `CMakeLists.txt` before adding a file (glob vs explicit list).
- **No `game_tests` by design.** Game test requests → refuse, redirect to behavioral check (unless human reopens the decision).
- **Keep `#include`s narrow** — reach only the pure-math header under test; do not transitively drag in sokol / GL.
- **GLM quaternion conventions**: ctor `glm::quat(w, x, y, z)` but `vec4` view `(x, y, z, w)`. Verify which the tested code uses.
- **Fixture paths** must go through `ASSET_ROOT` (Blueprint §3.6) — never relative.
- **Tests run primarily on `rtx3090`.** Do not require a specific GPU or GUI.

---

## 9. Validation

Before marking a test task `DONE` / `READY_FOR_REVIEW`:

1. The relevant `_tests` target builds clean (G1).
2. Tag-filtered run prints expected assertion count and passes (G2).
3. All float comparisons go through `Catch::Approx`.
4. Only files inside the task's declared `Notes: files:` set were touched (`AGENTS.md` §7).
5. No production code changes unless explicitly in scope.
6. Any exposed bug is logged as a blocker or follow-up task — not silently patched.

---

## 10. File loading (lazy)

- **Always:** `AGENTS.md`, this `SKILL.md`, the task row (and queue entry if any).
- **Domain context:** `engine-specialist/SKILL.md` for engine tests, `renderer-specialist/SKILL.md` for renderer tests.
- **Target under test:** only the single header (`raycast.h`, `physics.h`, `frustum.h`, `mesh_builders.h`, …).
- **Do not load:** full `sokol_gfx.h`, `entt.hpp`, `cgltf.h`, or the full Blueprint. Only Blueprint §3.1, §3.5, §8.6, §13 are load-bearing.

---

## 11. Evolution

Revisit when:
- The `_tests` CMake layout stabilizes post-R-M0 — lock file paths and linking recipe into §4/§6.
- A first real bug is caught by a test — add a pointed gotcha so agents write the same guard faster next time.
- A `game_tests` target is added (Blueprint reversal) — extend §2 and §3.
- A common mock appears (e.g., a `renderer` mock for engine tests) — document its location and toggle here.
