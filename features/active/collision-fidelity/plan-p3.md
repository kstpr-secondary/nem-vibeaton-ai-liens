# Phase Plan: Collision Fidelity — Phase 3: Game Integration

**Gate**: `checkpoint-p2.md` present (Result: PASS)
**Roadmap alignment**: N/A (Phased feature)
**Build target**: `cmake --build build --target game engine_tests`
**Groomer Verdict**: PASS

## Expected Outcome

The Phase 2 math module is wired into all live game systems. `engine_load_gltf()` precomputes and caches a convex hull for every glTF model it loads. `ConvexCollider` components are attached to all 600 asteroid entities at spawn with the correct per-tier scale. `engine_raycast()` (laser) dispatches to `ray_vs_hull()` for entities carrying `ConvexCollider`, AABB fallback for everything else. `detect_collisions()` uses `hull_vs_aabb_sat()` narrowphase for all mixed pairs (player/enemy/projectile ↔ asteroid) while keeping asteroid-to-asteroid on AABB. `damage_resolve()` guards both the kinetic damage path and the plasma projectile path with the same hull narrowphase. Players can fly through visually clear gaps and fire weapons along clear lines without being blocked by AABB corners.

---

## Out-of-Scope Guardrails

- No changes to `physics.cpp`, `collision_response.cpp`, or the Euler integrator.
- No hull-wireframe or hull debug-draw. The corrected `col.half_extents = hull->half_extents * scale` set in T2 makes the existing F3 AABB debug-boxes wrap tightly — that satisfies the brief's debug-draw success criterion without new rendering work.
- No new unit tests. Tests were completed in Phase 2; T4 must not regress them.
- No changes to frozen interfaces.

---

## Include Chain (verified)

After T1 adds `#include "convex_hull.h"` to `components.h`, the chain is:

```
engine.h          →  components.h  →  convex_hull.h   (ConvexCollider, hull_vs_aabb_sat visible)
collider.h        →  components.h  →  convex_hull.h
raycast.h         →  convex_hull.h                     (already added in Phase 2)
```

No TU (collider.cpp, raycast.cpp, scene_api.cpp, damage.cpp, spawn.cpp) needs a new explicit `#include "convex_hull.h"` — each already includes `engine.h` or a header in the chain above.

---

## Scale transform convention (applies to T3, T4, T5)

All hull intersection functions operate in local hull space (hull vertices at mesh scale,
`hull_pos = {0,0,0}`). The caller transforms inputs as follows:

```
// Given: world_pos (entity position), cc.scale (uniform scale), cc.hull
float inv_s = 1.0f / cc.scale;

// For ray_vs_hull (T3):
glm::vec3 lo = (ray_origin - world_pos) * inv_s;   // local ray origin
glm::vec3 ld = ray_dir * inv_s;                     // local direction — t-parameterization preserved
// t_local == t_world; h.distance is correct world parametric distance after the call.
// Recalculate h.point = ray_origin + ray_dir * h.distance in world space.

// For hull_vs_aabb_sat (T4, T5):
glm::vec3 lbc = (box_center - world_pos) * inv_s;  // box center in local hull space
glm::vec3 lbh = box_half * inv_s;                  // box half-extents in local hull space
// After the call: depth *= cc.scale; point = world_pos + hc.point * cc.scale; normal unchanged.
```

Degenerate guard (all callers): if `cc.scale <= 0` or `cc.hull == nullptr` or
`cc.hull->vertices.empty()`, fall through to the AABB path.

---

## SAT normal sign convention (T4)

`hull_vs_aabb_sat` returns `HullContact.normal` pointing **from hull toward box**.
`CollisionPair.normal` is **from b toward a** (per `collider.cpp` convention: `a = entries[i].e`, `b = entries[j].e`).

| Hull entity | Box entity | SAT normal direction | CollisionPair adjustment |
|-------------|------------|----------------------|--------------------------|
| entries[i]  | entries[j] | a → b (wrong)        | negate: `pair.normal = -hc.normal` |
| entries[j]  | entries[i] | b → a (correct)      | use as-is: `pair.normal = hc.normal` |

---

## Tasks

