#include "weapons.h"
#include "camera_rig.h"
#include "constants.h"
#include "damage.h"
#include "spawn.h"
#include <engine.h>
#include <renderer.h>
#include <sokol_app.h>
#include <glm/gtc/quaternion.hpp>

// Impulse magnitude applied to asteroids struck by the laser (units·kg/s).
// Tunable in T028.
static constexpr float k_laser_impulse = 80.0f;

// Laser beam colour: bright cyan.
static constexpr float k_laser_color[4] = {0.3f, 0.85f, 1.0f, 1.0f};

// ---------------------------------------------------------------------------
// Cooldown query
// ---------------------------------------------------------------------------

bool weapon_ready(const WeaponState& ws) {
    const double now = engine_now();
    if (ws.active_weapon == WeaponType::Laser)
        return (now - ws.laser_last_fire) >= static_cast<double>(ws.laser_cooldown);
    return (now - ws.plasma_last_fire) >= static_cast<double>(ws.plasma_cooldown);
}

// ---------------------------------------------------------------------------
// laser_fire — instant raycast hit, line visual, damage + asteroid impulse
// ---------------------------------------------------------------------------

static void laser_fire(entt::entity player_e,
                          const Transform& t,
                          WeaponState& ws) {
    glm::vec3 ray_origin, ray_dir;
    camera_rig_cursor_ray(ray_origin, ray_dir);

    const glm::vec3 origin = t.position;

    // Beam always renders to max range (pass-through visual), damage only
    // applies to the first entity along the ray.
    const glm::vec3 end = origin + ray_dir * constants::laser_max_range;

    const auto hit = engine_raycast(origin, ray_dir, constants::laser_max_range);
    if (hit.has_value() && hit->entity != player_e) {
        // Damage target (shield → HP).
        apply_damage(hit->entity, ws.laser_damage);

        // Extra impulse for asteroid hits — kick them away from the laser.
        if (engine_has_component<AsteroidTag>(hit->entity))
            engine_apply_impulse(hit->entity, ray_dir * k_laser_impulse);
    }

    // Render beam from muzzle to max range.
    const float p0[3] = {origin.x, origin.y, origin.z};
    const float p1[3] = {end.x,    end.y,    end.z   };
    renderer_enqueue_line_quad(p0, p1, constants::laser_line_width, k_laser_color);

    ws.laser_last_fire = engine_now();
}

// ---------------------------------------------------------------------------
// plasma_fire — spawn a projectile entity in front of the ship
// ---------------------------------------------------------------------------

// Must match k_ship_half_extent in spawn.cpp.
static constexpr float k_ship_half = 1.5f;

static void plasma_fire(entt::entity player_e,
                             const Transform& t,
                             WeaponState& ws) {
    glm::vec3 ray_origin, ray_dir;
    camera_rig_cursor_ray(ray_origin, ray_dir);

    // Spawn position: slightly in front of ship nose along ship forward axis.
    const glm::vec3 rig_forward = t.rotation * glm::vec3(0.f, 1.f, 0.f);
    const glm::vec3 spawn_pos   = t.position
        + rig_forward * (k_ship_half + constants::plasma_sphere_radius + 0.1f);
    // Projectile velocity follows the cursor ray direction.
    const glm::vec3 velocity    = ray_dir * ws.plasma_speed;

    spawn_projectile(player_e, spawn_pos, velocity);
    ws.plasma_last_fire = engine_now();
}

// ---------------------------------------------------------------------------
// weapon_update
// ---------------------------------------------------------------------------

void weapon_update(float dt) {
    (void)dt;

    // Edge-trigger state for weapon switch.
    static bool s_prev_q = false;
    static bool s_prev_e = false;

    const bool cur_q = engine_key_down(SAPP_KEYCODE_Q);
    const bool cur_e = engine_key_down(SAPP_KEYCODE_E);

    const bool switch_plasma = cur_q && !s_prev_q;
    const bool switch_laser  = cur_e && !s_prev_e;

    s_prev_q = cur_q;
    s_prev_e = cur_e;

    const bool fire = engine_mouse_button(1);  // right mouse

    // Skip the registry scan entirely when there is no relevant input.
    if (!switch_plasma && !switch_laser && !fire)
        return;

    auto& reg  = engine_registry();
    auto  view = reg.view<PlayerTag, Transform, WeaponState>();
    for (auto e : view) {
        const auto& t  = view.get<Transform>(e);
        auto&       ws = view.get<WeaponState>(e);

        // --- Weapon switch (edge-triggered) ---------------------------------
        if (switch_laser)
            ws.active_weapon = WeaponType::Laser;
        else if (switch_plasma)
            ws.active_weapon = WeaponType::Plasma;

        // --- Fire -----------------------------------------------------------
        if (fire && weapon_ready(ws)) {
            if (ws.active_weapon == WeaponType::Laser)
                laser_fire(e, t, ws);
            else if (ws.active_weapon == WeaponType::Plasma)
                plasma_fire(e, t, ws);
        }
    }
}
