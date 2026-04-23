#include "physics.h"
#include "engine.h"
#include "collision_response.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

// ---------------------------------------------------------------------------
// Inertia tensor helpers
// ---------------------------------------------------------------------------

glm::mat3 make_box_inv_inertia_body(float mass, glm::vec3 half_extents) {
    float hx = half_extents.x, hy = half_extents.y, hz = half_extents.z;
    float Ix = (1.0f / 3.0f) * mass * (hy * hy + hz * hz);
    float Iy = (1.0f / 3.0f) * mass * (hx * hx + hz * hz);
    float Iz = (1.0f / 3.0f) * mass * (hx * hx + hy * hy);
    return glm::mat3(
        glm::vec3(1.0f / Ix, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f / Iy, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f / Iz)
    );
}

glm::mat3 make_sphere_inv_inertia_body(float mass, float radius) {
    float I = (2.0f / 5.0f) * mass * radius * radius;
    return glm::mat3(1.0f / I);  // Scalar * identity
}

void update_world_inertia(RigidBody& rb, const glm::quat& rotation,
                          const glm::mat3& inv_inertia_body) {
    glm::mat3 R = glm::mat3_cast(rotation);
    rb.inv_inertia = R * inv_inertia_body * glm::transpose(R);
}

// ---------------------------------------------------------------------------
// Integration
// ---------------------------------------------------------------------------

void integrate_linear(Transform& tr, RigidBody& rb, const glm::vec3& force_accum, float h) {
    if (rb.inv_mass == 0.0f) return;  // Static — skip
    glm::vec3 accel = force_accum * rb.inv_mass;
    rb.linear_velocity += accel * h;   // velocity first (semi-implicit)
    tr.position += rb.linear_velocity * h;
}

void integrate_angular(Transform& tr, RigidBody& rb, const glm::vec3& torque_accum, float h) {
    if (rb.inv_mass == 0.0f) return;
    glm::vec3 angular_accel = rb.inv_inertia * torque_accum;
    rb.angular_velocity += angular_accel * h;

    // Quaternion derivative: dq/dt = 0.5 * q_omega * q
    glm::quat q_omega(0.0f, rb.angular_velocity.x,
                             rb.angular_velocity.y,
                             rb.angular_velocity.z);
    tr.rotation += 0.5f * q_omega * tr.rotation * h;
    tr.rotation = glm::normalize(tr.rotation);  // MUST normalize every substep
}

// ---------------------------------------------------------------------------
// Physics substep loop
// ---------------------------------------------------------------------------

void physics_substep(entt::registry& reg, float h) {
    // 1. Clear force accumulators
    auto accum_view = reg.view<ForceAccum>();
    for (auto e : accum_view) {
        accum_view.get<ForceAccum>(e) = {};
    }

    // 2. Integrate all Dynamic bodies (exclude Static tag)
    auto dyn_view = reg.view<Transform, RigidBody, Dynamic, ForceAccum>();
    dyn_view.each([&](auto e, Transform& tr, RigidBody& rb, ForceAccum& fa) {
        // Update world-space inertia before integration
        update_world_inertia(rb, tr.rotation, rb.inv_inertia_body);

        // Integrate linear and angular state
        integrate_linear(tr, rb, fa.force, h);
        integrate_angular(tr, rb, fa.torque, h);

        // Update world-space inertia again after orientation changes
        // (collision response needs current orientation)
        update_world_inertia(rb, tr.rotation, rb.inv_inertia_body);
    });

    // 3. Collision detection + response
    resolve_all_collisions(reg);
}

void physics_system_tick(entt::registry& reg, float dt, float physics_hz, float dt_cap) {
    if (dt <= 0.f) {
        fprintf(stderr, "[ENGINE] physics: dt <= 0 (%.6f); clamping to 0.0001s\n",
                static_cast<double>(dt));
        dt = 0.0001f;
    }
    if (dt > dt_cap) {
        dt = dt_cap;
    }

    constexpr float H = 1.0f / 120.0f;  // Fixed substep size (120 Hz)
    static float accumulator = 0.0f;

    accumulator += dt;
    while (accumulator >= H) {
        physics_substep(reg, H);
        accumulator -= H;
    }
}
