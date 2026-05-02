# Swept AABB + Convex Hull Collision — Research & Implementation Plan

> **Created:** 2026-05-01
> **Status:** Research / Backlog
> **Priority:** High (tunneling through walls and thin objects during physics)
> **Depends on:** Engine E-M4 (current Euler physics + AABB collision response), Game G-M2 (player ship collisions, containment reflection)

---

## 1. Problem Statement

### 1.1 Current Collision Detection

The engine uses **discrete-time AABB overlap detection**:

- `collider.cpp:59` iterates all Transform+Collider entities, computes world AABB from position + half_extents (ignoring rotation/scale).
- `detect_collisions()` does O(N²) brute-force pairwise overlap.
- `compute_contact()` finds minimum-penetration axis and returns a contact normal, depth, and midpoint.

**This only detects objects that are already overlapping.** It cannot prevent an object from passing through another between frames if the object's velocity is large enough to skip over the other entirely during Euler integration.

### 1.2 Observed Symptoms

- Entities drift outside arena bounds despite wall collisions being detected — positional correction (Baumgarte) at `K_BAUMGARTE = 0.3f` pushes entities back by only 30% of penetration per substep.
- Fast-moving entities (tier-2/3 spawner objects with velocities up to ±12 units/s) can tunnel through walls during substeps where the wall contact is missed entirely (no overlap at either start or end of substep).
- Player-ship vs asteroid collisions produce false positives when the player's box collider intersects a rotated/irregular asteroid mesh that is visually non-overlapping.

### 1.3 Why Baumgarte Alone Isn't Enough

Baumgarte stabilization (`positional_correction.cpp`) is a *post-detection* fix. It corrects penetration after it has already occurred. If detection never fires (tunneling), Baumgarte has nothing to correct. Tightening `K_BAUMGARTE` from 0.3 to 0.8 helps contact resolution quality but does not solve tunneling through thin/fast collisions.

---

## 2. Solution: Swept AABB

### 2.1 What It Is

**Swept AABB** (also called "continuous collision detection" or CCD for AABBs) tests the *trajectory volume* swept by an AABB as it moves over a time step, rather than just its final position.

For an AABB with half-extents `h` moving from `p_start` to `p_end = p_start + v * dt`:
- The swept volume is an AABB from `min(p_start-h, p_end-h)` to `max(p_start+h, p_end+h)`.
- We test this swept AABB against the static (or also-swept) AABB of every other collider.
- If overlap exists, we compute the **time of impact (TOI)** — the fraction of the timestep when contact first occurs.

### 2.2 Sweep Test Algorithm (2D/3D Slab Method)

The standard approach for swept AABB overlap with TOI:

```cpp
struct SweepHit {
    float toi;        // time of impact in [0, dt] — 0 = already overlapping
    glm::vec3 normal; // face normal at impact (from other toward this)
};

SweepHit swept_aabb(const Transform& tr_start, const Transform& tr_end,
                    const Collider& col, const WorldAABB& other) {
    // AABB of the swept volume
    glm::vec3 p_min = glm::min(tr_start.position - col.half_extents,
                               tr_end.position   - col.half_extents);
    glm::vec3 p_max = glm::max(tr_start.position + col.half_extents,
                               tr_end.position   + col.half_extents);

    // Check if swept volume overlaps other AABB at all
    if (!aabb_overlap({p_min, p_max}, other)) {
        return {1e9f, {}}; // no hit
    }

    // Slab method: find earliest entry and latest exit across all axes
    float t_enter = 0.0f;
    float t_exit  = 1.0f;
    glm::vec3 inv_dir = 1.0f / (tr_end.position - tr_start.position);

    for (int axis : {0, 1, 2}) {
        if (inv_dir[axis] == 0.0f) {
            // Stationary along this axis — check if inside slab
            if (p_min[axis] > other.max[axis] || p_max[axis] < other.min[axis])
                return {1e9f, {}}; // outside this axis slab entirely
        } else {
            float t0 = (other.min[axis] - p_max[axis]) * inv_dir[axis];
            float t1 = (other.max[axis] - p_min[axis]) * inv_dir[axis];
            if (t0 > t1) std::swap(t0, t1);
            t_enter = std::max(t_enter, t0);
            t_exit  = std::min(t_exit,  t1);
            if (t_enter > t_exit) return {1e9f, {}}; // no overlap
        }
    }

    // t_enter is the TOI if positive (already overlapping if t_enter <= 0)
    glm::vec3 contact_pos = tr_start.position + (tr_end.position - tr_start.position) * t_enter;
    glm::vec3 center_other = (other.min + other.max) * 0.5f;
    glm::vec3 normal = normalize(contact_pos - center_other);

    return {t_enter, normal};
}
```

