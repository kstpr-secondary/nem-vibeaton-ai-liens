---
name: physics-euler
description: Use when implementing or reviewing rigid-body physics in the game-engine workstream — linear and angular Euler integration, impulse-based elastic collision response, AABB collision geometry, inertia tensors, fixed-timestep substepping. Covers the full 6-DOF simulation needed for asteroids that behave like billiard balls in space (energy-conserving, restitution = 1). Activate for E-M4 (Euler physics + collision response), physics unit-test authoring, RigidBody component design, force/impulse API, and any task touching `physics.cpp`, `collision_response.cpp`, or `time.cpp`.
---

# Euler Rigid-Body Physics

Provides the formulas, GLM patterns, and project-specific rules needed to implement 6-DOF rigid-body dynamics for the asteroid-field space shooter. Target: solid elastic collisions — asteroids behave like billiard balls in vacuum, with full linear and angular momentum conserved.

This skill is pre-implementation. Every formula is domain-standard and confirmed against the project's stated component schema; see §3. Exact code structure is not yet frozen — implement the formulas as described, adapt to the actual engine file layout.

---

## 1. Objective

Help an agent implement E-M4 (Euler physics + collision response) correctly and completely:

- Semi-implicit Euler integration of linear and angular state.
- Impulse-based elastic collision response with full angular terms.
- AABB-vs-AABB collision detection (project's MVP collider type).
- Fixed-timestep substepping to prevent tunnelling and simulation drift.
- `Static` tag support (`inv_mass = 0`), zero extra logic.

The simulation must conserve kinetic energy (linear + rotational) for elastic collisions (e = 1.0) and conserve linear and angular momentum.

---

## 2. Scope

**In scope**
- 6-DOF rigid bodies: position, orientation (quaternion), linear velocity, angular velocity, accumulated forces/torques.
- Inertia tensor construction for box and sphere shapes at component creation.
- World-space inertia tensor update each frame from body-space inverse inertia and current rotation.
- Semi-implicit (symplectic) Euler integration — velocity-first, then position.
- Fixed-timestep substep loop with dt cap.
- AABB-vs-AABB overlap, penetration depth, and contact normal.
- Impulse-resolution formula including angular terms (linear + rotational denominator).
- Positional correction to prevent sinking (Baumgarte).
- Force and impulse accumulation API (`apply_force`, `apply_impulse_at_point`).

**Out of scope**
- Continuous collision detection (CCD / swept AABB) — stretch goal only.
- Convex-hull or sphere colliders — stretch.
- Spatial partitioning (BVH, uniform grid) — brute-force pairs at MVP.
- Friction, damping, rolling resistance — may be added by game layer; not engine-owned at MVP.
- Joints, constraints, ragdoll — cut entirely.
- Sleeping / deactivation — cut at MVP.

---

## 3. Project-confirmed component schema

From `pre_planning_docs/Game Engine Concept and Milestones.md` and `engine-specialist/SKILL.md` §3.

```
RigidBody {
    mass:             float     // > 0 always; 0 is illegal (use Static tag instead)
    inv_mass:         float     // 1/mass; precomputed at spawn
    linear_velocity:  vec3
    angular_velocity: vec3      // world-space ω, radians/s
    inv_inertia:      mat3      // world-space I⁻¹; recomputed each step from body-space
    restitution:      float     // coefficient of restitution; 1.0 = perfectly elastic
}

Collider { AABB { half_extents: vec3 } }  // local space; world AABB = pos ± half_extents

Tag: Static    // inv_mass = 0; never moved by integrator or impulse
Tag: Dynamic   // has RigidBody + Collider; moved by physics
```

Force and torque accumulators are **reset to zero after each substep**. They are not stored in the component — accumulate into a per-step local structure or a separate `ForceAccum` component cleared each physics tick.

---

## 4. Physics model at a glance

```
Each fixed substep h:

  1. Accumulate external forces (gravity, thrust, game-applied forces).
  2. Integrate linear:   v ← v + (F_total / m) * h
                         p ← p + v * h
  3. Integrate angular:  ω ← ω + I_world_inv * τ_total * h
                         q ← normalize(q + 0.5 * quat(0, ω) * q * h)
  4. Update world-space inertia inverse from new orientation.
  5. Broadphase: brute-force all (Dynamic, Dynamic) and (Dynamic, Static) AABB pairs.
  6. Narrowphase: AABB-vs-AABB overlap test per pair.
  7. Collision response: compute impulse from restitution + angular terms; apply.
  8. Position correction: push bodies apart by remaining penetration * Baumgarte factor.
  9. Clear force/torque accumulators.
```

The order matters. Always resolve collisions **after** integration, not before.

---

## 5. Fixed-timestep loop

```cpp
constexpr float PHYSICS_HZ  = 120.0f;
constexpr float H            = 1.0f / PHYSICS_HZ;   // substep size
constexpr float DT_CAP       = 0.1f;                 // prevent spiral-of-death

static float accumulator = 0.0f;

void physics_system_tick(entt::registry& reg, float dt) {
    accumulator += std::min(dt, DT_CAP);
    while (accumulator >= H) {
        physics_substep(reg, H);
        accumulator -= H;
    }
}
```

- **Do not use variable-dt Euler.** A single 200 ms frame spike will tunnel fast-moving asteroids through geometry at variable dt.
- 120 Hz substeps are cheap (brute-force AABB pairs, no spatial DS at MVP).
- Increase substep rate or add CCD only if tunnelling is observed in demo; do not pre-optimise.

---

## 6. Inertia tensors for project shapes

Compute **body-space inverse** at entity creation. Convert to world-space each physics substep.

### Solid box (AABB half-extents hx, hy, hz)
```cpp
float Ix = (1.0f/3.0f) * mass * (hy*hy + hz*hz);
float Iy = (1.0f/3.0f) * mass * (hx*hx + hz*hz);
float Iz = (1.0f/3.0f) * mass * (hx*hx + hy*hy);
glm::mat3 inv_inertia_body = glm::mat3(
    1.0f/Ix, 0, 0,
    0, 1.0f/Iy, 0,
    0, 0, 1.0f/Iz
);
```

### Solid sphere (radius r)
```cpp
float I = (2.0f/5.0f) * mass * r * r;
glm::mat3 inv_inertia_body = glm::mat3(1.0f/I); // diagonal, all equal
```

### World-space update (run each substep after orientation changes)
```cpp
glm::mat3 R = glm::mat3_cast(transform.rotation);  // rotation matrix from quat
rb.inv_inertia = R * inv_inertia_body * glm::transpose(R);
```

Store `inv_inertia_body` separately (a `RigidBodyInit` component, or computed once and cached). `rb.inv_inertia` is overwritten each substep — it is a **derived, transient** value.

---

## 7. Decision rules

- **Prefer semi-implicit (symplectic) Euler** over explicit Euler. Update velocity first using acceleration, then update position using the new velocity. This is energy-stable for harmonic oscillators and gives much better long-term behavior for orbiting/bouncing bodies than explicit Euler.
- **Prefer `inv_mass` / `inv_inertia`** stored in components over dividing by mass per frame. Inverting once at spawn avoids per-substep divisions and makes static bodies trivially handled by `inv_mass = 0`.
- **Prefer full angular impulse terms** in the collision response denominator. Dropping them gives wrong impulse magnitudes for non-centroidal contacts — asteroid spins will be wrong.
- **Prefer AABB contact normal from minimum-penetration axis.** The SAT axis with smallest overlap gives the most stable contact — avoids jitter from axis switching.
- **Prefer single impulse per contact pair per substep.** Do not accumulate impulses or use iterative solvers at MVP; one-shot impulse is sufficient for asteroid-field sparsity.
- **Prefer Baumgarte positional correction** over projection. Baumgarte is one multiply + one add per body; it prevents sinking without the velocity artifacts of full projection. Use `k_slop = 0.005` and `k_baumgarte = 0.3`.
- **Prefer brute-force pair iteration** at MVP. Spatial partitioning is a Stretch milestone. N < 100 asteroids is well within O(N²) budget at 120 Hz.
- **Escalate if** a game task requests friction, joint constraints, CCD, or sleeping — these are not in scope and should be flagged rather than quietly added.

---

## 8. Gotchas

- **Quaternion drift.** Euler quaternion integration accumulates floating-point error. **Always renormalize** q after integration: `q = glm::normalize(q)`. Skipping this causes slow orientation drift that eventually becomes NaN.
- **World-space `inv_inertia` must be recomputed every substep.** If you cache it across frames without updating from the current rotation, spinning asteroids will have wrong rotational responses — the bug is silent until contact.
- **AABB ignores rotation at MVP.** The project's AABB collider stays axis-aligned even when the entity rotates. This is intentional (§3 of engine-specialist). Do not add OBB narrowphase; flag if a task asks for it.
- **Contact normal must point from body B to body A** (i.e., from the *other* body toward *this* body) consistently throughout the impulse formula. Swapping the sign here flips the impulse direction and bodies attract instead of repel.
- **Restitution blending.** When two bodies collide use `e = min(rb_A.restitution, rb_B.restitution)` or the product, not the average. The project targets e = 1.0 for all asteroids; the formula still needs a valid e so don't hardcode 1.0 — pass it through the component.
- **Static tag means `inv_mass = 0` and `inv_inertia = mat3(0)`.** The impulse formula automatically produces zero velocity change for that body — no special-case branching needed.
- **dt cap hides bugs.** The DT_CAP = 0.1 s prevents spiral-of-death but if the cap triggers repeatedly the simulation is running behind. Log a warning; don't silently eat frames.
- **GLM column-major constructors.** `glm::mat3(a, b, c, d, e, f, g, h, i)` fills *columns*, not rows. `glm::mat3_cast(q)` is correct for rotation; do not roll a custom quat-to-mat.

---

## 9. Validation

Before marking an E-M4 task `READY_FOR_VALIDATION`:

1. Two equal-mass, head-on elastic bodies exchange velocities completely (like billiard balls). Check: `v_A_after ≈ v_B_before`, `v_B_after ≈ v_A_before` for linear case without angular.
2. Total kinetic energy (½mv² + ½ω·Iω) is conserved within 0.1% per collision in `engine_tests`.
3. Linear momentum (m_A * v_A + m_B * v_B) before = after (within float precision).
4. Static bodies do not move after any number of impulses (`inv_mass = 0` path).
5. Fast-moving asteroids do not tunnel through static walls at 120 Hz substep rate in `engine_app` demo.
6. Quaternions remain normalized after 60 seconds of continuous rotation in `engine_tests`.
7. `engine_app` demo shows a cluster of dynamic rigid cubes with applied impulses producing visible bouncing and spinning. Build passes: `cmake --build build --target engine_app engine_tests`.

---

## 10. File loading

- Read `references/physics-euler-integration.md` when implementing the integrator (`physics.cpp`) or constructing inertia tensors. Contains full GLM code patterns for integration and tensor math.
- Read `references/physics-elastic-collision.md` when implementing collision response (`collision_response.cpp`) or writing Catch2 tests for the impulse formula. Contains the full derivation, formula, contact-point handling, and positional correction code.

---

## 11. Companion skills

- `engine-specialist/SKILL.md` — owns the task schema, milestone playbooks (E-M4 section), and file-disjoint PG split rules for integrator vs collision-response.
- `entt-ecs/SKILL.md` — `view<Transform, RigidBody, Collider>` iteration, deferred destruction, structural-change safety.

---

## 12. Evolution

After E-M4 ships:
- Add Catch2 test results and any formula corrections to the references.
- If sphere colliders are needed (E-M7 capsule implies sphere), add sphere-vs-sphere impulse to `references/physics-elastic-collision.md`.
- If spatial partitioning is added (Stretch), add a section on BVH/uniform-grid broadphase here.
- If friction is added by the game layer, note the friction impulse extension pattern.
