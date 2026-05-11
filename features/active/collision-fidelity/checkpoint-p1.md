 # Checkpoint: Collision Fidelity — Phase 1

**Date**: 2026-05-11
**Verified by**: kstpr

## Result: STOP

## What was tested

The previous implementing agent partially executed Phase 1: T3 (debug visualization) was written while
T1 (mesh diagnostic) and T2 (half-extents calibration fix) were skipped, violating the GATE ordering
in the plan. The agent also made three unapproved changes outside any plan task: added rotation to
`compute_world_aabb` in `collider.cpp` and `raycast.cpp`, and had the debug boxes copy the asteroid's
rotation. These changes caused asteroid collision volumes to pulse with rotation (up to √3× the nominal
half-extents each substep), making the asteroid field impassable. The game was launched and the F3 debug
overlay was observed: large semi-transparent cubes, clearly oversized relative to the visible mesh and
jerking constantly as asteroids spun.

T1 was implemented and executed correctly in a follow-up session. The unapproved changes were then
reverted to restore the pre-branch collision behavior.

## Observations

**T1 diagnostic results** (local half-extents from mesh vertex data at load time):

| Model | verts | local half-extent |
|-------|-------|-------------------|
| Asteroid_1e.glb | 15 208 | **2.330** |
| Asteroid_1a.glb | 12 869 | 1.853 |
| Asteroid_2a.glb | 11 889 | 1.487 |
| Asteroid_2b.glb | 16 022 | 1.170 |

Models vary 2:1 in extent (>10% threshold). Per the plan's max policy, the conservative world-space
half-extents would be `render_scale × 2.33` — which for large asteroids is 41.9 vs the current 36.
The corrected values would be *larger* than current for most models, not smaller, because the current
implicit assumption (mesh reaches ±2 local units) overestimates three of four models while
underestimating Asteroid_1e.

**Shape mismatch is the dominant problem, not calibration.** Even with perfectly calibrated half-extents,
cube AABBs leave invisible corner volumes far outside the visible mesh surface and fail to occlude
raycasts along visually clear paths. The player was experiencing invisible-wall collisions and shots
deflecting off empty space before this feature branch began; this root cause is the cube shape, not
the size.

**Asteroid-to-asteroid physics is working acceptably.** The uniform half-extents act as a sphere
approximation; the impulse-based 6-DOF response produces plausible chaotic field behavior. This path
should not be disturbed.

**AABB (in any orientation) is not adequate for player ↔ asteroid collision or weapon hit detection.**
This was the Phase 1 gate question, and the answer is unambiguous: proceed to convex hull.

## Next step

Generate `plan-p2.md` with the following scope:

- Precompute a simplified convex hull (≤30 vertices) per asteroid model at asset-load time; cache 4
  hulls by model path, shared across all entity instances.
- Add a `ConvexCollider` component referencing the shared hull; attach to asteroid entities at spawn.
- Add a separate hull-based collision detection pass for player/enemy ↔ asteroid pairs (proximity
  filter + GJK or SAT narrowphase). Tag-filter those pairs out of the existing `detect_collisions()`
  AABB pass so asteroid-to-asteroid physics is unchanged.
- Update `engine_raycast()` to use ray-vs-hull for entities carrying `ConvexCollider`, AABB fallback
  for everything else.
- Unit-test the ray-vs-hull and hull-vs-AABB intersection paths.
- No changes to `physics.cpp`, `collision_response.cpp`, or the Euler integration; physics response
  consumes a `CollisionPair` and is agnostic to how the contact was detected.
