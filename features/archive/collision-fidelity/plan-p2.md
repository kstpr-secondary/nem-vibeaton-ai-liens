# Phase Plan: Collision Fidelity — Phase 2: Convex Hull Math Module

**Gate**: `checkpoint-p1.md` present (Result: STOP → Phase 2 authorized per checkpoint Next Step)
**Roadmap alignment**: N/A (Phased feature)
**Build target**: `cmake --build build --target engine_tests`
**Groomer Verdict**: PASS

## Expected Outcome

A self-contained convex hull math module exists in the engine, unit-tested and correct: `compute_convex_hull()` builds a ≤26-vertex hull from raw mesh positions; `hull_vs_aabb_sat()` detects and measures hull-vs-box overlap via SAT; `ray_vs_hull()` finds ray-hull intersection via the half-plane slab method. All three are exercised by `test_convex_hull.cpp`, which passes with no regressions in other engine tests. No game or integration code is touched in this phase.

---

## Out-of-Scope Guardrails

- `convex_hull.h` must not include any engine header (`collider.h`, `components.h`, `raycast.h`, `engine.h`). It includes only GLM and stdlib. This constraint is load-bearing: Phase 3 will include `convex_hull.h` from `components.h`; adding an engine-header dependency here would create a circular include chain.
- No changes to `src/game/`, `src/engine/collider.cpp`, `src/engine/raycast.cpp` (other than the `ray_vs_hull` declaration in `raycast.h` and implementation in `raycast.cpp` — see T2), `src/engine/scene_api.cpp`, or `src/engine/components.h`. Those belong to Phase 3.
- No frozen interface changes.

---

## Header Dependency Design

`convex_hull.h` sits at the base of the include graph and must have no upstream engine dependencies:

```
convex_hull.h  →  <glm/glm.hpp>, <vector>, <array>, <string>  (no engine headers)
components.h   →  convex_hull.h, renderer.h, glm              (Phase 3 change)
collider.h     →  components.h                                 (unchanged)
raycast.h      →  collider.h, convex_hull.h                    (adds convex_hull.h in T2)
```

`ray_vs_hull()` is declared in `raycast.h` (not `convex_hull.h`) so it can return `RayHit` without `convex_hull.h` needing to know about that type. `hull_vs_aabb_sat()` is declared in `convex_hull.h` and takes only GLM types.

---

## Tasks

