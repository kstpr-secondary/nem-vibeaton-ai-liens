#include "player.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/quaternion.hpp>
#include <sokol_app.h>
#include <cmath>

void player_update(float dt) {
    auto& reg = engine_registry();
    auto  view = reg.view<PlayerTag, Transform, RigidBody, Boost, Shield>();

    for (auto e : view) {
        auto& t     = view.get<Transform>(e);
        auto& rb    = view.get<RigidBody>(e);
        auto& boost = view.get<Boost>(e);
        auto& sh    = view.get<Shield>(e);

        // ----------------------------------------------------------------
        // Mouse look — yaw (world Y) and pitch (local X).
        // Incremental: apply mouse delta to the current rotation.
        // ----------------------------------------------------------------
        if (engine_mouse_button(0)) {
            const glm::vec2 delta = engine_mouse_delta();
            if (delta.x != 0.0f || delta.y != 0.0f) {
                // Yaw around world Y axis.
                const glm::quat q_yaw = glm::angleAxis(
                    -delta.x * constants::player_turn_speed,
                    glm::vec3(0.f, 1.f, 0.f));

                // Pitch around local right axis (extracted from current
                // rotation).  Post-multiply so it applies in ship-local space.
                const glm::vec3 local_right = t.rotation * glm::vec3(1.f, 0.f, 0.f);
                const glm::quat q_pitch = glm::angleAxis(
                    -delta.y * constants::player_turn_speed,
                    local_right);

                // Compose: yaw is applied in world space (pre-multiply),
                // pitch in local space (post-multiply).  Normalize to keep
                // the quaternion valid across many frames.
                t.rotation = glm::normalize(q_yaw * t.rotation * q_pitch);
            }
        }

        // ----------------------------------------------------------------
        // Velocity alignment — game-friendly turning feel.
        //
        // When LMB is held, sideways drift decays as the motion vector
        // rotates toward the nose direction.  "Turn left" means the ship's
        // travel direction adjusts to match its new facing.
        // ----------------------------------------------------------------
        const glm::vec3 facing = t.rotation * glm::vec3(0.f, 1.f, 0.f);
        const float speed = glm::length(rb.linear_velocity);

        if (speed > 0.1f) {
            const glm::vec3 vel_dir = rb.linear_velocity / speed;
            const float dot = glm::clamp(glm::dot(vel_dir, facing), -1.f, 1.f);
            const float angle = std::acos(dot);

            // Exponential decay toward nose-forward motion.
            const float align_rate = 2.0f;
            const float factor = 1.0f - std::exp(-align_rate * dt);
            const float rot_angle = angle * factor;

            if (rot_angle > 1e-6f) {
                const glm::vec3 axis = glm::cross(vel_dir, facing);
                const float axis_len = glm::length(axis);
                if (axis_len > 1e-6f) {
                    const glm::quat q_rot = glm::angleAxis(rot_angle, axis / axis_len);
                    rb.linear_velocity = glm::normalize(q_rot * rb.linear_velocity);
                    // Preserve speed magnitude — only direction changes.
                    rb.linear_velocity *= speed;
                }
            }
        }

        // ----------------------------------------------------------------
        // Thrust and strafe — apply acceleration in local ship space.
        // ----------------------------------------------------------------
        const glm::vec3 forward = t.rotation * glm::vec3(0.f, 1.f, 0.f);
        const glm::vec3 right   = t.rotation * glm::vec3(-1.f, 0.f, 0.f);

        const float thrust_accel = constants::player_thrust
            * (boost.active && boost.current > 0.f ? constants::boost_multiplier : 1.f);

        if (engine_key_down(SAPP_KEYCODE_W))
            rb.linear_velocity += forward * thrust_accel * dt;
        if (engine_key_down(SAPP_KEYCODE_S))
            rb.linear_velocity -= forward * thrust_accel * dt;
        if (engine_key_down(SAPP_KEYCODE_A))
            rb.linear_velocity -= right   * constants::player_strafe * dt;
        if (engine_key_down(SAPP_KEYCODE_D))
            rb.linear_velocity += right   * constants::player_strafe * dt;

        // Kill all angular velocity — player rotation is mouse-controlled only.
        // Physics collisions must not affect the ship's facing direction.
        rb.angular_velocity = glm::vec3(0.f);

        // ----------------------------------------------------------------
        // Boost — hold Space to activate; drain/regen handled below
        // ----------------------------------------------------------------
        boost.active = engine_key_down(SAPP_KEYCODE_SPACE);

        if (boost.active && boost.current > 0.f) {
            boost.current = std::max(0.f,
                boost.current - constants::boost_drain_rate * dt);
        } else if (!boost.active) {
            boost.current = std::min(boost.max,
                boost.current + constants::boost_regen_rate * dt);
        }

        // ----------------------------------------------------------------
        // Shield passive regen — gated by last_damage_time
        // ----------------------------------------------------------------
        const double time_since_hit = engine_now() - sh.last_damage_time;
        if (sh.current < sh.max &&
            time_since_hit > static_cast<double>(constants::shield_regen_delay))
        {
            sh.current = std::min(sh.max,
                sh.current + constants::shield_regen_rate * dt);
        }
    }
}