### 2.3 Sweep vs Static Detection Comparison

| Aspect | Discrete AABB (current) | Swept AABB |
|--------|------------------------|------------|
| Detects tunneling through thin walls? | No | Yes |
| TOI computation | N/A | Yes — enables substep repositioning at exact impact time |
| Performance | O(N²) simple overlap checks | O(N²) with more per-pair work (slab method + inv_dir) |
| Rotation handling | Ignored (AABB axis-aligned) | Ignored (still AABB-based) |
| Implementation complexity | Low | Medium |

### 2.4 Integration Points in Current Engine

Current `physics_substep()` flow (`physics.cpp:66-90`):

```
1. Clear ForceAccum for all Dynamic entities
2. Integrate linear + angular state (Euler)
3. resolve_all_collisions() — discrete AABB detection + impulse response
```

**Swept AABB integration has two viable strategies:**

#### Strategy A: Pre-sweep (between-integration, conservative)
Before Euler integration, sweep each entity's *current* position against all others to find potential future collisions. If a collision is predicted within the substep, reposition to TOI and apply impulse.

**Pros:** Simplest to integrate — replaces step 3 with sweep+collision.
**Cons:** Requires running sweep before integration, meaning velocities used for sweep are from previous substep (one-frame stale).

#### Strategy B: Post-sweep (after integration, what-not-happened)
After Euler integration at step 2, detect *what would have been missed* by sweeping the trajectory from old position to new position. For each hit, back-propagate the entity to TOI, apply collision response, then continue integration for remaining substep time.

**Pros:** Uses current velocities (more accurate).
**Cons:** Requires storing old positions before step 2; more complex control flow.

**Recommendation: Strategy A** for MVP. It's simpler and the velocity staleness is negligible at 120 Hz substeps. At T+0, the worst case is a fast object missing one wall bounce per few frames — acceptable.

### 2.5 Modified Substep Flow

```
physics_substep(reg, h):
  // -- Phase A: Sweep (NEW) --
  For each Dynamic entity e:
    tr_old[e] = tr[e].position          // snapshot
    sweep_result[e] = swept_aabb(tr_old[e], tr[e].position, col[e], all_other_AABBs)

  // -- Phase B: Integrate (existing) --
  integrate_linear(tr, rb, force_accum, h)
  integrate_angular(tr, rb, torque, h)

  // -- Phase C: Resolve (modified) --
  For each sweep_result with TOI < h:
    tr.position = tr_old + velocity * TOI   // back-propagate to impact time
    resolve_collision(tr, rb, ...)          // impulse response at exact contact
    tr.position += velocity * (h - TOI)     // continue for remaining time

  For each overlap detected by regular AABB check:
    resolve_collision(...)
```

---

## 3. Extension: Convex Hull Collision

### 3.1 Why Convex Hulls

The current system uses **AABB colliders** (`Collider { half_extents }`) which are axis-aligned boxes regardless of object orientation or shape. This causes false positives for:

- **Player ship vs irregular asteroids:** The player's box collider overlaps the asteroid's AABB even when the visual meshes don't intersect, because the asteroid's bounding box is much larger than its actual volume (especially for elongated/irregular shapes).
- **Player ship vs thin walls:** A narrow wall or obstacle has a wide AABB if rotated.
- **Asteroid-asteroid collisions:** Elongated asteroids have AABBs that overlap far beyond their visual intersection, producing unrealistic bounce angles.

