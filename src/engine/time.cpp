#include <ctime>  // ensures struct timespec / CLOCK_MONOTONIC visible before sokol

#define SOKOL_TIME_IMPL
#include <sokol_time.h>

#include "engine_time.h"

static uint64_t g_start_tick = 0;
static float    g_last_dt    = 0.f;

void time_init() {
    stm_setup();
    g_start_tick = stm_now();
    g_last_dt    = 0.f;
}

void time_set_delta(float dt) {
    g_last_dt = dt;
}

double engine_now() {
    return stm_sec(stm_since(g_start_tick));
}

float engine_delta_time() {
    return g_last_dt;
}
