# Feature Brief: Collision Fidelity

**Type**: Phased  
**Branch**: feature/better-player-collision  
**Workstreams**: engine, game  
**Frozen interfaces affected**: none (additive component additions only)  

## Goal

Replace the oversized AABB colliders on asteroids — which cause the player to bump invisible walls in the dense field and shoot through clear lines of sight — with accurate convex hull collision for player/enemy-vs-asteroid interactions and for weapon hit testing. Asteroid-to-asteroid physics keeps AABB.

## Success Criteria

- [ ] Visual: Player can fly through a visually clear gap between two asteroids without collision triggering.
- [ ] Visual: Plasma shots and laser follow a visually clear line to the enemy without being blocked by asteroid AABBs the ray does not visually cross.
- [ ] Visual (debug mode): A debug AABB or hull draw shows the collision volume tightly wrapping the asteroid mesh, not a much-larger box around it.
- [ ] Gameplay: Player can navigate a dense asteroid cluster at normal flight speed without constant invisible-wall bounces.
- [ ] Build: `cmake --build build --target game engine_tests` exits 0.

## Scope

**In**:
- Diagnose and fix current oversized asteroid AABB — determine whether `col.half_extents = glm::vec3(radius)` is calibrated correctly against the rendered scale (`k_asteroid_scale_* = asteroid_*_scale / 2.0f`) and against actual mesh vertex extents.
- Per-asteroid convex hull: store a low-poly hull (precomputed from the glTF mesh at spawn) for the four asteroid models. Used only for player/enemy ↔ asteroid and weapon ↔ asteroid tests.
- Convex hull hit detection for `engine_raycast()` (laser), replacing the AABB slab test for entities that carry a hull.
- Convex hull collision detection for plasma projectiles: two-tier test (AABB broadphase → hull narrowphase).
- Proximity filter for player/enemy-asteroid collision pairs: only test convex hulls against asteroids within a radius (avoid testing all 600 asteroid hulls every substep).
- Unit tests for the convex hull intersection and ray-vs-hull path.

**Out (explicit)**:
- Asteroid-to-asteroid collision — AABB stays (players do not experience those interactions up close).
- OBB collision (skipped in favour of convex hull, which is a strict superset).
- Swept AABB / continuous collision detection / tunneling prevention (separate future concern, different problem).
- Convex hull for the player ship itself (box AABB is close enough for the ship shape; focus is asteroids).
- GJK-based TOI or swept convex hull (not needed for this fix).
- Spatial partitioning / BVH (the proximity filter is the pragmatic replacement for Phase 1).

## Key Unknowns

| Unknown | Risk if wrong | How to verify | Fallback |
|---------|--------------|---------------|---------|
| Are `col.half_extents = glm::vec3(radius)` values calibrated to the actual mesh vertex extents at the applied render scale, or are they wrong? The render scale is `asteroid_*_scale / 2.0f` but half_extents use `asteroid_*_scale` directly, implying the mesh must reach ±2 units in local space for the math to work. | AABB fix alone may solve most of the issue; convex hull work may be less urgent than expected. | Inspect model vertex bounds in `asset_import.cpp` (or add a one-off print) and compare to `half_extents / scale`. | If AABB calibration was correct and the hulls are needed, proceed to Phase 2 without the calibration fix. |
| What are the actual glTF asteroid mesh geometries — are they convex, or do they have concavities that make a convex hull meaningfully different from the mesh? (Visual inspection of the game confirms irregular, lumpy, mostly-convex shapes — not spheres.) | Even near-spherical asteroids are tested as AABB cubes at present; the cube's corners extend far beyond the sphere surface and fire false positive collisions. A sphere collider is not a valid fallback. Convex hull is needed regardless of roundness. | Inspect vertex positions of the four models in the importer to confirm convexity and overall shape. | None — convex hull is the appropriate solution for all asteroid shapes observed. |
| How many vertices do the four asteroid models have? The hull construction cost and runtime support() cost depend on vertex count. | High vertex count means per-frame GJK support() is expensive at 600 asteroids. | Check vertex count during model inspection. | Pre-simplify hulls to ≤30 vertices at import time. |

## Known Risks

- The four asteroid models (`Asteroid_1a.glb`, `Asteroid_1e.glb`, `Asteroid_2a.glb`, `Asteroid_2b.glb`) are reused across all 600 asteroid entities. The convex hull must be stored per-model (4 hulls total, looked up at collision time) — a per-entity copy would multiply memory usage ~150×.
- `compute_world_aabb` ignores `Transform.scale` and `Transform.rotation`. Fixing it to use scale is trivial but may break assumptions in the existing AABB tests if the existing half_extents were already scaled by hand. Any fix to calibration must be verified against `engine_tests` after the change.
- The laser uses `engine_raycast()` which queries all `Interactable` entities including the player shield sphere, the enemy, and projectiles. Adding hull tests only for entities that carry a `ConvexCollider` component keeps backward compatibility; entities without it continue to use AABB.
- Proximity filter radius must be tuned: too small and the player can clip asteroids that just entered the radius; too large and it defeats the purpose. A radius of `2 × largest_asteroid_half_extent + player_half_extent` is a safe starting point.

## Code Locations (for implementors)

- `src/engine/collider.h` / `collider.cpp` — AABB, WorldAABB, detect_collisions()
- `src/engine/raycast.cpp:74` — `engine_raycast` uses `compute_world_aabb(t.position, c.half_extents)` with no hull fallback
- `src/engine/components.h:35-38` — `Collider { half_extents }` — the component to extend or supplement
- `src/game/spawn.cpp:307-349` — asteroid spawn sets `col.half_extents = glm::vec3(radius)` where `radius = constants::asteroid_*_scale` and render `scale = constants::asteroid_*_scale / 2.0f`
- `src/game/constants.h:84-108` — tier scale constants (`asteroid_large_scale = 36.0f`, etc.)
- `src/engine/asset_import.cpp` — mesh vertex extraction; hull precomputation belongs here or in a new `convex_hull.cpp`

## Planned Phases

### Phase 1 — Diagnose & Fix AABB Calibration

Inspect actual mesh vertex extents for all four asteroid models. Compare to `col.half_extents` at the applied render scale. Fix the half_extents values in `spawn_asteroid()` if they are wrong. Add a toggleable debug-draw mode that spawns a semi-transparent AABB box mesh (cube entity with an alpha-blended Blinn-Phong material, depth-write off) co-located with each asteroid, so the AABB volume is visible over the lit mesh. This is the only practical debug approach since the renderer has no edge/wireframe drawing capability.

**Gate for Phase 2**: human checkpoint confirms whether AABB fix alone makes navigation tolerable, or whether convex hull is still needed.

### Phase 2 — Convex Hull Collision

Add `ConvexCollider` component carrying a precomputed low-poly vertex hull (per model, shared across entity instances). Implement ray-vs-convex and AABB-broadphase + point-in-convex for plasma projectiles. Add proximity-filtered player/enemy-vs-asteroid convex hull dispatch in `detect_collisions()`. Unit-test the new intersection paths.