### 3.2 OBB (Oriented Bounding Box) — Interim Step

Before full convex hulls, **OBB** (oriented bounding box) is the logical next step:
- Each collider stores half-extents + an orientation quaternion.
- Collision detection uses **Separating Axis Theorem (SAT)** for two oriented boxes.
- Test axes: 3 from each box = 9 axes total + 3 cross products = 12 axes (optimized to 9 by omitting redundant crosses).

**OBB vs AABB:**
| Aspect | AABB | OBB |
|--------|------|-----|
| Collision accuracy | Low for rotated objects | High |
| False positives | Common (axis-aligned box covers corners) | Few (box rotates with object) |
| Performance | Very fast (3 comparisons per axis) | Moderate (9 SAT axes) |
| Implementation complexity | Trivial | Medium |
| Swept support | Not applicable | OBB-swept is complex; deferred to convex hulls |

**Recommendation:** Implement OBB as a *separate* collider type (`OBBCollider { half_extents, rotation_matrix }`) alongside the existing `Collider`. This allows gradual migration — asteroids use OBB, simple cubes keep AABB.

### 3.3 Convex Hull Collision (Full)

For truly irregular objects (player ship mesh, detailed asteroid models), convex hull collision is the proper solution:

**GJK Algorithm (Gilbert-Johnson-Keerthi):**
- Determines if two convex shapes intersect without explicit mesh representation.
- Works with support functions: `support(shape, direction)` returns the vertex furthest in a given direction.
- For polyhedra: support = max dot product over vertices.
- For spheres: support = center + radius * direction.
- For Minkowski sum of two shapes: support_A(dir) - support_B(-dir).

** EPA (Expanding Polytope Algorithm):**
- Extension of GJK that computes contact normal and penetration depth when shapes intersect.
- Builds a simplex from the Minkowski difference, expands it toward the origin.
- Output: closest point on simplex boundary to origin = contact normal + depth.

**Implementation complexity:** High. Requires:
1. Extracting vertex lists from glTF assets (or pre-computed hulls).
2. Implementing GJK+EPA from scratch (no library available in current dependency set).
3. Swept convex collision (even harder — requires continuous GJK or conservative advancement).

### 3.4 Convex Hull + Player-Object Collision Impact

The game design document notes: *"player to object collision — there are a lot of false positives there."* This is the primary driver for convex hulls:

- **Player ship** has a custom glTF mesh (not a box). Current AABB collider covers far more volume than the visible ship.
- **Asteroids** have irregular shapes; their AABBs overlap even when visually separated.
- **Weapons:** Laser raycast is already accurate (line test). Plasma projectiles are small spheres — their AABB is close to actual shape, so false positives are minimal.

---

## 4. Implementation Plan

### 4.1 Phase 1: Swept AABB (High Priority)

| Task | Description | Tier | Depends On |
|------|-------------|------|------------|
| SA-01 | Add `SweepHit` struct to `collider.h` with `toi` and `normal` fields | LOW | — |
| SA-02 | Implement `swept_aabb()` function using slab method in `collider.cpp` | MED | SA-01 |
| SA-03 | Add position snapshot array to `physics_substep()` — store old positions before integration | LOW | — |
| SA-04 | Post-integration sweep: for each Dynamic entity, compute trajectory volume and test against all other AABBs | MED | SA-02, SA-03 |
| SA-05 | Back-propagate to TOI on hit, apply collision response, continue integration for remaining time | HIGH | SA-04 |
| SA-06 | Update `resolve_collision()` to accept TOI parameter (optional — may not need if we just reposition) | LOW | SA-05 |
| SA-07 | Unit tests: swept AABB vs discrete overlap edge cases (tunneling scenarios, grazing contacts) | MED | SA-05 |
| SA-08 | Demo scene verification: no more OOB entities when walls are present; wall bounce timing is accurate | SPEC-VALIDATE + REVIEW | SA-07 |

**Estimated effort:** 4–6 hours (1 person)

### 4.2 Phase 2: OBB Collision (Medium Priority)