| ID | Status | Description | Files | Schedule | Acceptance |
|----|--------|-------------|-------|----------|------------|
| 1 | [x] | **ConvexCollider component; hull precomputation at load time.** (a) `components.h`: add `struct ConvexCollider { const ConvexHull* hull = nullptr; float scale = 1.0f; };` after the existing tag-component block. Add `#include "convex_hull.h"` before the struct definition. The `hull` pointer is into the hull cache (`unique_ptr`-backed, stable address); `scale` is the entity's uniform render scale. This establishes the include chain described above — no downstream TU needs further includes. (b) `scene_api.cpp` / `engine_load_gltf()`: immediately after `asset_import_gltf()` returns a non-empty `ImportedMesh` and before the `s_gltf_cache` insert, add: if `hull_cache_get(relative_path) == nullptr` → `ConvexHull h = compute_convex_hull(mesh.positions.data(), mesh.positions.size() / 3)`; if `h.vertices.empty()` log `[ENGINE] convex_hull: degenerate mesh %s, hull disabled`; else call `hull_cache_insert(relative_path, std::move(h))` and log `[ENGINE] convex_hull: %zu vertices for %s`. If `mesh.positions.empty()` skip hull computation entirely (size/3 == 0 is a valid no-op for `compute_convex_hull`). Second call for the same path hits the `s_gltf_cache` early return before hull code is reached — no double computation. | `src/engine/components.h`, `src/engine/scene_api.cpp` | GATE | `cmake --build build --target engine_tests` exits 0; no regressions. |
| 2 | [x] | **spawn_asteroid: attach ConvexCollider; calibrate AABB broadphase half-extents.** In `spawn_asteroid()` in `src/game/spawn.cpp`: after `engine_add_component<Collider>(e)` sets `col.half_extents = glm::vec3(radius)`, add: `const ConvexHull* hull = hull_cache_get(model);` (the `model` variable holding the random model path is already in scope at line 326). If `hull != nullptr && !hull->vertices.empty()`: (a) `engine_add_component<ConvexCollider>(e) = {hull, scale}` — `hull` is the cache pointer, `scale` is the tier-specific float from the switch block. (b) Update `col.half_extents = hull->half_extents * scale` — replaces the oversized `glm::vec3(radius)` with mesh-accurate extents, fixing the F3 debug-box size. Else: log `[GAME] spawn_asteroid: no hull for %s, using radius AABB` and leave `col.half_extents` unchanged. The null path cannot fire in normal operation (hull is precomputed inside `engine_load_gltf`, which `spawn_from_model` calls before returning); the guard is defensive. | `src/game/spawn.cpp` | → after T1 | `cmake --build build --target game` exits 0. |
| 3 | [ ] | **engine_raycast hull narrowphase.** In `engine_raycast()` in `src/engine/raycast.cpp`: inside the existing `view<Transform, Collider, Interactable>().each(...)` lambda, before the AABB path, add: `const auto* cc = reg.try_get<ConvexCollider>(e);`. Hull path guard: `cc && cc->hull && !cc->hull->vertices.empty() && cc->scale > 0.f`. If guard passes: apply the scale transform (see Scale Transform Convention): `lo = (origin - t.position) / cc->scale`, `ld = direction / cc->scale`. Call `ray_vs_hull(lo, ld, max_distance, *cc->hull, {0,0,0})`. If `h.hit`: recalculate `h.point = origin + direction * h.distance` (the `h.point` from `ray_vs_hull` is in local space; `h.distance` is already the correct world parametric t because the t-parameterization is preserved). Use `h` for the existing `best` comparison. If guard fails: existing `ray_vs_aabb` path, unchanged. Ray-inside-hull: `ray_vs_hull` returns `hit=true` at `distance=0` in this case (documented in Phase 2). This is handled correctly — the laser fires from outside asteroids in all gameplay scenarios. | `src/engine/raycast.cpp` | ‖ with T2, T4, T5 | `cmake --build build --target game` exits 0. |
| 4 | [ ] | **detect_collisions hull narrowphase for mixed ConvexCollider pairs.** In `src/engine/collider.cpp` / `detect_collisions()`: extend the internal `Entry` struct with `const ConvexCollider* cc = nullptr`. In the entity-collection lambda: `cc = reg.try_get<ConvexCollider>(e)`. In the pair loop, after the existing `aabb_overlap` broadphase gate, compute `bool a_hull = entries[i].cc && entries[i].cc->hull && !entries[i].cc->hull->vertices.empty() && entries[i].cc->scale > 0.f;` (same for `b_hull`). Dispatch: (a) `!a_hull && !b_hull` — existing `compute_contact()` path. (b) `a_hull && b_hull` (asteroid↔asteroid) — existing `compute_contact()` path. Asteroid-to-asteroid physics stays AABB. (c) `a_hull ^ b_hull` — hull narrowphase for all mixed pairs (player/enemy/plasma ↔ asteroid): Identify `hull_entry` (whichever of i/j has true hull) and `box_entry`. Apply scale transform: `hull_pos = (hull_entry.aabb.min + hull_entry.aabb.max) * 0.5f` (equals entity position since AABB is axis-aligned around position), `box_center = (box_entry.aabb.min + box_entry.aabb.max) * 0.5f`, `box_half = (box_entry.aabb.max - box_entry.aabb.min) * 0.5f`, `inv_s = 1.f / hull_entry.cc->scale`, `lbc = (box_center - hull_pos) * inv_s`, `lbh = box_half * inv_s`. Call `hull_vs_aabb_sat(*hull_entry.cc->hull, {0,0,0}, lbc, lbh)`. If `!hc.hit`: skip. If `hc.hit`: build `CollisionPair p{entries[i].e, entries[j].e, normal, depth, point}` where `depth = hc.depth * hull_entry.cc->scale`, `point = hull_pos + hc.point * hull_entry.cc->scale`, and normal as per the SAT sign table above. Intentional behavior change: plasma projectile ↔ asteroid pairs now use hull narrowphase in physics; asteroid impulses from plasma fire only on actual hull intersection. | `src/engine/collider.cpp` | ‖ with T2, T3, T5 | `cmake --build build --target game engine_tests` exits 0; no regression in `test_collision.cpp`. |
| 5 | [ ] | **damage_resolve hull narrowphase: kinetic damage and plasma projectile.** In `src/game/damage.cpp`: `ConvexCollider` and `hull_vs_aabb_sat` are visible via `engine.h → components.h → convex_hull.h` (T1). (a) **Kinetic damage path (player ↔ asteroid)**: After the existing `aabb_overlap(pt.position, pc.half_extents, at.position, ac.half_extents)` guard passes: `const auto* acc = engine_try_get_component<ConvexCollider>(ae);` — if `acc && acc->hull && !acc->hull->vertices.empty() && acc->scale > 0.f`: apply scale transform with `at.position` as hull origin → if `!hc.hit`: `continue`. This prevents kinetic damage from firing when the player is in the AABB corner but outside the hull. (b) **Projectile ↔ target path**: After the existing `aabb_overlap(pt.position, pc.half_extents, tt.position, tc.half_extents)` broadphase check passes (AABB remains the broadphase): `const auto* tcc = engine_try_get_component<ConvexCollider>(te);` — if `tcc && tcc->hull && !tcc->hull->vertices.empty() && tcc->scale > 0.f`: apply scale transform with `tt.position` as hull origin, projectile AABB (`pt.position`, `pc.half_extents`) as the box → if `!hc.hit`: `continue`. The shield check runs before the AABB broadphase (unchanged) and player/enemy entities have no `ConvexCollider` → `tcc` is null for them → hull code is skipped, AABB path unchanged for those targets. | `src/game/damage.cpp` | ‖ with T2, T3, T4 | `cmake --build build --target game` exits 0. |

