#include "game_shaders.h"

#include <sokol_gfx.h>

// Generated sokol-shdc headers — already on the include path for the game target.
#include "shaders/shield.glsl.h"
#include "shaders/plasma_ball.glsl.h"

RendererShaderHandle g_shield_shader = {};
RendererShaderHandle g_plasma_shader = {};

void game_shaders_init() {
    g_shield_shader  = renderer_create_shader(shield_shader_desc(sg_query_backend()));
    g_plasma_shader  = renderer_create_shader(plasma_ball_shader_desc(sg_query_backend()));
}
