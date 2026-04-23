#pragma once

// Spawns all asteroids with random positions, velocities, and angular velocities.
// Call once from game_init before the first frame.
void asteroid_field_init();

// Reflects out-of-bounds entities back into the containment sphere and caps speed.
// Implemented in T011 — stub until then.
void containment_update();
