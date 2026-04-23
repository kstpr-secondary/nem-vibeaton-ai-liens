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

    auto player_view   = reg.view<PlayerTag,   Transform, RigidBody, Collider, Health>();
    auto asteroid_view = reg.view<AsteroidTag, Transform, RigidBody, Collider>();

    for (auto pe : player_view) {
        const auto& pt  = player_view.get<Transform>(pe);
        const auto& prb = player_view.get<RigidBody>(pe);
        const auto& pc  = player_view.get<Collider>(pe);

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
        }
    }

    // T020: projectile ↔ entity hits and laser raycast damage added in Phase 4.
}
