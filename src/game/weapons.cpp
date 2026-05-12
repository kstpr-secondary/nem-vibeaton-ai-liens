#include "weapons.h"
#include "camera_rig.h"
#include "constants.h"
#include "damage.h"
#include "spawn.h"
#include "vfx.h"
#include <engine.h>
#include <renderer.h>
#include <sokol_app.h>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <cmath>

// ---------------------------------------------------------------------------
// Cooldown query
// ---------------------------------------------------------------------------

bool weapon_ready(const WeaponState& ws) {
    const double now = engine_now();
    if (ws.active_weapon == WeaponType::Laser)
        return (now - ws.laser_last_fire) >= static_cast<double>(ws.laser_cooldown);
    return (now - ws.plasma_last_fire) >= static_cast<double>(ws.plasma_cooldown);
}

// Must match k_enemy_half_extent in spawn.cpp (2.0f) plus a small buffer.
static constexpr float k_ship_half = 2.1f;

// ---------------------------------------------------------------------------
// Sphere-ray intersection helper — finds the near-point t along ray ro+rd*t
// where the ray enters a sphere centered at `center` with radius `radius`.
// Returns -1.0f if no intersection.
// ---------------------------------------------------------------------------

static float sphere_intersect_t(const glm::vec3& ro, const glm::vec3& rd,
                                const glm::vec3& center, float radius) {
    glm::vec3 oc = ro - center;
    float b    = glm::dot(oc, rd);
    float c    = glm::dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) return -1.0f;
    return -b - sqrtf(disc);
}

// ---------------------------------------------------------------------------
// laser_update — handles firing and fading phases of the laser beam.
// Called every frame from weapon_update() for all entities with LaserBeam.
// ---------------------------------------------------------------------------

