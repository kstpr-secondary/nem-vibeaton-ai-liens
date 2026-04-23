#pragma once

#include "components.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

// Match state singleton — tracks phase, enemy count, restart timers.
struct MatchState {
    MatchPhase phase              = MatchPhase::Playing;
    double     phase_enter_time   = 0.0;
    float      auto_restart_delay = 2.0f;
    int        enemies_remaining  = 0;
};

MatchState& get_match_state();

void game_init();
void game_tick(float dt);
void game_shutdown();
