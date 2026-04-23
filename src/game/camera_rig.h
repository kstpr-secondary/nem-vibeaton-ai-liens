#pragma once

#include <entt/entt.hpp>

// Call from game_init after the camera entity is created.
void camera_rig_init(entt::entity camera_entity);

// Call each frame from game_tick after player_update.
void camera_rig_update(float dt);
