#include "camera_rig.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// ── Static file-scope state ──────────────────────────────────────────────────

static entt::entity s_camera       = entt::null;
static entt::entity s_player       = entt::null;
static glm::vec3    s_cam_position = {0.f, 0.f, 0.f};
static glm::vec3    s_aim_point    = {0.f, 0.f, 0.f};

// ── camera_rig_init ──────────────────────────────────────────────────────────

void camera_rig_init(entt::entity camera_entity) {
    s_camera = camera_entity;

    auto& reg     = engine_registry();
    auto  player_view = reg.view<PlayerTag, Transform, CameraRigState>();
    if (player_view.begin() == player_view.end())
        return;

    s_player = *player_view.begin();
    auto& crs  = player_view.get<CameraRigState>(s_player);
    auto& pt   = player_view.get<Transform>(s_player);

    // Copy initial orientation from the player's Transform.
    crs.rig_rotation = pt.rotation;
    s_cam_position   = pt.position;

    engine_set_active_camera(s_camera);
}

// ── camera_rig_input (step 2 of game_tick) ───────────────────────────────────

void camera_rig_input(float dt) {
    if (s_player == entt::null)
        return;

    auto& reg     = engine_registry();
    auto  view    = reg.view<PlayerTag, Transform, CameraRigState>();
    if (view.begin() == view.end())
        return;

    auto& t   = view.get<Transform>(s_player);
    auto& crs = view.get<CameraRigState>(s_player);

    // Steering: mouse delta drives yaw and pitch rates when LMB held.
    if (engine_mouse_button(0)) {
        glm::vec2 delta = engine_mouse_delta();

        // Yaw: rotate rig around rig_up axis (model -Z = cockpit-up).
        glm::vec3 rig_up = crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f);
        glm::quat q_yaw = glm::angleAxis(-delta.x * constants::cam_turn_rate * dt, rig_up);

        // Pitch: rotate rig around rig_right axis (model -X = right).
        glm::vec3 rig_right = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f);
        glm::quat q_pitch = glm::angleAxis(-delta.y * constants::cam_turn_rate * dt, rig_right);

        // Apply yaw first, then pitch; both in world space (pre-multiply).
        crs.rig_rotation = glm::normalize(q_yaw * q_pitch * crs.rig_rotation);
    }

    // Write ship Transform — no roll yet, just rig orientation.
    t.rotation = crs.rig_rotation;
}

// ── camera_rig_finalize (step 13 of game_tick) ───────────────────────────────

void camera_rig_finalize(float dt) {
    if (s_player == entt::null || s_camera == entt::null)
        return;

    auto& reg     = engine_registry();
    auto  view    = reg.view<PlayerTag, Transform, CameraRigState>();
    if (view.begin() == view.end())
        return;

    auto& t   = view.get<Transform>(s_player);
    auto& crs = view.get<CameraRigState>(s_player);

    // ── Step A: spring visual_bank toward steering-based target ──────────
    float bank_target = 0.f;
    if (engine_mouse_button(0)) {
        bank_target = -engine_mouse_delta().x * constants::visual_bank_gain;
        bank_target = glm::clamp(bank_target,
                                 -constants::visual_bank_max,
                                 constants::visual_bank_max);
    }
    crs.visual_bank += (bank_target - crs.visual_bank)
                       * constants::visual_bank_spring * dt;

    // ── Step B: spring collision_roll toward 0 (damped spring) ───────────
    // Accumulate any new impulse from damage_resolve.
    if (crs.collision_roll_impulse != 0.f) {
        float sign = (crs.collision_roll_impulse > 0.f) ? 1.f : -1.f;
        float mag  = glm::min(
            glm::abs(crs.collision_roll_impulse) * constants::collision_roll_gain,
            constants::collision_roll_max);
        crs.collision_roll += sign * mag;
        crs.collision_roll = glm::clamp(
            crs.collision_roll,
            -constants::collision_roll_max,
            constants::collision_roll_max);
        crs.collision_roll_impulse = 0.f;
    }

    // Spring decay: acceleration = -spring * position - damping * velocity.
    const float damping = 2.f * glm::sqrt(constants::collision_roll_spring);
    crs.collision_roll_vel += (-constants::collision_roll_spring * crs.collision_roll
                               - damping * crs.collision_roll_vel) * dt;
    crs.collision_roll += crs.collision_roll_vel * dt;

    // ── Step C: overwrite t.rotation (undoes any physics-baked rotation) ─
    float total_roll = crs.visual_bank + crs.collision_roll;
    glm::quat roll_quat = glm::angleAxis(total_roll, glm::vec3(0.f, 1.f, 0.f));
    t.rotation = crs.rig_rotation * roll_quat;

    // ── Step D: compute camera position (smoothed) ───────────────────────
    glm::vec3 rig_fwd = crs.rig_rotation * glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 rig_up  = crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f);

    glm::vec3 desired_cam_pos = t.position
        - rig_fwd * constants::cam_offset_back
        + rig_up  * constants::cam_offset_up;

    float alpha = glm::clamp(constants::cam_lag_factor * dt, 0.f, 1.f);
    s_cam_position = glm::mix(s_cam_position, desired_cam_pos, alpha);

    // ── Step E: position camera; aim toward look-target above and ahead ──
    glm::vec3 look_target = t.position
        + rig_fwd * constants::cam_look_ahead
        + rig_up  * constants::cam_look_up_bias;

    s_aim_point = look_target;

    auto& cam_t = engine_get_component<Transform>(s_camera);
    cam_t.position = s_cam_position;

    glm::vec3 to_target = look_target - s_cam_position;
    if (glm::dot(to_target, to_target) > 1e-6f) {
        // Build view matrix with rig_up as camera up — no gimbal-lock risk
        // because rig_up is derived from rig_rotation, not from world-Y.
        glm::vec3 cam_up = rig_up;
        if (glm::abs(glm::dot(glm::normalize(to_target), rig_up)) > 0.99f) {
            cam_up = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f); // rig_right fallback
        }
        glm::mat4 view = glm::lookAt(s_cam_position, look_target, cam_up);
        cam_t.rotation = glm::quat_cast(glm::transpose(glm::mat3(view)));
    }

    engine_set_active_camera(s_camera);
}

// ── camera_rig_aim_point ─────────────────────────────────────────────────────

glm::vec3 camera_rig_aim_point() {
    return s_aim_point;
}
