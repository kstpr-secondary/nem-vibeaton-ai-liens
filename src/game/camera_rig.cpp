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

void camera_rig_update(float dt, glm::vec3 clean_forward) {
    if (s_camera == entt::null)
        return;

    // Find the player entity.
    auto& reg = engine_registry();
    auto  player_view = reg.view<PlayerTag, Transform>();
    if (player_view.begin() == player_view.end())
        return;

    const auto& pt       = player_view.get<Transform>(*player_view.begin());
    const glm::vec3 player_pos = pt.position;

    // Desired position: offset behind and above the ship.
    // clean_forward is roll-stripped so the camera offset stays stable
    // even when the ship has been knocked sideways by collisions.
    const glm::vec3 desired_pos = player_pos
        - clean_forward * constants::cam_offset_back
        + glm::vec3(0.f, constants::cam_offset_up, 0.f);

    // Smooth-follow via lerp — clamped so large dt never overshoots.
    auto& cam_t = engine_get_component<Transform>(s_camera);
    const float alpha = glm::clamp(constants::cam_lag_factor * dt, 0.f, 1.f);
    cam_t.position = glm::mix(cam_t.position, desired_pos, alpha);

    // Aim camera at player.
    // Player rotation is guaranteed roll-free (stripped in game.cpp),
    // so world-up (0,1,0) produces a stable camera — cockpit always
    // points up in screen space.  Bank rotation around the forward
    // axis can be added later for turn feedback.
    const glm::vec3 to_player = player_pos - cam_t.position;
    if (glm::dot(to_player, to_player) > 1e-6f) {
        const glm::mat4 view = glm::lookAt(
            cam_t.position, player_pos, glm::vec3(0.f, 1.f, 0.f));
        cam_t.rotation = glm::quat_cast(glm::transpose(glm::mat3(view)));
    }

    engine_set_active_camera(s_camera);
}
