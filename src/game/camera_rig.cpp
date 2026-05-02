#include "camera_rig.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

static entt::entity s_camera = entt::null;

// Camera bank angle — temporary roll added during mouse-driven turns.
// Smoothly decays back to zero when the player releases the left mouse button.
static float   s_camera_bank   = 0.0f;
static constexpr float k_bank_max     = 0.26f;    // ~15 degrees max bank
static constexpr float k_bank_decay   = 1.5f;     // exponential decay rate
static constexpr float k_bank_sensitivity = 0.003f; // radians per mouse pixel

void camera_rig_init(entt::entity camera_entity) {
    s_camera = camera_entity;
    engine_set_active_camera(s_camera);
}

void camera_rig_update(float dt) {
    if (s_camera == entt::null)
        return;

    // Update camera bank — accumulate during mouse drag, decay when released.
    const bool mouse_held = engine_mouse_button(0);
    if (mouse_held) {
        const glm::vec2 delta = engine_mouse_delta();
        s_camera_bank += delta.x * k_bank_sensitivity;
        s_camera_bank = glm::clamp(s_camera_bank, -k_bank_max, k_bank_max);
    } else {
        // Exponential decay back to zero.
        const float factor = 1.0f - std::exp(-k_bank_decay * dt);
        s_camera_bank *= (1.0f - factor);
    }

    // Find the player entity.
    auto& reg = engine_registry();
    auto  player_view = reg.view<PlayerTag, Transform>();
    if (player_view.begin() == player_view.end())
        return;

    const auto& pt       = player_view.get<Transform>(*player_view.begin());
    const glm::vec3 player_pos = pt.position;

    // Desired position: offset behind and above the ship.
    // Player rotation is roll-free by design (extracted in player_update),
    // so forward is a stable direction for camera positioning.
    const glm::vec3 forward = glm::normalize(glm::vec3(
        pt.rotation * glm::vec3(0.f, 1.f, 0.f)));

    const glm::vec3 desired_pos = player_pos
        - forward * constants::cam_offset_back
        + glm::vec3(0.f, constants::cam_offset_up, 0.f);

    // Smooth-follow via lerp — clamped so large dt never overshoots.
    auto& cam_t = engine_get_component<Transform>(s_camera);
    const float alpha = glm::clamp(constants::cam_lag_factor * dt, 0.f, 1.f);
    cam_t.position = glm::mix(cam_t.position, desired_pos, alpha);

    // Aim camera at player with optional bank rotation.
    // Player rotation is roll-free, so world-up (0,1,0) produces a stable
    // camera orientation — the cockpit always points up in screen space.
    // Bank rotation adds temporary roll around the camera-to-player axis
    // to highlight turning motion during mouse gestures.
    const glm::vec3 to_player = player_pos - cam_t.position;
    const float dist = glm::length(to_player);
    if (dist > 1e-6f) {
        const glm::vec3 forward_cam = to_player / dist;

        // Base up vector — world-up gives a stable, roll-free camera.
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

        // Apply bank rotation around the camera's forward axis.
        if (std::abs(s_camera_bank) > 1e-6f) {
            const glm::quat bank_quat = glm::angleAxis(s_camera_bank, forward_cam);
            up = glm::normalize(bank_quat * up);
        }

        const glm::mat4 view = glm::lookAt(
            cam_t.position, player_pos, up);
        cam_t.rotation = glm::quat_cast(glm::transpose(glm::mat3(view)));
    }

    engine_set_active_camera(s_camera);
}
