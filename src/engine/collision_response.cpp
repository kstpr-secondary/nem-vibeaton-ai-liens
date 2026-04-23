#include "collision_response.h"
#include "engine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// ---------------------------------------------------------------------------
// Impulse-based elastic collision response
// ---------------------------------------------------------------------------

void resolve_collision(Transform& tr_a, RigidBody& rb_a,
                       Transform& tr_b, RigidBody& rb_b,
                       const CollisionPair& contact) {
    const glm::vec3& n = contact.normal;   // B → A
    const glm::vec3& cp = contact.point;

    // Radius vectors from center of mass to contact point
    glm::vec3 r_a = cp - tr_a.position;
    glm::vec3 r_b = cp - tr_b.position;

    // Velocity of contact point on each body: v_cm + omega × r
    glm::vec3 vel_a_at_cp = rb_a.linear_velocity + glm::cross(rb_a.angular_velocity, r_a);
    glm::vec3 vel_b_at_cp = rb_b.linear_velocity + glm::cross(rb_b.angular_velocity, r_b);

    // Relative velocity along normal
    float v_rel_n = glm::dot(vel_a_at_cp - vel_b_at_cp, n);

    // Bodies already separating — no impulse needed
    if (v_rel_n >= 0.0f) return;

    // Restitution: use minimum (both should be 1.0 for elastic)
    float e = std::min(rb_a.restitution, rb_b.restitution);

    // Angular contribution terms
    glm::vec3 ang_a = glm::cross(rb_a.inv_inertia * glm::cross(r_a, n), r_a);
    glm::vec3 ang_b = glm::cross(rb_b.inv_inertia * glm::cross(r_b, n), r_b);
    float angular_denom = glm::dot(n, ang_a) + glm::dot(n, ang_b);

    // Impulse magnitude
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

// ---------------------------------------------------------------------------
// Positional correction (Baumgarte)
// ---------------------------------------------------------------------------

void positional_correction(Transform& tr_a, const RigidBody& rb_a,
                           Transform& tr_b, const RigidBody& rb_b,
                           const CollisionPair& contact) {
    constexpr float K_SLOP       = 0.005f;   // Allow tiny overlap before correcting
    constexpr float K_BAUMGARTE  = 0.3f;     // Fraction of penetration to correct per substep

    float total_inv_mass = rb_a.inv_mass + rb_b.inv_mass;
    if (total_inv_mass == 0.0f) return;   // Both static — impossible but guard anyway

    float correction_mag = std::max(contact.depth - K_SLOP, 0.0f)
                           * K_BAUMGARTE / total_inv_mass;
    glm::vec3 correction = correction_mag * contact.normal;

    tr_a.position += correction * rb_a.inv_mass;
    tr_b.position -= correction * rb_b.inv_mass;
}

// ---------------------------------------------------------------------------
// Collision resolution pipeline
// ---------------------------------------------------------------------------

void resolve_all_collisions(entt::registry& reg) {
    // Detect all collisions using the collider system
    std::vector<CollisionPair> pairs = detect_collisions();

    // Resolve each collision pair
    for (const auto& pair : pairs) {
        // Skip if penetration is non-positive (shouldn't happen, but guard)
        if (pair.depth <= 0.0f) continue;

        auto& tr_a = reg.get<Transform>(pair.a);
        auto& tr_b = reg.get<Transform>(pair.b);

        // Check if entities have RigidBody (Dynamic) or are Static
        bool a_has_rb = reg.all_of<RigidBody>(pair.a);
        bool b_has_rb = reg.all_of<RigidBody>(pair.b);

        // Static sentinel for bodies without RigidBody
        static RigidBody zero_rb{};
        zero_rb.inv_mass = 0.0f;
        zero_rb.inv_inertia = glm::mat3(0.0f);
        zero_rb.restitution = 1.0f;

        auto& rb_a = a_has_rb ? reg.get<RigidBody>(pair.a) : zero_rb;
        auto& rb_b = b_has_rb ? reg.get<RigidBody>(pair.b) : zero_rb;

        // Resolve collision with impulse response
        resolve_collision(tr_a, rb_a, tr_b, rb_b, pair);

        // Apply positional correction to prevent sinking
        positional_correction(tr_a, rb_a, tr_b, rb_b, pair);
    }
}
