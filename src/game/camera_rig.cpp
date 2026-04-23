#include "camera_rig.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

static entt::entity s_camera = entt::null;

void camera_rig_init(entt::entity camera_entity) {
    s_camera = camera_entity;
    engine_set_active_camera(s_camera);
}

void camera_rig_update(float dt) {
    if (s_camera == entt::null)
        return;

    // Find the player entity.
    auto& reg = engine_registry();
    auto  player_view = reg.view<PlayerTag, Transform>();
    if (player_view.begin() == player_view.end())
        return;

    const auto& pt       = player_view.get<Transform>(*player_view.begin());
    const glm::vec3 player_pos = pt.position;

    // Desired position: offset behind (+Z local) and above (+Y local) the ship.
    const glm::vec3 local_offset = pt.rotation
        * glm::vec3(0.f, constants::cam_offset_up, constants::cam_offset_back);
    const glm::vec3 desired_pos = player_pos + local_offset;

    // Smooth-follow via lerp — clamped so large dt never overshoots.
    auto& cam_t = engine_get_component<Transform>(s_camera);
    const float alpha = glm::clamp(constants::cam_lag_factor * dt, 0.f, 1.f);
    cam_t.position = glm::mix(cam_t.position, desired_pos, alpha);

    // Aim camera at player.  glm::lookAt gives a world-to-camera view matrix;
    // its 3×3 rotation block transposed is the camera-to-world rotation we want.
    const glm::vec3 to_player = player_pos - cam_t.position;
    if (glm::dot(to_player, to_player) > 1e-6f) {
        const glm::mat4 view = glm::lookAt(
            cam_t.position, player_pos, glm::vec3(0.f, 1.f, 0.f));
        cam_t.rotation = glm::quat_cast(glm::transpose(glm::mat3(view)));
    }

    engine_set_active_camera(s_camera);
}
