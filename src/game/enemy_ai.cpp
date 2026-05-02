#include "enemy_ai.h"
#include "components.h"
#include "constants.h"
#include "spawn.h"
#include <engine.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Half-extent of the enemy ship cube — used to offset projectile spawn point
// so the projectile starts outside the enemy's own collider.
static constexpr float k_ship_half   = 1.5f;

// Base orientation to map glTF model (+Y up, nose along +Y) to game space (-Z forward, +Y up).
static const glm::quat k_ship_base_orientation = 
    glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) * 
    glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));

void enemy_ai_update(float /*dt*/) {
    auto& reg = engine_registry();

    // Locate the player — if dead or not spawned, enemies hold position.
    auto player_view = reg.view<PlayerTag, Transform>();
    if (player_view.begin() == player_view.end())
        return;

    const entt::entity player_e  = *player_view.begin();
    const glm::vec3    player_pos =
        player_view.get<Transform>(player_e).position;

    // -----------------------------------------------------------------------
    // Per-enemy update
    // -----------------------------------------------------------------------
    auto enemy_view = reg.view<EnemyTag, Transform, RigidBody, EnemyAI, WeaponState>();
    for (auto e : enemy_view) {
        auto& t  = enemy_view.get<Transform>(e);
        auto& rb = enemy_view.get<RigidBody>(e);
        auto& ai = enemy_view.get<EnemyAI>(e);
        auto& ws = enemy_view.get<WeaponState>(e);

        const glm::vec3 to_player = player_pos - t.position;
        const float     dist      = glm::length(to_player);

        if (dist < 1e-6f)
            continue;

        const glm::vec3 dir = to_player / dist;

        // -------------------------------------------------------------------
        // Seek: set velocity toward player, but stop when close enough to
        // avoid fighting the physics collision response (twitching).
        // -------------------------------------------------------------------
        static constexpr float k_stop_distance = 5.0f;
        if (dist > k_stop_distance)
            rb.linear_velocity = dir * ai.pursuit_speed;
        else
            rb.linear_velocity *= 0.9f;  // damp when stopped

        // Face the player so the visual nose points along the seek direction.
        // Compose with model base orientation: WorldLookAt * ModelBase.
        t.rotation = glm::quatLookAtRH(dir, glm::vec3(0.f, 1.f, 0.f)) * k_ship_base_orientation;

        // Kill angular velocity — enemy rotation is AI-controlled only.
        rb.angular_velocity = glm::vec3(0.f);

        // -------------------------------------------------------------------
        // Fire: range + LOS check + cooldown.
        // -------------------------------------------------------------------
        if (dist > ai.fire_range)
            continue;

        const double now = engine_now();
        if ((now - ai.last_fire_time) < static_cast<double>(ai.fire_cooldown))
            continue;

        // Calculate actual forward direction for spawning and velocity.
        const glm::vec3 ship_forward = t.rotation * glm::vec3(0.0f, 0.0f, -1.0f);

        // Line-of-sight via engine_raycast.
        const auto hit = engine_raycast(t.position, ship_forward, dist);
        if (hit.has_value() && hit->entity != player_e)
            continue;  // something blocks the shot

        // Spawn projectile from just outside the enemy's own collider.
        const glm::vec3 spawn_pos = t.position
            + ship_forward * (k_ship_half + constants::plasma_sphere_radius + 0.1f);
        // Inherit enemy velocity so the shot respects relative motion.
        const glm::vec3 proj_vel = rb.linear_velocity
            + ship_forward * ws.plasma_speed;

        spawn_projectile(e, spawn_pos, proj_vel);
        ai.last_fire_time = now;
    }
}
