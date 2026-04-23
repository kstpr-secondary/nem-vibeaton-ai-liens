# Euler Integration Reference

Full GLM code patterns for rigid-body state integration. Use this when implementing `physics.cpp` or the force/torque accumulation API (`apply_force`, `apply_impulse_at_point`).

---

## Semi-implicit Euler (symplectic)

Semi-implicit Euler updates **velocity first using end-of-step acceleration**, then position using the new velocity. This is first-order like explicit Euler but energy-stable for oscillatory motion — far better for bouncing bodies.

```cpp
// Linear integration — one substep of size h
void integrate_linear(Transform& tr, RigidBody& rb, const glm::vec3& force_accum, float h) {
    if (rb.inv_mass == 0.0f) return;  // Static — skip
    glm::vec3 accel = force_accum * rb.inv_mass;
    rb.linear_velocity  += accel * h;   // velocity first (semi-implicit)
    tr.position         += rb.linear_velocity * h;
}

// Angular integration — one substep of size h
void integrate_angular(Transform& tr, RigidBody& rb, const glm::vec3& torque_accum, float h) {
    if (rb.inv_mass == 0.0f) return;
    glm::vec3 angular_accel = rb.inv_inertia * torque_accum;
    rb.angular_velocity += angular_accel * h;

    // Quaternion derivative: dq/dt = 0.5 * q_omega * q
    // where q_omega = quat(0, omega.x, omega.y, omega.z) — pure quaternion
    glm::quat q_omega(0.0f, rb.angular_velocity.x,
                             rb.angular_velocity.y,
                             rb.angular_velocity.z);
    tr.rotation += 0.5f * q_omega * tr.rotation * h;
    tr.rotation = glm::normalize(tr.rotation);  // MUST normalize every substep
}
```

**Why semi-implicit?** Explicit Euler would be: `v_new = v + a*h; p_new = p + v_old*h`. Using `v_old` for position makes energy grow slowly over time. Semi-implicit uses `v_new` for position — energy is nearly conserved for conservative forces.

---

## World-space inertia update

Body-space `inv_inertia_body` is constant (set at spawn). World-space `rb.inv_inertia` must be refreshed every substep after orientation changes.

```cpp
// Store body-space inverse inertia in a separate component or init struct
void update_world_inertia(RigidBody& rb, const glm::quat& rotation,
                          const glm::mat3& inv_inertia_body) {
    glm::mat3 R = glm::mat3_cast(rotation);
    rb.inv_inertia = R * inv_inertia_body * glm::transpose(R);
}
```

Call this after angular integration but before collision resolution — the collision impulse formula uses `rb.inv_inertia`.

---

## Inertia tensors

### Solid box — half-extents (hx, hy, hz), mass m

The factor `1/3` (not `1/12`) applies because the formula is for half-extents. Full-side-length formula uses `1/12 * m * (a² + b²)` where a, b are full lengths; with half-extents substitute `a = 2*hx` to get `(1/12)*m*(4hx²+4hz²) = (1/3)*m*(hx²+hz²)`.

```cpp
glm::mat3 box_inv_inertia_body(float mass, glm::vec3 half_extents) {
    float hx = half_extents.x, hy = half_extents.y, hz = half_extents.z;
    float Ix = (1.0f/3.0f) * mass * (hy*hy + hz*hz);
    float Iy = (1.0f/3.0f) * mass * (hx*hx + hz*hz);
    float Iz = (1.0f/3.0f) * mass * (hx*hx + hy*hy);
    return glm::mat3(
        1.0f/Ix,    0.0f,    0.0f,
           0.0f, 1.0f/Iy,    0.0f,
           0.0f,    0.0f, 1.0f/Iz
    );
}
```

### Solid sphere — radius r, mass m

```cpp
glm::mat3 sphere_inv_inertia_body(float mass, float radius) {
    float I = (2.0f/5.0f) * mass * radius * radius;
    return glm::mat3(1.0f / I);   // scalar * identity
}
```

### Notes on GLM mat3 constructor

`glm::mat3(c0, c1, c2)` where c0–c2 are `vec3` columns. The scalar constructor `glm::mat3(s)` produces `s` on the diagonal (identity * s). The nine-scalar constructor fills **column-major**: first three scalars are column 0.

```cpp
// Diagonal matrix — preferred for inertia:
glm::mat3 diag(float a, float b, float c) {
    return glm::mat3(glm::vec3(a,0,0), glm::vec3(0,b,0), glm::vec3(0,0,c));
}
```

---

## Force and torque accumulation

Forces and torques reset to zero after every substep. Accumulate them before calling integration.

```cpp
struct ForceAccum {
    glm::vec3 force  = glm::vec3(0.0f);
    glm::vec3 torque = glm::vec3(0.0f);
};

// Apply a force at a world-space point (generates both linear force and torque)
void apply_force_at_point(ForceAccum& accum,
                          const glm::vec3& force,
                          const glm::vec3& world_point,
                          const glm::vec3& center_of_mass) {
    accum.force  += force;
    accum.torque += glm::cross(world_point - center_of_mass, force);
}

// Apply a central force (no torque)
void apply_force(ForceAccum& accum, const glm::vec3& force) {
    accum.force += force;
}

// Apply an impulse directly (instantaneous velocity change, bypasses accumulator)
void apply_impulse(RigidBody& rb, const glm::vec3& impulse) {
    rb.linear_velocity += impulse * rb.inv_mass;
}

void apply_impulse_at_point(RigidBody& rb,
                             const glm::vec3& impulse,
                             const glm::vec3& world_point,
                             const glm::vec3& center_of_mass) {
    rb.linear_velocity  += impulse * rb.inv_mass;
    rb.angular_velocity += rb.inv_inertia * glm::cross(world_point - center_of_mass, impulse);
}
```

**Impulse vs force.** Use `apply_force` for continuous effects (gravity, thrust). Use `apply_impulse_at_point` for instantaneous events (collision response, explosion). Forces integrate over the substep; impulses take effect immediately on the velocity.

---

## Substep loop with force clearing

```cpp
void physics_substep(entt::registry& reg, float h) {
    // 1. Clear accumulators
    auto accum_view = reg.view<ForceAccum>();
    for (auto e : accum_view) accum_view.get<ForceAccum>(e) = {};

    // 2. Apply external forces (game layer may have pushed forces already;
    //    engine adds gravity if any — project uses space setting so gravity = 0)

    // 3. Integrate
    auto dyn_view = reg.view<Transform, RigidBody, ForceAccum>();
    for (auto e : dyn_view) {
        auto& [tr, rb, fa] = dyn_view.get<Transform, RigidBody, ForceAccum>(e);
        update_world_inertia(rb, tr.rotation, get_inv_inertia_body(e, reg));
        integrate_linear(tr, rb, fa.force, h);
        integrate_angular(tr, rb, fa.torque, h);
        update_world_inertia(rb, tr.rotation, get_inv_inertia_body(e, reg));
        // update again so collision response uses current orientation
    }

    // 4. Collision detection + response (see physics-elastic-collision.md)
    resolve_collisions(reg, h);
}
```

`get_inv_inertia_body` should read a `RigidBodyInit` component or equivalent that stores the body-space tensor computed at entity creation.

---

## Gravity note

The project is a space shooter in an asteroid field. **Gravity is zero by default.** Do not add a global gravity constant in the engine physics module. If the game layer wants gravitational attraction between asteroids (stretch), it should call `apply_force_at_point` itself.
