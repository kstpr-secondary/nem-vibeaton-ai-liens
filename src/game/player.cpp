#include "player.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/quaternion.hpp>
#include <sokol_app.h>
#include <cmath>

void player_update(float dt) {
    auto& reg = engine_registry();
    auto view = reg.view<PlayerTag, Transform, RigidBody, Boost, Shield, CameraRigState>();

    for (auto e : view) {
        auto& rb    = view.get<RigidBody>(e);
        auto& boost = view.get<Boost>(e);
        auto& sh    = view.get<Shield>(e);
        auto& crs   = view.get<CameraRigState>(e);

        // ----------------------------------------------------------------
        // Thrust and strafe — use rig_rotation (no visual bank).
        // ----------------------------------------------------------------
        const glm::vec3 forward = crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f);
        const glm::vec3 right   = crs.rig_rotation * glm::vec3(1.f, 0.f, 0.f);

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
