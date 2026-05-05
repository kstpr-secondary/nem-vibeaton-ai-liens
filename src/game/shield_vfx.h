#pragma once

#include <glm/glm.hpp>

// FS param struct — must match layout in shaders/game/shield.glsl
struct ShieldFSParams {
    glm::vec4 shield_color;     // .rgb = tint, .a = base opacity scale
    glm::vec4 view_pos_w;       // .xyz = camera world position
    glm::vec4 fresnel_params;   // .x = exponent, .y = rim intensity
};

// Update shield sphere visuals each frame: sync transform to owner,
// update alpha based on shield health, hide when shield is depleted.
void shield_vfx_update(float dt);
