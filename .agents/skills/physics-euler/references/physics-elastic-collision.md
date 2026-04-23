# Elastic Collision Reference

AABB collision detection, impulse-based elastic response with angular terms, positional correction, and energy-conservation validation. Use when implementing `collision_response.cpp` or writing Catch2 tests for the impulse formula.

---

## AABB-vs-AABB overlap

The project MVP uses axis-aligned bounding boxes in world space. Rotation is ignored for collision geometry — the AABB uses `transform.position ± half_extents` regardless of orientation (engine-specialist §4.3).

```cpp
struct AABB {
    glm::vec3 min, max;  // world-space corners
};

AABB world_aabb(const Transform& tr, const Collider& col) {
    return { tr.position - col.half_extents,
             tr.position + col.half_extents };
}

struct Contact {
    bool       colliding;
    glm::vec3  normal;       // unit vector pointing from B toward A
    float      penetration;  // overlap depth along normal, always >= 0
    glm::vec3  point;        // world-space contact point
};

Contact aabb_vs_aabb(const AABB& a, const AABB& b) {
    // Overlap per axis
    float ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    float oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f)
        return { false };

    // Minimum penetration axis = contact normal
    glm::vec3 normal{};
    float penetration{};
    glm::vec3 ca = (a.min + a.max) * 0.5f;  // center A
    glm::vec3 cb = (b.min + b.max) * 0.5f;  // center B

    if (ox <= oy && ox <= oz) {
        penetration = ox;
        normal = glm::vec3(ca.x > cb.x ? 1.0f : -1.0f, 0.0f, 0.0f);
    } else if (oy <= ox && oy <= oz) {
        penetration = oy;
        normal = glm::vec3(0.0f, ca.y > cb.y ? 1.0f : -1.0f, 0.0f);
    } else {
        penetration = oz;
        normal = glm::vec3(0.0f, 0.0f, ca.z > cb.z ? 1.0f : -1.0f);
    }

    // Contact point = midpoint of overlap region
    glm::vec3 overlap_min = glm::max(a.min, b.min);
    glm::vec3 overlap_max = glm::min(a.max, b.max);
    glm::vec3 point = (overlap_min + overlap_max) * 0.5f;

    return { true, normal, penetration, point };
}
```

---

## Impulse-based elastic collision response

### Derivation summary

The impulse **j** along the contact normal **n** (pointing from B to A) is chosen so that the relative normal velocity after collision satisfies the restitution condition:

```
v_rel_n_after = -e * v_rel_n_before
```

where `v_rel_n_before = dot(v_rel, n)` and `v_rel = v_A_contact - v_B_contact`.

Velocity at contact point for a rigid body:
```
v_contact = v_cm + omega × r      (r = contact_point - center_of_mass)
```

Solving for j:
```
       -(1 + e) * dot(v_rel, n)
j = ─────────────────────────────────────────────────────────────
    inv_mass_A + inv_mass_B
    + dot(n, (I_A_inv * (r_A × n)) × r_A)
    + dot(n, (I_B_inv * (r_B × n)) × r_B)
```

Apply impulse:
```
v_A  ← v_A  + j * n * inv_mass_A
v_B  ← v_B  - j * n * inv_mass_B
ω_A  ← ω_A  + I_A_inv * (r_A × (j * n))
ω_B  ← ω_B  - I_B_inv * (r_B × (j * n))
```

### Full implementation

