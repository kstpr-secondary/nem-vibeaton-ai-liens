#pragma once

// Per-frame projectile update: expire projectiles whose lifetime has elapsed
// and mark them DestroyPending.
// Collision-triggered despawn is applied by damage_resolve (T020) when a
// projectile hits a target.
void projectile_update(float dt);
