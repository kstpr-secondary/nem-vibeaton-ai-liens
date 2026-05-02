#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

// Call from game_init after the camera entity is created.
void camera_rig_init(entt::entity camera_entity);

// Call each frame from game_tick after player_update.
// clean_forward — roll-stripped forward vector from game.cpp (avoids reading
// player Transform.rotation directly which may have physics-baked roll).
void camera_rig_update(float dt, glm::vec3 clean_forward);
