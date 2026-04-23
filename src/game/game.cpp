#include "game.h"
#include <engine.h>

void game_init() {
    // Phase 2 (T010): spawn player, asteroids, camera, directional light
}

void game_tick(float dt) {
    engine_tick(dt);
    // Phase 3 (T014): player_update, camera_rig_update, containment_update, damage_resolve, render_submit
    // Phase 4 (T022): weapon_update, projectile_update, enemy_ai_update
    // Phase 5 (T027): match_state_update, hud_render
}

void game_shutdown() {
    // nothing to tear down beyond engine/renderer (handled in main)
}
