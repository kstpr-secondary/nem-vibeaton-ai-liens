#pragma once

// Internal init — called by engine_init()
void time_init();

// Internal dt update — called by engine_tick()
void time_set_delta(float dt);

// Public API (also declared in engine.h)
double engine_now();        // seconds since engine_init()
float  engine_delta_time(); // last frame dt in seconds
