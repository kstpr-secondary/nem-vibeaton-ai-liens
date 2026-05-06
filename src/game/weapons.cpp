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

static void laser_update(entt::entity player_e, float dt) {
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

            // Get cursor ray direction for raycast.
            glm::vec3 ray_origin, ray_dir;
            camera_rig_cursor_ray(ray_origin, ray_dir);
            (void)ray_origin;

            // Raycast from muzzle along cursor direction.
            const auto hit = engine_raycast(muzzle_origin, ray_dir, constants::laser_max_range);

            if (hit.has_value() && hit->entity != player_e) {
                // Apply DPS-scaled damage.
                apply_damage(hit->entity, constants::laser_dps * dt);

                // Apply asteroid impulse.
                if (engine_has_component<AsteroidTag>(hit->entity))
                    engine_apply_impulse(hit->entity, ray_dir * constants::laser_impulse_per_second * dt);

                // Check for active shield on hit entity (Section 3.7.1).
                auto* target_sh = engine_try_get_component<Shield>(hit->entity);
                const auto* target_col = engine_try_get_component<Collider>(hit->entity);
                const auto* target_t  = engine_try_get_component<Transform>(hit->entity);

                glm::vec3 hit_point;
                bool shielded = false;

                if (target_sh && target_sh->current > 0.f && target_col && target_t) {
                    float shield_r = target_col->half_extents.x * constants::shield_sphere_scale;
                    float t = sphere_intersect_t(muzzle_origin, ray_dir, target_t->position, shield_r);
                    if (t > 0.0f) {
                        hit_point = muzzle_origin + ray_dir * t;
                        shielded  = true;
                    } else {
                        hit_point = muzzle_origin + ray_dir * hit->distance;
                    }
                } else {
                    hit_point = muzzle_origin + ray_dir * hit->distance;
                }

                lb.end = hit_point;

                // Spawn impact VFX on new target contact.
                if (hit->entity != lb.last_hit_entity) {
                    if (shielded) {
                        spawn_shield_impact(hit_point);
                    } else {
                        spawn_laser_impact(hit_point);
                    }
                    lb.last_hit_entity = hit->entity;
                }
            } else {
                // No hit — extend to max range.
                lb.end = muzzle_origin + ray_dir * constants::laser_max_range;
                lb.last_hit_entity = entt::null;
            }

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

static void laser_charging(entt::entity player_e, WeaponState& ws) {
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

        // Raycast from muzzle along cursor direction.
        glm::vec3 ray_origin, ray_dir;
        camera_rig_cursor_ray(ray_origin, ray_dir);
        (void)ray_origin;

        glm::vec3 beam_end = muzzle_origin + ray_dir * constants::laser_max_range;
        const auto hit = engine_raycast(muzzle_origin, ray_dir, constants::laser_max_range);
        if (hit.has_value() && hit->entity != player_e) {
            beam_end = muzzle_origin + ray_dir * hit->distance;
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

    // Skip the registry scan entirely when there is no relevant input.
    if (!switch_plasma && !switch_laser && !fire_pressed) {
        // Still need to run laser_update/laser_charging even without new input.
    }

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
