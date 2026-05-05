#pragma once

#include <glm/glm.hpp>

void vfx_update(float dt);

void spawn_plasma_impact(const glm::vec3& position);
void spawn_laser_impact(const glm::vec3& position);
void spawn_shield_impact(const glm::vec3& position);