```cpp
void resolve_collision(Transform& tr_a, RigidBody& rb_a,
                       Transform& tr_b, RigidBody& rb_b,
                       const Contact& contact) {
    const glm::vec3& n = contact.normal;   // B → A
    const glm::vec3& cp = contact.point;

    // Radius vectors from center of mass to contact point
    glm::vec3 r_a = cp - tr_a.position;
    glm::vec3 r_b = cp - tr_b.position;

    // Velocity of contact point on each body: v_cm + omega × r
    glm::vec3 vel_a_at_cp = rb_a.linear_velocity + glm::cross(rb_a.angular_velocity, r_a);
    glm::vec3 vel_b_at_cp = rb_b.linear_velocity + glm::cross(rb_b.angular_velocity, r_b);

    float v_rel_n = glm::dot(vel_a_at_cp - vel_b_at_cp, n);

    // Bodies already separating — no impulse needed
    if (v_rel_n >= 0.0f) return;

    float e = std::min(rb_a.restitution, rb_b.restitution);

    // Angular contribution terms
    glm::vec3 ang_a = glm::cross(rb_a.inv_inertia * glm::cross(r_a, n), r_a);
    glm::vec3 ang_b = glm::cross(rb_b.inv_inertia * glm::cross(r_b, n), r_b);
    float angular_denom = glm::dot(n, ang_a) + glm::dot(n, ang_b);

    float j = -(1.0f + e) * v_rel_n
              / (rb_a.inv_mass + rb_b.inv_mass + angular_denom);

    // Apply linear impulse
    glm::vec3 impulse = j * n;
    rb_a.linear_velocity += impulse * rb_a.inv_mass;
    rb_b.linear_velocity -= impulse * rb_b.inv_mass;

    // Apply angular impulse
    rb_a.angular_velocity += rb_a.inv_inertia * glm::cross(r_a, impulse);
    rb_b.angular_velocity -= rb_b.inv_inertia * glm::cross(r_b, impulse);
}
```

**Note on `inv_mass = 0` (Static tag).** The formula is branch-free. When `inv_mass_A = 0` and `inv_inertia_A = mat3(0)`, both linear and angular velocity changes for body A are zero — the static body is not moved regardless of impulse magnitude.

---

## Positional correction (Baumgarte)

Impulse alone does not remove the penetration that triggered the collision (bodies remain partially overlapping). Without correction they sink into each other over many frames.

```cpp
void positional_correction(Transform& tr_a, const RigidBody& rb_a,
                            Transform& tr_b, const RigidBody& rb_b,
                            const Contact& contact) {
    constexpr float K_SLOP       = 0.005f;   // allow tiny overlap before correcting
    constexpr float K_BAUMGARTE  = 0.3f;     // fraction of penetration to correct per substep

    float total_inv_mass = rb_a.inv_mass + rb_b.inv_mass;
    if (total_inv_mass == 0.0f) return;   // both static — impossible but guard anyway

    float correction_mag = std::max(contact.penetration - K_SLOP, 0.0f)
                           * K_BAUMGARTE / total_inv_mass;
    glm::vec3 correction = correction_mag * contact.normal;

    tr_a.position += correction * rb_a.inv_mass;
    tr_b.position -= correction * rb_b.inv_mass;
}
```

Call `positional_correction` immediately after `resolve_collision` for the same contact pair.

---

## Broadphase pair iteration

Brute-force at MVP. Build the pair list from all Dynamic entities; check each Dynamic against all Dynamic + Static.

```cpp
void resolve_collisions(entt::registry& reg, float /*h*/) {
    // Collect all collidable entities with their world AABBs
    struct ColEntry {
        entt::entity entity;
        AABB         aabb;
        bool         is_static;
    };
    std::vector<ColEntry> entries;
    entries.reserve(128);

    reg.view<Transform, Collider>().each([&](auto e, const Transform& tr, const Collider& col) {
        bool is_static = reg.all_of<Tag::Static>(e);
        entries.push_back({ e, world_aabb(tr, col), is_static });
    });

    // Test every unique pair
    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = i + 1; j < entries.size(); ++j) {
            if (entries[i].is_static && entries[j].is_static) continue;

            Contact c = aabb_vs_aabb(entries[i].aabb, entries[j].aabb);
            if (!c.colliding) continue;

            auto& tr_a = reg.get<Transform>(entries[i].entity);
            auto& tr_b = reg.get<Transform>(entries[j].entity);
            // Static bodies have no RigidBody component — use zero-mass sentinel
            static RigidBody zero_rb = []{ RigidBody r{}; r.inv_mass=0; return r; }();
            auto& rb_a = entries[i].is_static ? zero_rb : reg.get<RigidBody>(entries[i].entity);
            auto& rb_b = entries[j].is_static ? zero_rb : reg.get<RigidBody>(entries[j].entity);

            resolve_collision(tr_a, rb_a, tr_b, rb_b, c);
            positional_correction(tr_a, rb_a, tr_b, rb_b, c);
        }
    }
}
```

