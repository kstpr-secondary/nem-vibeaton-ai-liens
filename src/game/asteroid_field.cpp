#include "asteroid_field.h"
#include "components.h"
#include "constants.h"
#include "spawn.h"
#include <engine.h>
#include <glm/glm.hpp>
#include <cmath>
#include <random>

// Minimum distance from origin (player spawn) to avoid instant spawn damage.
static constexpr float k_min_spawn_radius = 80.0f;

// ---------------------------------------------------------------------------
// Random helpers
// ---------------------------------------------------------------------------

// Returns a uniformly distributed unit vector via the Gaussian method.
static glm::vec3 random_direction(std::mt19937& rng) {
    std::normal_distribution<float> nd(0.f, 1.f);
    glm::vec3 v(nd(rng), nd(rng), nd(rng));
    const float len = glm::length(v);
    // Extremely unlikely all three are zero, but guard anyway.
    return len > 1e-7f ? v / len : glm::vec3(0.f, 0.f, 1.f);
}

// Returns a point uniformly distributed in a spherical shell [r_min, r_max].
static glm::vec3 random_point_in_shell(std::mt19937& rng,
                                        float r_min, float r_max) {
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    // Uniform volume sampling: r = (r_min³ + u*(r_max³ - r_min³))^(1/3)
    const float r3_min = r_min * r_min * r_min;
    const float r3_max = r_max * r_max * r_max;
    const float r = std::cbrt(r3_min + dist(rng) * (r3_max - r3_min));
    return random_direction(rng) * r;
}

// ---------------------------------------------------------------------------
// Spawn helpers per tier
// ---------------------------------------------------------------------------

static void spawn_tier(std::mt19937& rng, SizeTier tier,
                        int count,
                        float speed_min, float speed_max,
                        float spin_max) {
    std::uniform_real_distribution<float> speed_dist(speed_min, speed_max);
    std::uniform_real_distribution<float> spin_dist(0.f, spin_max);

    for (int i = 0; i < count; ++i) {
        const glm::vec3 pos     = random_point_in_shell(rng,
                                      k_min_spawn_radius,
                                      constants::field_radius);
        const glm::vec3 lin_vel = random_direction(rng) * speed_dist(rng);
        const glm::vec3 ang_vel = random_direction(rng) * spin_dist(rng);

        spawn_asteroid(pos, lin_vel, ang_vel, tier);
    }
}

// ---------------------------------------------------------------------------
// asteroid_field_init
// ---------------------------------------------------------------------------

void asteroid_field_init() {
    std::mt19937 rng(42u);  // fixed seed — reproducible field layout

    const int total    = constants::asteroid_count;
    const int n_small  = static_cast<int>(total * constants::asteroid_tier_small_frac);
    const int n_medium = static_cast<int>(total * constants::asteroid_tier_medium_frac);
    const int n_large  = total - n_small - n_medium;

    spawn_tier(rng, SizeTier::Small,
               n_small,
               constants::asteroid_small_speed_min,
               constants::asteroid_small_speed_max,
               constants::asteroid_small_spin_max);

    spawn_tier(rng, SizeTier::Medium,
               n_medium,
               constants::asteroid_medium_speed_min,
               constants::asteroid_medium_speed_max,
               constants::asteroid_medium_spin_max);

    spawn_tier(rng, SizeTier::Large,
               n_large,
               constants::asteroid_large_speed_min,
               constants::asteroid_large_speed_max,
               constants::asteroid_large_spin_max);
}

// ---------------------------------------------------------------------------
// containment_update
// ---------------------------------------------------------------------------

// Velocity damping fraction applied to the player on a boundary hit.
// Tunable in T028.
static constexpr float k_player_boundary_damp = 0.5f;

// Reflect v off the boundary whose outward normal is n_out.
// Only reflects the outward component; inward-moving entities are left alone.
static glm::vec3 reflect_off_boundary(const glm::vec3& v,
                                       const glm::vec3& n_out) {
    const float vn = glm::dot(v, n_out);
    if (vn <= 0.f)
        return v;                          // already moving inward — no-op
    return v - 2.f * vn * n_out;
}

void containment_update() {
    auto& reg = engine_registry();

    // --- Asteroids: reflect + clamp speed + angular damping -----------------
    {
        auto view = reg.view<AsteroidTag, Transform, RigidBody>();
        for (auto e : view) {
            auto& t  = view.get<Transform>(e);
            auto& rb = view.get<RigidBody>(e);

            // Gentle angular damping so asteroids don't spin forever.
            rb.angular_velocity *= 0.995f;

            const float dist = glm::length(t.position);
            if (dist <= constants::field_radius)
                continue;

            // Push back to surface to prevent repeated triggering.
            const glm::vec3 n_out = t.position / dist;   // outward unit normal
            t.position = n_out * constants::field_radius;

            rb.linear_velocity = reflect_off_boundary(rb.linear_velocity, n_out);

            // Clamp speed so repeated bounces don't accumulate energy.
            const float speed = glm::length(rb.linear_velocity);
            if (speed > constants::max_asteroid_speed)
                rb.linear_velocity *= constants::max_asteroid_speed / speed;
        }
    }

    // --- Player: reflect + slight velocity damp -----------------------------
    {
        auto view = reg.view<PlayerTag, Transform, RigidBody>();
        for (auto e : view) {
            auto& t  = view.get<Transform>(e);
            auto& rb = view.get<RigidBody>(e);

            const float dist = glm::length(t.position);
            if (dist <= constants::field_radius)
                continue;

            const glm::vec3 n_out = t.position / dist;
            t.position = n_out * constants::field_radius;

            rb.linear_velocity = reflect_off_boundary(rb.linear_velocity, n_out)
                                 * k_player_boundary_damp;
        }
    }
}