| Task | Description | Tier | Depends On |
|------|-------------|------|------------|
| OB-01 | Add `OBBCollider` component to `components.h`: `{ half_extents, rotation }` | LOW | — |
| OB-02 | Implement SAT-based OBB-vs-OBB overlap test in `collider.cpp` | HIGH | OB-01 |
| OB-03 | Implement OBB-vs-AABB overlap test (for mixed scenarios) | MED | OB-02 |
| OB-04 | Modify `detect_collisions()` to dispatch to correct test based on collider type | MED | OB-03 |
| OB-05 | Update asteroid spawn to use OBB colliders (half_extents + rotation from mesh orientation) | LOW | OB-04 |
| OB-06 | Update player ship spawn to use OBB collider | LOW | OB-05 |
| OB-07 | Update `compute_world_aabb()` for OBB (rotate half-extents by quaternion) | MED | OB-01 |
| OB-08 | Unit tests: SAT overlap, edge cases (nearly parallel boxes, grazing contacts) | MED | OB-04 |
| OB-09 | Demo scene: verify false positives reduced for player-asteroid collisions | SPEC-VALIDATE + REVIEW | OB-08 |

**Estimated effort:** 6–8 hours (1 person)

### 4.3 Phase 3: Convex Hull Collision (Desirable / Stretch)

| Task | Description | Tier | Depends On |
|------|-------------|------|------------|
| CH-01 | Research GJK/EPA implementations (Open Source references, licensing check) | LOW | — |
| CH-02 | Implement support function for convex polyhedra (vertex list + direction → max dot vertex) | MED | CH-01 |
| CH-03 | Implement GJK algorithm (simplex-based intersection test) | HIGH | CH-02 |
| CH-04 | Implement EPA for contact normal and depth | HIGH | CH-03 |
| CH-05 | Add `ConvexCollider` component `{ vertices[], vertex_count }` to `components.h` | LOW | — |
| CH-06 | glTF mesh → convex hull conversion pipeline (pre-compute hulls at editor/import time) | HIGH | CH-05 |
| CH-07 | GJK-vs-GJK collision test in `detect_collisions()` | HIGH | CH-04, CH-06 |
| CH-08 | Swept convex collision (conservative advancement or substep GJK sweep) | HIGH | CH-07 |
| CH-09 | Player ship uses ConvexCollider from mesh vertices | MED | CH-07 |
| CH-10 | Unit tests: GJK intersection, EPA contact output, edge cases (touching, grazing, nested) | MED | CH-07 |

**Estimated effort:** 12–16 hours (2 people for parallel work on GJK vs EPA)

---

## 5. Performance Considerations

### 5.1 Swept AABB Cost

- Current discrete AABB: 3 comparisons per pair (x, y, z overlap check).
- Swept AABB slab method: ~15–20 floating-point operations per axis × 3 axes = ~45–60 ops per pair.
- With N=250 entities (200 asteroids + player + enemy + projectiles): N²/2 ≈ 31,250 pairs.
- Total: ~1.5M–2M additional FLOPs per substep × 8 substeps/frame ≈ 12–16M FLOPs/frame.
- At 60 FPS: ~720M–960M FLOPs/sec for collision detection alone — well within GPU-bound rendering budget, but measurable on CPU.

**Optimization:** Spatial partitioning (uniform grid or BVH) reduces N² to O(N log N). This is a **Phase 4** concern if swept AABB proves too slow at scale.

### 5.2 OBB SAT Cost

- 9 axes × (dot product + projection) ≈ 30–40 ops per pair.
- Similar cost to swept AABB; acceptable when combined.

### 5.3 GJK/EPA Cost

- GJK: iterative simplex construction, typically 5–10 iterations × support function call per iteration.
- Support function on N vertices: O(N) dot products.
- EPA: additional iterations (typically 3–8) to find closest point.
- Per pair: ~100–200 ops for GJK + ~50–100 ops for EPA = ~150–300 ops.
- At N=250: ~4.7M–9.4M FLOPs per substep — significantly higher than AABB/OBB.
- **Recommendation:** Use convex hulls only for player/important objects; keep asteroids as OBB or simplified AABBs.