static void laser_update(entt::entity /*player_e*/, float dt) {
    auto& reg = engine_registry();
    auto  view = reg.view<LaserBeam, Transform>();

    for (auto e : view) {
        auto& lb   = view.get<LaserBeam>(e);
        auto& t    = view.get<Transform>(e);

        double fire_age = engine_now() - lb.fire_time;

        // --- Firing phase ---------------------------------------------------
        if (fire_age < static_cast<double>(lb.fire_duration)) {
            // Detect early release: clamp remaining duration to begin fading.
            if (!engine_mouse_button(1)) {
                lb.fire_duration = static_cast<float>(fire_age);
            }

            // Compute muzzle origin from ship forward axis.
            const glm::vec3 ship_forward = t.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
            const float offset = k_ship_half + 0.1f;
            const glm::vec3 muzzle_origin = t.position + ship_forward * offset;

            // Get cursor ray from camera — origin + direction define the world aim ray.
            glm::vec3 ray_origin, ray_dir;
            camera_rig_cursor_ray(ray_origin, ray_dir);

            // Default: short fixed-distance endpoint from the muzzle along cursor direction.
            lb.end = muzzle_origin + ray_dir * constants::laser_default_range;
            lb.last_hit_entity = entt::null;

            // Find nearest enemy under cursor using direct AABB slab test — the same
            // approach the HUD uses. engine_raycast is not used here because it returns
            // the first obstacle in the scene (usually one of the 600 asteroids), making
            // the enemy check on the result always fail.
            {
                auto ev = reg.view<EnemyTag, Transform, Collider>();
                entt::entity hit_enemy = entt::null;
                float hit_t = constants::laser_max_range;

                for (auto ee : ev) {
                    const auto& et = ev.get<Transform>(ee);
                    const auto& ec = ev.get<Collider>(ee);
                    const glm::vec3 inv_d = 1.f / ray_dir;
                    const glm::vec3 t_min = (et.position - ec.half_extents - ray_origin) * inv_d;
                    const glm::vec3 t_max = (et.position + ec.half_extents - ray_origin) * inv_d;
                    const glm::vec3 t1    = glm::min(t_min, t_max);
                    const glm::vec3 t2    = glm::max(t_min, t_max);
                    const float t_near    = glm::compMax(t1);
                    const float t_far     = glm::compMin(t2);
                    if (t_far >= t_near && t_near > 0.f && t_near < hit_t) {
                        hit_t = t_near;
                        hit_enemy = ee;
                    }
                }

                if (hit_enemy != entt::null) {
                    apply_damage(hit_enemy, constants::laser_dps * dt);

                    const auto* target_t   = engine_try_get_component<Transform>(hit_enemy);
                    auto*       target_sh  = engine_try_get_component<Shield>(hit_enemy);
                    const auto* target_col = engine_try_get_component<Collider>(hit_enemy);

                    glm::vec3 hit_point = ray_origin + ray_dir * hit_t;
                    bool shielded = false;

                    if (target_sh && target_sh->current > 0.f && target_col && target_t) {
                        float shield_r     = target_col->half_extents.x * constants::shield_sphere_scale;
                        float shield_t_val = sphere_intersect_t(ray_origin, ray_dir, target_t->position, shield_r);
                        if (shield_t_val > 0.0f) {
                            hit_point = ray_origin + ray_dir * shield_t_val;
                            shielded  = true;
                        }
                    }

                    lb.end = hit_point;

                    if (hit_enemy != lb.last_hit_entity) {
                        if (shielded) spawn_shield_impact(hit_point);
                        else          spawn_laser_impact(hit_point);
                    }
                    lb.last_hit_entity = hit_enemy;
                }
            }

            // Track current muzzle position so fading phase starts from the right spot.
            lb.origin = muzzle_origin;

            // Submit full-brightness beam (two additive line quads).
            float opacity = std::min(1.0f, static_cast<float>(fire_age) * 8.0f)
                          * (1.0f + 0.04f * sinf(static_cast<float>(fire_age) * 25.0f));

            const float p0[3] = {muzzle_origin.x, muzzle_origin.y, muzzle_origin.z};
            const float p1[3] = {lb.end.x, lb.end.y, lb.end.z};

            renderer_enqueue_line_quad(p0, p1,
                constants::laser_core_width * opacity, constants::laser_core_color,
                BlendMode::Additive);
            renderer_enqueue_line_quad(p0, p1,
                constants::laser_halo_width * opacity, constants::laser_halo_color,
                BlendMode::Additive);

        // --- Fading phase ---------------------------------------------------
        } else if (fire_age < static_cast<double>(lb.fire_duration + lb.fade_duration)) {
            // No raycast, no damage. Beam end stays fixed.
            lb.last_hit_entity = entt::null;

            float fade_t = static_cast<float>(fire_age - lb.fire_duration) / lb.fade_duration;
            float opacity = 1.0f - fade_t;

            const float p0[3] = {lb.origin.x, lb.origin.y, lb.origin.z};
            const float p1[3] = {lb.end.x, lb.end.y, lb.end.z};

            renderer_enqueue_line_quad(p0, p1,
                constants::laser_core_width * opacity, constants::laser_core_color,
                BlendMode::Additive);
            renderer_enqueue_line_quad(p0, p1,
                constants::laser_halo_width * opacity, constants::laser_halo_color,
                BlendMode::Additive);

            // Remove beam when fade complete.
            if (fade_t >= 1.0f) {
                reg.remove<LaserBeam>(e);
            }
        } else {
            // Past fade — remove.
            reg.remove<LaserBeam>(e);
        }
    }
}

// ---------------------------------------------------------------------------
// laser_charging — handles the charging phase of the laser.
// Called every frame from weapon_update() for all entities with LaserCharge.
// ---------------------------------------------------------------------------

