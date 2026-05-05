#pragma once

#include <renderer.h>

extern RendererShaderHandle g_shield_shader;
extern RendererShaderHandle g_plasma_shader;

void game_shaders_init();  // must be called from game_init(), after renderer_init()
