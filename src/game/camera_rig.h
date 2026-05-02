#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

void camera_rig_init(entt::entity camera_entity);

// Call from game_tick step 2: reads mouse input, updates rig_rotation,
// writes t.rotation = rig_rotation * roll_quat. Does NOT move the camera yet.
void camera_rig_input(float dt);

// Call from game_tick step 13: overwrites t.rotation (undoes physics bake),
// springs visual_bank and collision_roll, positions and orients the camera.
void camera_rig_finalize(float dt);

// Returns the world-space aim point (camera look-target) for the current frame.
// Used by weapons to fire toward the crosshair, not along the ship nose.
// Returns player_pos + rig_fwd * cam_look_ahead if called before finalize() runs.
glm::vec3 camera_rig_aim_point();