**Schedule codes**:
- `GATE` — must complete and build cleanly before T2–T5 start
- `→ after T1` — sequential after T1
- `‖` — can run in parallel with other ‖ tasks (all touch disjoint files)

---

## Fallbacks

- **T1 (degenerate hull)**: The `compute_convex_hull` degenerate check (already in Phase 2) handles pathological input. None of the four asteroid models are degenerate — this path will not fire in practice. Entities whose model produced no hull get no `ConvexCollider` and fall through to AABB everywhere.
- **T3/T4/T5 (null hull guard)**: `cc->hull == nullptr` or `vertices.empty()` falls through to AABB at every call site — the system degrades gracefully to Phase 1 behavior for any entity missing a hull.
- **T4 (SAT face-normal-only fallback)**: If edge-edge SAT terms produce numerical issues in practice, the face-normal-only SAT (axes from face normals and AABB normals only) is an acceptable fallback — it may false-positive on pure edge-edge contacts but those are rare in asteroid-vs-ship scenarios.

---

## Human Checkpoint

**Run**: `cmake --build build --target game engine_tests`

**Look for**:
1. Build exits 0 with no new errors or warnings.
2. All existing tests (`test_convex_hull.cpp`, `test_collision.cpp`, `test_physics.cpp`, `test_ecs.cpp`, `test_math.cpp`) pass with unchanged counts.
3. Launch the game and check:
   - `[ENGINE] convex_hull: X vertices for <model>` log lines appear for all four asteroid models at startup.
   - **Visual**: Fly through a visually clear gap between two asteroids — no collision triggers.
   - **Visual**: Fire the laser along a visually clear line toward the enemy near asteroids — beam does not terminate at empty space beside an asteroid.
   - **Visual**: Fire plasma through a visually clear gap — shot passes through without detonating on empty AABB corners.
   - **Visual (F3 debug)**: Semi-transparent AABB debug boxes now wrap tightly around asteroid meshes rather than over-extending at corners.

**Pass**: `cmake --build build --target game engine_tests` exits 0; all tests pass; all three visual weapon checks confirm no invisible-wall or invisible-wall-blocking behavior.

**Stop**: Any build error, test regression, persistent invisible-wall collision in a visually clear gap, or laser/plasma intercepted by empty AABB space beside an asteroid. Do not proceed to doc update/archive until all visual checks pass.
