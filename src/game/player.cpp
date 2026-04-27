#include "player.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/quaternion.hpp>
#include <sokol_app.h>

void player_update(float dt) {
    auto& reg = engine_registry();
    auto  view = reg.view<PlayerTag, Transform, RigidBody, Boost, Shield>();

    for (auto e : view) {
        auto& t     = view.get<Transform>(e);
        auto& rb    = view.get<RigidBody>(e);
        auto& boost = view.get<Boost>(e);
        auto& sh    = view.get<Shield>(e);

        // ----------------------------------------------------------------
        // Mouse look — pitch (local X) and yaw (world Y)
        // Mouse delta is already per-frame; no dt factor needed.
        // ----------------------------------------------------------------
        if (engine_mouse_button(0)) {
            const glm::vec2 delta = engine_mouse_delta();
            if (delta.x != 0.0f || delta.y != 0.0f) {
                const glm::vec3 local_right = t.rotation * glm::vec3(1.f, 0.f, 0.f);
                const glm::quat yaw   = glm::angleAxis(
                    -delta.x * constants::player_turn_speed,
                    glm::vec3(0.f, 1.f, 0.f));          // world Y — keeps horizon stable
                const glm::quat pitch = glm::angleAxis(
                    -delta.y * constants::player_turn_speed,
                    local_right);
                t.rotation = glm::normalize(yaw * t.rotation * pitch);
            }
        }

        // ----------------------------------------------------------------
        // Thrust and strafe — apply acceleration in local ship space
        // Player input is applied directly to velocity for immediate,
        // per-frame responsiveness (standard game-control pattern).
        // ----------------------------------------------------------------
        const glm::vec3 forward = t.rotation * glm::vec3(0.f, 1.f, 0.f);
        const glm::vec3 right   = t.rotation * glm::vec3(-1.f, 0.f, 0.f);

        const float thrust_accel = constants::player_thrust
            * (boost.active && boost.current > 0.f ? constants::boost_multiplier : 1.f);

        const bool is_thrusting =
            engine_key_down(SAPP_KEYCODE_W) ||
            engine_key_down(SAPP_KEYCODE_S) ||
            engine_key_down(SAPP_KEYCODE_A) ||
            engine_key_down(SAPP_KEYCODE_D);

        if (engine_key_down(SAPP_KEYCODE_W))
            rb.linear_velocity += forward * thrust_accel * dt;
        if (engine_key_down(SAPP_KEYCODE_S))
            rb.linear_velocity -= forward * thrust_accel * dt;
        if (engine_key_down(SAPP_KEYCODE_A))
            rb.linear_velocity -= right   * constants::player_strafe * dt;
        if (engine_key_down(SAPP_KEYCODE_D))
            rb.linear_velocity += right   * constants::player_strafe * dt;

        // Drag force — only applied when not thrusting, so the player can
        // accelerate freely.  Uses ForceAccum so Euler integration handles it
        // correctly across fixed-timestep substeps.
        if (!is_thrusting) {
            auto* drag_fb = engine_try_get_component<ForceAccum>(e);
            if (drag_fb) {
                drag_fb->force -= constants::player_drag_coeff * rb.linear_velocity;
            }
        }

        // Kill all angular velocity — player rotation is mouse-controlled only.
        // Physics collisions must not affect the ship's facing direction.
        rb.angular_velocity = glm::vec3(0.f);

        // ----------------------------------------------------------------
        // Boost — hold Space to activate; drain/regen handled in T013
        // ----------------------------------------------------------------
        boost.active = engine_key_down(SAPP_KEYCODE_SPACE);

        // ----------------------------------------------------------------
        // Boost drain / regen
        // Drain while held and not empty; regen only when released.
        // Holding Space with an empty boost bar neither drains nor regens
        // — the player must release to start recovery.
        // ----------------------------------------------------------------
        if (boost.active && boost.current > 0.f) {
            boost.current = std::max(0.f,
                boost.current - constants::boost_drain_rate * dt);
        } else if (!boost.active) {
            boost.current = std::min(boost.max,
                boost.current + constants::boost_regen_rate * dt);
        }

        // ----------------------------------------------------------------
        // Shield passive regen — gated by last_damage_time
        // Regen starts only after shield_regen_delay seconds of no damage.
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
