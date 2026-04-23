#pragma once

// Called by engine_init() — registers the event callback with the renderer.
void input_init();

// Called at the start of engine_tick() — resets per-frame mouse deltas.
void input_begin_frame();
