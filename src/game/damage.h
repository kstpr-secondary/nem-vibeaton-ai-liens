#pragma once

#include <entt/entt.hpp>

// Apply `amount` damage to `target`: absorbed by Shield first, remainder
// goes to Health.  Records Shield.last_damage_time for regen gating.
// Returns true if the entity died (Health.current reached 0).
bool apply_damage(entt::entity target, float amount);

// Detect AABB overlaps between health-bearing entities and resolve kinetic-
// energy collision damage.  Call once per frame from game_tick after engine_tick.
// T020 extends this function to also handle weapon hits.
void damage_resolve();
