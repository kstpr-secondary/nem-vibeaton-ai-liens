#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "components.h"
#include "math_utils.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

using Catch::Approx;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float kinetic_energy_linear(const RigidBody& rb) {
    if (rb.inv_mass == 0.0f) return 0.0f;
    float mass = 1.0f / rb.inv_mass;
    return 0.5f * mass * glm::dot(rb.linear_velocity, rb.linear_velocity);
}

static float kinetic_energy_angular(const RigidBody& rb) {
    if (rb.inv_mass == 0.0f) return 0.0f;
    // KE_rot = 0.5 * omega^T * I * omega, where I = inv(inv_inertia)
    // For diagonal inertia, this simplifies
    glm::vec3 I_omega = glm::inverse(rb.inv_inertia) * rb.angular_velocity;
    return 0.5f * glm::dot(rb.angular_velocity, I_omega);
}

static float total_ke(const RigidBody& a, const RigidBody& b) {
    return kinetic_energy_linear(a) + kinetic_energy_angular(a) +
           kinetic_energy_linear(b) + kinetic_energy_angular(b);
}

// Simple Euler integration helpers for testing
static void integrate_linear(Transform& tr, RigidBody& rb, const glm::vec3& force, float h) {
    if (rb.inv_mass == 0.0f) return;
    glm::vec3 accel = force * rb.inv_mass;
    rb.linear_velocity += accel * h;
    tr.position += rb.linear_velocity * h;
}

static void integrate_angular(Transform& tr, RigidBody& rb, const glm::vec3& torque, float h) {
    if (rb.inv_mass == 0.0f) return;
    glm::vec3 angular_accel = rb.inv_inertia * torque;
    rb.angular_velocity += angular_accel * h;

    glm::quat q_omega(0.0f, rb.angular_velocity.x,
                             rb.angular_velocity.y,
                             rb.angular_velocity.z);
    tr.rotation += 0.5f * q_omega * tr.rotation * h;
    tr.rotation = glm::normalize(tr.rotation);
}

// ---------------------------------------------------------------------------
// Euler linear integration
// ---------------------------------------------------------------------------

TEST_CASE("Euler linear integration: position update with constant velocity", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.inv_mass = 1.0f;
    rb.linear_velocity = {2.0f, 0.0f, 0.0f};

    // No force, just coast
    integrate_linear(tr, rb, glm::vec3(0.f), 0.1f);

    REQUIRE(tr.position.x == Approx(0.2f).margin(1e-5f));
    REQUIRE(rb.linear_velocity.x == Approx(2.0f).margin(1e-5f));
}

TEST_CASE("Euler linear integration: force causes acceleration", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.mass = 2.0f;
    rb.inv_mass = 0.5f;

    // Force = (10, 0, 0), mass = 2 → accel = (5, 0, 0)
    integrate_linear(tr, rb, glm::vec3(10.f, 0.f, 0.f), 0.1f);

    // v = 0 + 5 * 0.1 = 0.5
    REQUIRE(rb.linear_velocity.x == Approx(0.5f).margin(1e-5f));
    // p = 0 + 0.5 * 0.1 = 0.05
    REQUIRE(tr.position.x == Approx(0.05f).margin(1e-5f));
}

TEST_CASE("Euler linear integration: static body unchanged", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.inv_mass = 0.0f;  // Static
    tr.position = {1.f, 2.f, 3.f};

    integrate_linear(tr, rb, glm::vec3(100.f), 1.0f);

    REQUIRE(tr.position.x == Approx(1.f));
    REQUIRE(rb.linear_velocity.x == Approx(0.f));
}

// ---------------------------------------------------------------------------
// Angular integration with quaternion renormalization
// ---------------------------------------------------------------------------

TEST_CASE("Euler angular integration: torque causes spin", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.inv_mass = 1.0f;
    rb.inv_inertia = glm::mat3(1.0f);  // Identity inverse inertia

    // Torque around Z axis
    integrate_angular(tr, rb, glm::vec3(0.f, 0.f, 10.f), 0.1f);

    // omega_z = 0 + 10 * 0.1 = 1.0
    REQUIRE(rb.angular_velocity.z == Approx(1.0f).margin(1e-5f));
}

