#pragma once

#include <glm/glm.hpp>

// FS param struct — must match layout in shaders/game/plasma_ball.glsl
struct PlasmaBallFSParams {
    glm::vec4 core_color;     // .rgb = warm white (1.0, 1.0, 0.9)
    glm::vec4 rim_color;      // .rgb = blue-white (0.4, 0.6, 1.0)
    glm::vec4 bolt_color;     // .rgb = per-instance tint
    glm::vec4 params;         // .x=time, .y=fresnel_exp(3.0), .z=core_thresh(0.65), .w=opacity(1.0)
    glm::vec4 view_pos_w;     // .xyz = camera world position
};

void vfx_update(float dt);

void spawn_plasma_impact(const glm::vec3& position);
void spawn_laser_impact(const glm::vec3& position);
void spawn_shield_impact(const glm::vec3& position);