| ID | Status | Description | Files | Schedule | Acceptance |
|----|--------|-------------|-------|----------|------------|
| 1 | [ ] | **ConvexHull data structure, extremal-vertex hull computation, and hull cache.** Add `src/engine/convex_hull.h` and `src/engine/convex_hull.cpp`. **`convex_hull.h`** exports (no engine header includes — GLM and stdlib only): `struct ConvexHull { std::vector<glm::vec3> vertices; std::vector<std::array<int,3>> faces; std::vector<glm::vec3> face_normals; glm::vec3 half_extents; };` — `faces` are CCW-wound index triples into `vertices`; `face_normals` are precomputed outward unit normals (one per face entry); `half_extents` is the local-space AABB half-size of the hull vertex set. Hull cache: `void hull_cache_insert(const std::string& path, ConvexHull hull)` and `const ConvexHull* hull_cache_get(const std::string& path)`. **`convex_hull.cpp`** implements: (a) `compute_convex_hull(const float* positions, size_t vertex_count)`: sample 26 fixed unit directions — 6 axis-aligned (`±X`, `±Y`, `±Z`), 8 body-diagonal (`(±1,±1,±1)/√3`), 12 edge-midpoint (`(±1,±1,0)/√2` and its axis permutations) — find the furthest vertex (max dot product) in each direction, collect unique result indices using a small index set. The candidate set is **at most 26 unique vertices by construction**, which satisfies the ≤30 constraint without any thinning step. Build the triangulated convex hull from these candidates using 3D gift-wrapping: seed with the lowest-Y vertex and one edge from it, then for each open boundary edge pick the vertex that maximizes the interior dihedral angle (equivalently, is furthest "around" the edge), add the new triangle, and continue until all boundary edges are closed. If the candidate set is degenerate (all coplanar — volume of initial tetrahedron < 1e-9), log `[ENGINE] convex_hull: degenerate mesh %s, hull disabled` and return an empty `ConvexHull` (empty `vertices` vector); callers detect this via `hull.vertices.empty()`. Recompute `face_normals` and `half_extents` from the final vertex set after gift-wrapping completes. (b) Hull cache: backed by `static std::unordered_map<std::string, std::unique_ptr<ConvexHull>>`. The `unique_ptr` guarantees the managed object never moves on map rehash, so raw `const ConvexHull*` pointers stored in components remain valid for the process lifetime. These new files are picked up automatically by the existing `vibeaton_collect_sources` glob — no CMakeLists change required. | `src/engine/convex_hull.h`, `src/engine/convex_hull.cpp` | GATE | `cmake --build build --target engine_tests` exits 0. No new test file yet — this gate verifies the module compiles cleanly without warnings. `hull_cache_insert` / `hull_cache_get` round-trip verified by a smoke-test call site in `convex_hull.cpp`'s own file or via a preliminary test if desired; full correctness is T2's gate. |
| 2 | [ ] | **Intersection functions and unit tests.** Extend the module with the two query functions and exercise them in a new test file. (a) `hull_vs_aabb_sat()` in `convex_hull.h` / `convex_hull.cpp`: `struct HullContact { bool hit; glm::vec3 normal; float depth; glm::vec3 point; }; HullContact hull_vs_aabb_sat(const ConvexHull& hull, const glm::vec3& hull_pos, const glm::vec3& box_center, const glm::vec3& box_half)`. Full SAT with three axis sets: AABB face normals (`+X`, `+Y`, `+Z`); hull `face_normals`; edge-edge cross products (each unique hull edge × each of `{+X, +Y, +Z}`, skipping results with `glm::length() < 1e-6`). For each axis: project all hull vertices (offset by `hull_pos`) and all 8 box corners (from `box_center ± box_half`) onto the axis. If any axis shows a gap between the two projection intervals, return `{hit=false}`. Track the minimum-overlap axis: its overlap magnitude → `depth`, its direction pointing from hull toward box → `normal`, midpoint of overlap interval projected back to world → `point`. (b) `ray_vs_hull()` in `raycast.h` (declaration added alongside `ray_vs_aabb`) / `raycast.cpp` (implementation): `RayHit ray_vs_hull(const glm::vec3& origin, const glm::vec3& dir, float max_dist, const ConvexHull& hull, const glm::vec3& hull_pos)`. Half-plane slab method over hull faces: for each face compute the signed distance of `origin` from the face plane (using `hull_pos + hull.vertices[faces[i][0]]` as a plane point and `face_normals[i]` as normal) and `dot(dir, face_normal)`. Accumulate `tmin`/`tmax` with the same slab logic as `ray_vs_aabb` (back-facing faces update `tmin`, front-facing update `tmax`). Return `{hit=true, distance=tmin, point=origin+dir*tmin, normal=face_normal_that_set_tmin}` if `0 <= tmin <= tmax && tmin <= max_dist`. Add `#include "convex_hull.h"` to `raycast.h`. Add new test file `src/engine/tests/test_convex_hull.cpp` (picked up by glob, no CMakeLists change). Required TEST_CASEs: `compute_convex_hull` on 8 unit-cube vertices → 8 hull vertices, 12 triangular faces, `half_extents == (1,1,1)`; on 4 non-coplanar vertices forming a known tetrahedron → 4 vertices, 4 faces; on 1000 uniformly-random unit-sphere samples → `vertices.size() <= 26` and each returned vertex has distance ≈ 1.0 from origin (within 0.01). `hull_vs_aabb_sat` on non-overlapping hull and box → `hit=false`; on a box centered at hull origin with large extents → `hit=true, depth > 0, glm::length(normal) ≈ 1`. `ray_vs_hull` on a ray aimed directly at a convex hull → `hit=true` at the geometrically correct distance; on a ray that clearly misses → `hit=false`; on a ray starting inside the hull → behavior must not crash (result may be hit=false or hit=true with tmin=0; document which in a comment). | `src/engine/convex_hull.h`, `src/engine/convex_hull.cpp`, `src/engine/raycast.h`, `src/engine/raycast.cpp`, `src/engine/tests/test_convex_hull.cpp` | → | `cmake --build build --target engine_tests` exits 0; all `test_convex_hull.cpp` TEST_CASEs pass; no regression in existing `test_collision.cpp` tests. |

**Schedule codes**:
- `GATE` — runs alone first; T2 depends on it
- `→` — runs after T1 completes

---

## Fallbacks

- **T1 (degenerate mesh)**: The explicit degenerate-mesh check (tetrahedron volume < 1e-9) handles pathological cases. None of the four asteroid models are expected to be degenerate — they are closed, manifold, near-spherical meshes — so this path should never fire in practice.
- **T2 (ray starting inside hull)**: Decide at implementation time whether `tmin < 0` is clamped to 0 (reports a hit at the exit face) or left negative (reports no hit). Either is acceptable for the game use cases (laser and plasma fire from outside asteroids). Document the chosen behavior with a one-line comment in `ray_vs_hull`.
- **T2 (SAT edge-edge numerical stability)**: The `glm::length() < 1e-6` skip on near-zero cross products prevents degenerate-axis projections. If correctness issues appear only when edge-edge terms are included, the face-normal-only SAT (axes from sets 1 and 2 only) is an acceptable fallback — it may false-positive on pure edge-edge contacts, which are rare in asteroid-vs-player scenarios.

---

## Human Checkpoint

**Run**: `cmake --build build --target engine_tests`

**Look for**: Build output: 0 errors, 0 warnings on the new files. Test output: all `test_convex_hull.cpp` TEST_CASEs report `PASSED`; the existing test counts for `test_collision.cpp`, `test_physics.cpp`, `test_ecs.cpp`, and `test_math.cpp` are unchanged.

**Pass**: `cmake --build build --target engine_tests` exits 0 and the test runner reports all new and existing tests passing.

**Stop**: Any test in `test_convex_hull.cpp` fails, or any previously-passing test regresses, or the build exits non-zero. Do not proceed to Phase 3 planning until the math module is correct.