---

## 6. Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Swept AABB too slow at 250+ entities | Performance regression, frame drops | Medium | Implement spatial partitioning (uniform grid) as Phase 4; fall back to discrete AABB if performance slips |
| TOI computation numerical instability at grazing angles | Entities jitter or slide along walls | Low | Clamp TOI to [1e-6, 0.999]; add small epsilon to overlap check |
| Swept AABB doesn't handle rotating objects correctly | Fast-spinning objects still tunnel | Medium | Add rotational sweep: test swept volume at multiple orientations (simplified: test at start and end rotation, take union) |
| GJK/EPA implementation bugs produce false positives/negatives | Gameplay-breaking collision errors | High | Extensive unit tests for every edge case; visual verification in demo scene |
| glTF mesh vertices → convex hull conversion is non-trivial | Cannot use detailed meshes as colliders | Medium | Pre-compute hulls offline (HullLib, CGAL); store simplified hulls in glTF extensions |

---

## 7. Acceptance Criteria

### Swept AABB (Phase 1)
- [ ] No entities tunnel through walls during physics simulation at max velocity (12 units/s).
- [ ] Arena OOB debug count remains zero over extended runtime (>5 minutes).
- [ ] Wall bounce timing is accurate: objects reflect at correct TOI, not after penetration.
- [ ] Energy conservation within 5% (E-M4 requirement maintained).
- [ ] All existing engine_tests pass (118 assertions / 51 test cases).

### OBB Collision (Phase 2)
- [ ] Player ship vs asteroid false positives eliminated (visual overlap = collision overlap).
- [ ] Asteroid-asteroid collision normals are accurate for rotated objects.
- [ ] Mixed AABB-OBB detection works correctly (cubes vs asteroids).
- [ ] Demo scene renders correctly with OBB colliders on all entities.

### Convex Hull Collision (Phase 3)
- [ ] Player ship uses convex hull collider from mesh vertices — zero false positives against static obstacles.
- [ ] GJK/EPA correctly reports intersection/non-intersection for all test pairs.
- [ ] Contact normals and depths are physically meaningful (used correctly in impulse response).
- [ ] Performance budget maintained: ≥30 FPS at 200 asteroids + player + enemy + active combat.

---

## 8. References

| Resource | Type | Link |
|----------|------|------|
| Erin Catto, "Continuous Dynamics" (GDC 2008) | Presentation | [youtube.com/watch?v=6HrDzskopUU](https://www.youtube.com/watch?v=6HrDzskopUU) |
| Ericson, Christer. *Real-Time Collision Detection*. Morgan Kaufmann, 2005. | Chapter 5 (AABB), Chapter 7 (OBB/SAT) | ISBN 978-0123872620 |
| Godot Engine, `physics_server_3d_sw.cpp` — GJK/EPA implementation | Open Source (MIT) | [github.com/godotengine/godot](https://github.com/godotengine/godot/blob/master/physics_servers.h) |
| Real-Time Collision Detection — swept AABB slab method | Online reference | [realtimecollisiondetection.net](http://www.realtimecollisiondetection.net/) |
| Bullet Physics, `btConvexHullIntersection` | Open Source (Zlib) | [github.com/bulletphysics/bullet3](https://github.com/bulletphysics/bullet3) |

---

## 9. Next Steps

1. **Immediate:** Implement swept AABB (Phase 1) to fix tunneling — resolves the OOB debug count issue in the demo scene.
2. **After G-M2:** Implement OBB (Phase 2) to eliminate player-asteroid false positives.
3. **Stretch:** Convex hull collision (Phase 3) if time permits and player ship mesh quality warrants it.

**Note:** This work belongs in the engine workstream (`src/engine/collider.cpp`, `src/engine/physics.cpp`). Game code consumes the updated API transparently — no interface changes required for Phase 1. Phase 2+ may require adding `OBBCollider` as a new component type, which is additive to the frozen engine interface (new component = no breaking change).
