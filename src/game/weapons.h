#pragma once

#include "components.h"

// Returns true if the active weapon's cooldown has elapsed.
bool weapon_ready(const WeaponState& ws);

// Per-frame update: edge-triggered weapon switch (Q → Plasma, E → Laser).
// Laser fire logic added in T018; plasma fire logic added in T019.
void weapon_update(float dt);