static void laser_charging(entt::entity /*player_e*/, WeaponState& ws) {
    auto& reg = engine_registry();
    auto  view = reg.view<LaserCharge, Transform>();

    for (auto e : view) {
        auto& lc      = view.get<LaserCharge>(e);
        auto& t       = view.get<Transform>(e);

        double age = engine_now() - lc.charge_start;

        // Check for early release — cancelled, no cooldown.
        if (!engine_mouse_button(1)) {
            reg.remove<LaserCharge>(e);
            continue;
        }

        // Compute opacity: grows from 0 to 0.40 over charge time.
        float opacity = std::min(1.0f, static_cast<float>(age) / lc.charge_time) * 0.40f;

        // Muzzle origin from ship forward axis.
        const glm::vec3 ship_forward = t.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        const float offset = k_ship_half + 0.1f;
        const glm::vec3 muzzle_origin = t.position + ship_forward * offset;

        // Cursor ray for aim preview. Default to fixed short range; snap to enemy if found.
        glm::vec3 ray_origin, ray_dir;
        camera_rig_cursor_ray(ray_origin, ray_dir);

        glm::vec3 beam_end = muzzle_origin + ray_dir * constants::laser_default_range;
        {
            auto ev = reg.view<EnemyTag, Transform, Collider>();
            float hit_t = constants::laser_max_range;
            for (auto ee : ev) {
                const auto& et = ev.get<Transform>(ee);
                const auto& ec = ev.get<Collider>(ee);
                const glm::vec3 inv_d = 1.f / ray_dir;
                const glm::vec3 t_min = (et.position - ec.half_extents - ray_origin) * inv_d;
                const glm::vec3 t_max = (et.position + ec.half_extents - ray_origin) * inv_d;
                const glm::vec3 t1    = glm::min(t_min, t_max);
                const glm::vec3 t2    = glm::max(t_min, t_max);
                const float t_near    = glm::compMax(t1);
                const float t_far     = glm::compMin(t2);
                if (t_far >= t_near && t_near > 0.f && t_near < hit_t) {
                    hit_t  = t_near;
                    beam_end = ray_origin + ray_dir * t_near;
                }
            }
        }

        // Submit dim aiming beam (two additive line quads).
        const float p0[3] = {muzzle_origin.x, muzzle_origin.y, muzzle_origin.z};
        const float p1[3] = {beam_end.x, beam_end.y, beam_end.z};

        renderer_enqueue_line_quad(p0, p1,
            constants::laser_core_width * 0.5f * opacity, constants::laser_core_color,
            BlendMode::Additive);
        renderer_enqueue_line_quad(p0, p1,
            constants::laser_halo_width * 0.7f * opacity, constants::laser_halo_color,
            BlendMode::Additive);

        // Charge complete — transition to firing phase.
        if (age >= static_cast<double>(lc.charge_time)) {
            reg.remove<LaserCharge>(e);

            LaserBeam lb;
            lb.fire_time = engine_now();
            lb.fire_duration = constants::laser_fire_duration;
            // fade_duration uses default 0.5f from component definition.
            lb.origin = muzzle_origin;
            lb.end = beam_end;
            lb.last_hit_entity = entt::null;

            reg.emplace<LaserBeam>(e, std::move(lb));

            // Start cooldown now (at the charge→fire transition).
            ws.laser_last_fire = engine_now();
        }
    }
}

// ---------------------------------------------------------------------------
// plasma_fire — spawn a projectile entity in front of the ship
// ---------------------------------------------------------------------------

static void plasma_fire(entt::entity player_e,
                       const Transform& t,
                       WeaponState& ws) {
    glm::vec3 ray_origin, ray_dir;
    camera_rig_cursor_ray(ray_origin, ray_dir);

    // Spawn position: slightly in front of ship nose along ship forward axis.
    const glm::vec3 rig_forward = t.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
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

    // Edge-trigger for right mouse (laser charge start).
    static bool s_prev_fire = false;
    const bool fire_pressed = engine_mouse_button(1) && !s_prev_fire;
    s_prev_fire = engine_mouse_button(1);

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

        // --- Start laser charge on fire press (if ready) --------------------
        if (ws.active_weapon == WeaponType::Laser && fire_pressed && weapon_ready(ws)) {
            LaserCharge lc;
            lc.charge_start = engine_now();
            lc.charge_time  = constants::laser_charge_time;
            reg.emplace<LaserCharge>(e, std::move(lc));
        }

        // --- Plasma fire (unchanged: instant on press) ----------------------
        if (ws.active_weapon == WeaponType::Plasma && engine_mouse_button(1) && weapon_ready(ws)) {
            plasma_fire(e, t, ws);
        }

        // --- Laser update (charging / firing / fading) ----------------------
        laser_charging(e, ws);
        laser_update(e, dt);
    }
}