**Alternative static design.** If Static entities *do* have a `RigidBody` with `inv_mass = 0`, remove the sentinel entirely and iterate `view<Transform, RigidBody, Collider>`. Either approach works; pick one and be consistent with the component schema freeze.

---

## Catch2 test templates

### Test 1 — Head-on equal-mass elastic: velocities exchange

```cpp
TEST_CASE("elastic head-on equal mass") {
    Transform tr_a, tr_b;
    RigidBody rb_a, rb_b;
    tr_a.position = glm::vec3(-1, 0, 0);
    tr_b.position = glm::vec3( 1, 0, 0);
    rb_a.inv_mass = rb_b.inv_mass = 1.0f;
    rb_a.restitution = rb_b.restitution = 1.0f;
    rb_a.inv_inertia = rb_b.inv_inertia = glm::mat3(0.0f);  // no rotation
    rb_a.linear_velocity = glm::vec3(2, 0, 0);
    rb_b.linear_velocity = glm::vec3(-2, 0, 0);
    rb_a.angular_velocity = rb_b.angular_velocity = glm::vec3(0);

    Contact c{ true, glm::vec3(1, 0, 0), 0.0f, glm::vec3(0) };
    resolve_collision(tr_a, rb_a, tr_b, rb_b, c);

    REQUIRE(rb_a.linear_velocity.x == Approx(-2.0f).margin(1e-4f));
    REQUIRE(rb_b.linear_velocity.x == Approx( 2.0f).margin(1e-4f));
}
```

### Test 2 — Energy conservation with angular terms

```cpp
TEST_CASE("elastic collision conserves kinetic energy") {
    // ... set up two rotating bodies ...
    float KE_before = kinetic_energy(rb_a, rb_b);
    resolve_collision(tr_a, rb_a, tr_b, rb_b, c);
    float KE_after  = kinetic_energy(rb_a, rb_b);
    REQUIRE(KE_after == Approx(KE_before).epsilon(0.001f));  // within 0.1%
}

float kinetic_energy(const RigidBody& a, const RigidBody& b) {
    auto linear_ke = [](const RigidBody& rb) {
        return rb.inv_mass > 0 ? 0.5f / rb.inv_mass * glm::dot(rb.linear_velocity, rb.linear_velocity) : 0.0f;
    };
    auto angular_ke = [](const RigidBody& rb) {
        // KE_rot = 0.5 * omega · I * omega = 0.5 * omega · (inv_I)^-1 * omega
        // Simpler via I = inv(inv_I): use transpose(inv_I) = inv_I for symmetric
        // Compute omega^T * I * omega; I = inv(inv_inertia) is expensive — instead
        // accept a slight test approximation and use inv_inertia * omega as proxy,
        // OR pass inv_inertia_body to compute I from it.
        // For the test, compute I from body-space values passed in.
        return 0.0f;  // fill in with actual I when test is written
    };
    return linear_ke(a) + angular_ke(a) + linear_ke(b) + angular_ke(b);
}
```

### Test 3 — Static body unaffected

```cpp
TEST_CASE("static body unaffected by impulse") {
    // rb_static.inv_mass = 0; rb_static.inv_inertia = mat3(0)
    glm::vec3 static_v_before = rb_static.linear_velocity;
    resolve_collision(tr_dyn, rb_dyn, tr_static, rb_static, c);
    REQUIRE(rb_static.linear_velocity == static_v_before);
}
```

---

## Common mistakes

- **Forgetting to check `v_rel_n >= 0`.** Without the separation guard the impulse is applied even when bodies are moving apart after a previous substep's response — velocities diverge explosively.
- **Using body-center instead of contact-point for r vectors.** If `r_a = glm::vec3(0)` (center-of-mass contact), angular terms disappear. Use the actual contact point.
- **Wrong normal direction.** The normal must point from B toward A (i.e., from the "other" body toward "this" body) before entering the formula. If A flies through B after response, the normal is inverted.
- **Mixing world-space and body-space inertia.** `rb.inv_inertia` must be world-space (rotated from body-space each substep). Using body-space tensors in world-space cross products gives wrong spin axes.
- **Single static sentinel for Static entities.** If multiple Static contacts are resolved in one substep and you share a mutable sentinel, data races or accumulated velocity changes corrupt results. Use `inv_mass = 0` directly on the RigidBody component or a const zero sentinel.