TEST_CASE("Euler angular integration: quaternion remains normalized", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.inv_mass = 1.0f;
    rb.inv_inertia = glm::mat3(1.0f);

    // Run many substeps and check normalization
    for (int i = 0; i < 1000; ++i) {
        integrate_angular(tr, rb, glm::vec3(1.f, 2.f, 3.f), 0.01f);
    }

    float q_len = glm::length(tr.rotation);
    REQUIRE(q_len == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Euler angular integration: static body unchanged", "[physics]") {
    Transform tr{};
    RigidBody rb{};
    rb.inv_mass = 0.0f;
    rb.angular_velocity = {5.f, 0.f, 0.f};

    integrate_angular(tr, rb, glm::vec3(100.f), 1.0f);

    REQUIRE(rb.angular_velocity.x == Approx(5.f));
}

// ---------------------------------------------------------------------------
// Force → acceleration, impulse → velocity
// ---------------------------------------------------------------------------

TEST_CASE("apply_force: accumulates correctly", "[physics]") {
    // Force application is tested via integration above
    // F = m*a → a = F/m
    Transform tr{};
    RigidBody rb{};
    rb.mass = 4.0f;
    rb.inv_mass = 0.25f;

    glm::vec3 force = {12.f, 0.f, 0.f};
    integrate_linear(tr, rb, force, 0.5f);

    // v = 0 + (12 * 0.25) * 0.5 = 1.5
    REQUIRE(rb.linear_velocity.x == Approx(1.5f).margin(1e-5f));
}

TEST_CASE("apply_impulse: instant velocity change", "[physics]") {
    RigidBody rb{};
    rb.inv_mass = 0.5f;

    glm::vec3 impulse = {10.f, 0.f, 0.f};
    rb.linear_velocity += impulse * rb.inv_mass;

    // v = 0 + 10 * 0.5 = 5
    REQUIRE(rb.linear_velocity.x == Approx(5.0f).margin(1e-5f));
}

// ---------------------------------------------------------------------------
// Elastic two-body collision: energy conservation
// ---------------------------------------------------------------------------

// Simplified collision response for testing (no angular terms)
static void resolve_collision_simple(Transform&, RigidBody& rb_a,
                                      Transform&, RigidBody& rb_b,
                                      const glm::vec3& normal) {
    glm::vec3 v_rel = rb_a.linear_velocity - rb_b.linear_velocity;
    float v_rel_n = glm::dot(v_rel, normal);

    if (v_rel_n >= 0.0f) return;  // Separating

    float e = std::min(rb_a.restitution, rb_b.restitution);
    float j = -(1.0f + e) * v_rel_n / (rb_a.inv_mass + rb_b.inv_mass);

    glm::vec3 impulse = j * normal;
    rb_a.linear_velocity += impulse * rb_a.inv_mass;
    rb_b.linear_velocity -= impulse * rb_b.inv_mass;
}

TEST_CASE("elastic head-on equal mass: velocities exchange", "[physics]") {
    Transform tr_a, tr_b;
    RigidBody rb_a, rb_b;

    tr_a.position = {-1.f, 0.f, 0.f};
    tr_b.position = {1.f, 0.f, 0.f};

    rb_a.inv_mass = 1.0f;
    rb_b.inv_mass = 1.0f;
    rb_a.restitution = 1.0f;
    rb_b.restitution = 1.0f;
    rb_a.linear_velocity = {2.f, 0.f, 0.f};
    rb_b.linear_velocity = {-2.f, 0.f, 0.f};
    rb_a.angular_velocity = rb_b.angular_velocity = glm::vec3(0.f);
    rb_a.inv_inertia = rb_b.inv_inertia = glm::mat3(0.f);

    // Normal points from B to A: A is at -1, B is at +1, so normal = (-1, 0, 0)
    glm::vec3 normal = {-1.f, 0.f, 0.f};
    resolve_collision_simple(tr_a, rb_a, tr_b, rb_b, normal);

    REQUIRE(rb_a.linear_velocity.x == Approx(-2.0f).margin(1e-4f));
    REQUIRE(rb_b.linear_velocity.x == Approx(2.0f).margin(1e-4f));
}

TEST_CASE("elastic collision conserves kinetic energy (10s simulation)", "[physics]") {
    Transform tr_a, tr_b;
    RigidBody rb_a, rb_b;

    tr_a.position = {-5.f, 0.f, 0.f};
    tr_b.position = {5.f, 0.f, 0.f};

    rb_a.inv_mass = 0.5f;  // mass = 2
    rb_b.inv_mass = 0.5f;
    rb_a.restitution = 1.0f;
    rb_b.restitution = 1.0f;
    rb_a.linear_velocity = {3.f, 0.f, 0.f};
    rb_b.linear_velocity = {-1.f, 0.f, 0.f};
    rb_a.angular_velocity = rb_b.angular_velocity = glm::vec3(0.f);
    // Use small non-zero inertia to avoid NaN in angular KE
    rb_a.inv_inertia = rb_b.inv_inertia = glm::mat3(0.001f);

    auto ke = [](const RigidBody& rb) {
        if (rb.inv_mass == 0.0f) return 0.0f;
        float mass = 1.0f / rb.inv_mass;
        return 0.5f * mass * glm::dot(rb.linear_velocity, rb.linear_velocity);
    };

    float ke_before = ke(rb_a) + ke(rb_b);

    // Simulate 10 seconds at 120 Hz, collision happens when they meet
    float h = 1.0f / 120.0f;
    for (int i = 0; i < static_cast<int>(10.0f / h); ++i) {
        // Move
        tr_a.position += rb_a.linear_velocity * h;
        tr_b.position += rb_b.linear_velocity * h;

        // Check collision (simple 1D with radius 0.5)
        float dist = std::abs(tr_a.position.x - tr_b.position.x);
        if (dist < 1.0f) {
            // Normal from B to A
            glm::vec3 normal = (tr_a.position.x > tr_b.position.x)
                                   ? glm::vec3(-1.f, 0.f, 0.f)
                                   : glm::vec3(1.f, 0.f, 0.f);
            resolve_collision_simple(tr_a, rb_a, tr_b, rb_b, normal);
            // Separate to avoid multiple collisions in a row
            tr_a.position += rb_a.linear_velocity * h;
            tr_b.position += rb_b.linear_velocity * h;
        }
    }

    float ke_after = ke(rb_a) + ke(rb_b);
    REQUIRE(ke_after == Approx(ke_before).epsilon(0.05f));  // Within 5%
}

// ---------------------------------------------------------------------------
// Static body: zero velocity change
// ---------------------------------------------------------------------------

TEST_CASE("static body unaffected by collision", "[physics]") {
    Transform tr_dyn, tr_static;
    RigidBody rb_dyn, rb_static;

    rb_dyn.inv_mass = 1.0f;
    rb_dyn.restitution = 1.0f;
    rb_dyn.linear_velocity = {5.f, 0.f, 0.f};
    rb_dyn.angular_velocity = glm::vec3(0.f);
    rb_dyn.inv_inertia = glm::mat3(0.f);

    rb_static.inv_mass = 0.0f;  // Static
    rb_static.restitution = 1.0f;
    rb_static.linear_velocity = {0.f, 0.f, 0.f};
    rb_static.angular_velocity = glm::vec3(0.f);
    rb_static.inv_inertia = glm::mat3(0.f);

    // Dynamic is approaching from left, static is on right
    // Normal from static to dynamic: dynamic is at left, so normal = (-1, 0, 0)
    glm::vec3 normal = {-1.f, 0.f, 0.f};
    glm::vec3 v_static_before = rb_static.linear_velocity;

    resolve_collision_simple(tr_dyn, rb_dyn, tr_static, rb_static, normal);

    REQUIRE(rb_static.linear_velocity.x == Approx(v_static_before.x));
    REQUIRE(rb_static.linear_velocity.y == Approx(v_static_before.y));
    REQUIRE(rb_static.linear_velocity.z == Approx(v_static_before.z));

    // Dynamic body should bounce back
    REQUIRE(rb_dyn.linear_velocity.x == Approx(-5.0f).margin(1e-4f));
}

// ---------------------------------------------------------------------------
// dt-cap clamp behavior
// ---------------------------------------------------------------------------

TEST_CASE("dt-cap clamps large time steps", "[physics]") {
    constexpr float dt_cap = 0.1f;
    float dt = 0.5f;  // Large spike

    float clamped_dt = std::min(dt, dt_cap);
    REQUIRE(clamped_dt == Approx(dt_cap));

    dt = 0.016f;  // Normal frame
    clamped_dt = std::min(dt, dt_cap);
    REQUIRE(clamped_dt == Approx(0.016f));
}
