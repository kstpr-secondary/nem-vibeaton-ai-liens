#include "damage.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/glm.hpp>
#include <algorithm>

// ---------------------------------------------------------------------------
// apply_damage
// ---------------------------------------------------------------------------

bool apply_damage(entt::entity target, float amount) {
    if (amount <= 0.f)
        return false;

    auto* hp = engine_try_get_component<Health>(target);
    if (!hp)
        return false;

    // Shield absorbs damage first.
    auto* sh = engine_try_get_component<Shield>(target);
    if (sh && sh->current > 0.f) {
        const float absorbed = std::min(sh->current, amount);
        sh->current         -= absorbed;
        sh->last_damage_time = engine_now();
        amount              -= absorbed;
    }

    // Remaining damage goes to HP.
    if (amount > 0.f)
        hp->current = std::max(0.f, hp->current - amount);

    return hp->current <= 0.f;
}

// ---------------------------------------------------------------------------
// AABB overlap helper
// ---------------------------------------------------------------------------

static bool aabb_overlap(const glm::vec3& pos_a, const glm::vec3& half_a,
                          const glm::vec3& pos_b, const glm::vec3& half_b) {
    const glm::vec3 dist = glm::abs(pos_a - pos_b);
    const glm::vec3 sum  = half_a + half_b;
    return dist.x <= sum.x && dist.y <= sum.y && dist.z <= sum.z;
}

// ---------------------------------------------------------------------------
// damage_resolve
// ---------------------------------------------------------------------------

void damage_resolve() {
    auto& reg = engine_registry();

    // --- Kinetic energy damage: Player ↔ Asteroid ----------------------------
    //
    // The engine's physics resolves collision impulses inside engine_tick.
    // We detect overlap + approaching velocity here to compute the KE damage
    // that corresponds to the contact.  The approaching guard (v_rel · sep > 0)
    // ensures we fire once per contact: the next frame the bodies are separating
    // so the condition fails naturally.

    auto player_view   = reg.view<PlayerTag,   Transform, RigidBody, Collider, Health, CameraRigState>();
    auto asteroid_view = reg.view<AsteroidTag, Transform, RigidBody, Collider>();

    for (auto pe : player_view) {
        const auto& pt  = player_view.get<Transform>(pe);
        const auto& prb = player_view.get<RigidBody>(pe);
        const auto& pc  = player_view.get<Collider>(pe);
        auto&       crs  = player_view.get<CameraRigState>(pe);

        for (auto ae : asteroid_view) {
            const auto& at  = asteroid_view.get<Transform>(ae);
            const auto& arb = asteroid_view.get<RigidBody>(ae);
            const auto& ac  = asteroid_view.get<Collider>(ae);

            if (!aabb_overlap(pt.position, pc.half_extents,
                              at.position, ac.half_extents))
                continue;

            // Only apply damage on approach — prevents repeated hits while
            // the physics engine holds the bodies in contact for one tick.
            const glm::vec3 sep     = at.position - pt.position;
            const float     sep_len = glm::length(sep);
            if (sep_len < 1e-6f)
                continue;

            const glm::vec3 sep_dir      = sep / sep_len;
            const glm::vec3 v_rel        = prb.linear_velocity - arb.linear_velocity;
            const float     approach_spd = glm::dot(v_rel, sep_dir);
            if (approach_spd <= 0.f)
                continue;  // separating — damage already applied this contact

            // Reduced-mass kinetic energy of the approaching component.
            const float m_r  = (prb.mass * arb.mass) / (prb.mass + arb.mass);
            const float ke   = 0.5f * m_r * approach_spd * approach_spd;
            const float dmg  = ke * constants::kinetic_damage_scale;

            apply_damage(pe, dmg);

            // Feed collision visual roll to camera rig.
            glm::vec3 rig_right = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f);
            float roll_sign     = glm::dot(sep_dir, rig_right);
            crs.collision_roll_impulse += roll_sign * ke;
        }
    }

    // --- Projectile ↔ collidable-entity hits ---------------------------------
    //
    // Iterate active projectiles and check them against every entity that has a
    // Collider (asteroids, player, enemies).  On first overlap:
    //   • apply ProjectileData.damage if the target has Health
    //   • mark the projectile DestroyPending (despawns after engine_tick sweep)
    // The owner is excluded so the projectile never hits the entity that fired it.
    // Other projectiles are skipped to avoid projectile-vs-projectile collisions.
    //
    // Laser raycast damage is applied directly inside laser_fire (T018) via
    // apply_damage — no additional work needed here for that path.

    auto proj_view = reg.view<ProjectileTag, ProjectileData, Transform, Collider>();
    auto target_view = reg.view<Transform, Collider>();

    for (auto pe : proj_view) {
        if (reg.all_of<DestroyPending>(pe))
            continue;  // already handled this tick

        const auto& pd = proj_view.get<ProjectileData>(pe);
        const auto& pt = proj_view.get<Transform>(pe);
        const auto& pc = proj_view.get<Collider>(pe);

        for (auto te : target_view) {
            if (te == pe)                          continue;  // self
            if (te == pd.owner)                    continue;  // firing entity
            if (reg.all_of<ProjectileTag>(te))     continue;  // other projectile

            const auto& tt = target_view.get<Transform>(te);
            const auto& tc = target_view.get<Collider>(te);

            if (!aabb_overlap(pt.position, pc.half_extents,
                              tt.position, tc.half_extents))
                continue;

            // Apply damage only to entities that have health.
            auto* hp = engine_try_get_component<Health>(te);
            if (hp)
                apply_damage(te, pd.damage);

            // Projectile is consumed on first hit regardless of target type.
            reg.emplace<DestroyPending>(pe);
            break;
        }
    }
}
