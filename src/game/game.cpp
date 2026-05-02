#include "game.h"
#include "asteroid_field.h"
#include "camera_rig.h"
#include "components.h"
#include "constants.h"
#include "damage.h"
#include "enemy_ai.h"
#include "hud.h"
#include "player.h"
#include "projectile.h"
#include "spawn.h"
#include "weapons.h"
#include <engine.h>
#include <renderer.h>
#include <sokol_app.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// ---------------------------------------------------------------------------
// MatchState singleton
// ---------------------------------------------------------------------------

static MatchState s_match_state;

MatchState& get_match_state() {
    return s_match_state;
}



// Explosion VFX lifetime — how long the death sphere stays visible.
static constexpr float k_explosion_lifetime = 0.6f;

// ---------------------------------------------------------------------------
// Roll-stripping — remove physics-induced roll from player Transform.
//
// The engine's Euler integration can accumulate roll on the player when
// collision impulses apply torque (even though angular_velocity is zeroed
// each frame by player_update, the rotation quaternion already baked in
// the previous frame's integration).  This function strips roll so the
// camera offset and model display stay upright.
//
// Strategy: rebuild the rotation from an orthonormal frame built from
// forward (preserves yaw + pitch) and a level-up vector (strips roll).
// ---------------------------------------------------------------------------

static glm::vec3 strip_player_roll() {
    auto& reg = engine_registry();
    auto  view = reg.view<PlayerTag, Transform>();
    if (view.begin() == view.end())
        return glm::vec3(0.f, 1.f, 0.f);

    auto& t = view.get<Transform>(*view.begin());

    // Current forward direction — encodes both yaw and pitch.
    glm::vec3 fwd = t.rotation * glm::vec3(0.f, 1.f, 0.f);
    fwd = glm::normalize(fwd);

    // Project world-up onto the plane perpendicular to forward.
    // This gives a "level" up vector — perpendicular to forward, no roll.
    const glm::vec3 world_up(0.f, 1.f, 0.f);
    const float dot = glm::dot(world_up, fwd);

    if (glm::abs(dot) > 0.999f) {
        // Forward is nearly vertical — yaw undefined from forward alone.
        // Use the existing rotation's Y/W components to estimate yaw.
        const float yaw = 2.0f * std::atan2(t.rotation.y, t.rotation.w);
        t.rotation = glm::angleAxis(yaw, glm::vec3(0.f, 1.f, 0.f));
    } else {
        // Level up: remove the component parallel to forward.
        const glm::vec3 level_up = glm::normalize(world_up - fwd * dot);

        // Right is perpendicular to both forward and level-up.
        const glm::vec3 right = glm::normalize(glm::cross(fwd, level_up));

        // Recompose rotation from orthonormal frame.
        // Columns: right (model X), fwd (model Y = forward), level_up (model Z).
        // This preserves yaw (from forward's XZ direction) and pitch
        // (from forward's angle above/below horizontal), while removing roll.
        const glm::mat3 R(right, fwd, level_up);
        t.rotation = glm::normalize(glm::quat(R));
    }

    // Return roll-stripped forward for camera positioning.
    return t.rotation * glm::vec3(0.f, 1.f, 0.f);
}

// ---------------------------------------------------------------------------
// render_submit — called each frame from game_tick
// ---------------------------------------------------------------------------

static void render_submit() {
    auto& reg  = engine_registry();
    auto  view = reg.view<Transform, Mesh, EntityMaterial>();
    for (auto e : view) {
        const auto& t   = view.get<Transform>(e);
        const auto& m   = view.get<Mesh>(e);
        const auto& em  = view.get<EntityMaterial>(e);

        if (!renderer_handle_valid(m.handle))
            continue;

        const glm::mat4 world =
            glm::translate(glm::mat4(1.f), t.position)
            * glm::mat4_cast(t.rotation)
            * glm::scale(glm::mat4(1.f), t.scale);

        renderer_enqueue_draw(m.handle, glm::value_ptr(world), em.mat);
    }
}

// ---------------------------------------------------------------------------
// Enemy death handling — detect dead enemies, spawn explosion VFX, despawn
// ---------------------------------------------------------------------------

static void enemy_death_update() {
    auto& reg = engine_registry();
    auto view = reg.view<EnemyTag, Health, Transform>();

    for (auto it = view.begin(); it != view.end(); ) {
        auto e = *it;
        ++it;  // advance iterator before potential structural change

        const auto& hp = view.get<Health>(e);
        if (hp.current > 0.f)
            continue;

        // Spawn explosion VFX — short-lived orange sphere.
        const auto& t = view.get<Transform>(e);
        const float rgba[4] = {1.0f, 0.5f, 0.1f, 1.0f};
        const Material mat  = renderer_make_unlit_material(rgba);
        entt::entity explosion = engine_spawn_sphere(t.position, 3.0f, mat);

        // Attach Lifetime component — cleaned up by vfx_cleanup() pass.
        auto& lt = engine_add_component<Lifetime>(explosion);
        lt.spawn_time = engine_now();
        lt.lifetime   = k_explosion_lifetime;

        // Decrement enemy count and mark enemy for destruction.
        s_match_state.enemies_remaining--;
        reg.emplace<DestroyPending>(e);
    }
}

// ---------------------------------------------------------------------------
// VFX cleanup — expire short-lived entities (explosions, etc.)
// ---------------------------------------------------------------------------

static void vfx_cleanup() {
    auto& reg = engine_registry();
    auto view = reg.view<Lifetime>();

    for (auto e : view) {
        if (reg.all_of<DestroyPending>(e))
            continue;

        const auto& lt = view.get<Lifetime>(e);
        if (engine_now() - lt.spawn_time >= static_cast<double>(lt.lifetime))
            reg.emplace<DestroyPending>(e);
    }
}

// ---------------------------------------------------------------------------
// Scene reset — destroy all entities, clear game-layer statics
// ---------------------------------------------------------------------------

static void scene_reset() {
    auto& reg = engine_registry();

    // Mark every entity for destruction.  engine_tick will sweep them.
    for (auto e : reg.view<entt::entity>())
        reg.emplace<DestroyPending>(e);

    // Reset camera-rig static so it doesn't hold a dangling handle.
    camera_rig_init(entt::null);
}

// ---------------------------------------------------------------------------
// Restart / quit input handling
// ---------------------------------------------------------------------------

static void handle_restart_quit_input() {
    static bool s_prev_enter = false;
    static bool s_prev_esc   = false;

    const bool cur_enter = engine_key_down(SAPP_KEYCODE_ENTER);
    const bool cur_esc   = engine_key_down(SAPP_KEYCODE_ESCAPE);

    // Manual restart — Enter from any terminal phase, skips countdown.
    if (cur_enter && !s_prev_enter) {
        if (s_match_state.phase == MatchPhase::PlayerDead ||
            s_match_state.phase == MatchPhase::Victory ||
            s_match_state.phase == MatchPhase::Restarting) {
            s_match_state.phase           = MatchPhase::Restarting;
            s_match_state.phase_enter_time = engine_now();
        }
    }

    // Quit — Esc edge-triggered.
    if (cur_esc && !s_prev_esc)
        sapp_request_quit();

    s_prev_enter = cur_enter;
    s_prev_esc   = cur_esc;
}

// ---------------------------------------------------------------------------
// Match state machine — phase transitions, win/loss, auto-restart countdown
// ---------------------------------------------------------------------------

static void match_state_update(float /*dt*/) {
    // Restarting → Playing: destroy all entities, rebuild scene.
    if (s_match_state.phase == MatchPhase::Restarting) {
        scene_reset();
        game_init();
        return;
    }

    if (s_match_state.phase == MatchPhase::Playing) {
        // Victory check first — spec: simultaneous player+enemy death → player wins.
        if (s_match_state.enemies_remaining <= 0) {
            s_match_state.phase           = MatchPhase::Victory;
            s_match_state.phase_enter_time = engine_now();
            s_match_state.auto_restart_delay = constants::restart_delay_win;
            return;
        }

        // Player death check.
        auto player_view = engine_registry().view<PlayerTag, Health>();
        for (auto e : player_view) {
            const auto& hp = player_view.get<Health>(e);
            if (hp.current <= 0.f) {
                s_match_state.phase           = MatchPhase::PlayerDead;
                s_match_state.phase_enter_time = engine_now();
                s_match_state.auto_restart_delay = constants::restart_delay_death;
                return;
            }
        }
    } else if (s_match_state.phase == MatchPhase::PlayerDead ||
               s_match_state.phase == MatchPhase::Victory) {
        // Auto-restart countdown.
        const double elapsed = engine_now() - s_match_state.phase_enter_time;
        if (elapsed >= static_cast<double>(s_match_state.auto_restart_delay)) {
            s_match_state.phase           = MatchPhase::Restarting;
            s_match_state.phase_enter_time = engine_now();
        }
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void game_init() {
    // Reset match state.
    s_match_state                = {};
    s_match_state.phase          = MatchPhase::Playing;
    s_match_state.phase_enter_time = engine_now();

    // Directional light entity — warm sun from upper-left at ~60° elevation.
    // The engine iterates all Light entities each tick and pushes them to
    // the renderer via renderer_set_directional_light.
    auto light_e = engine_create_entity();
    auto& l      = engine_add_component<Light>(light_e);
    l.direction  = {0.366f, 0.695f, 0.474f};
    l.color      = {1.0f, 0.95f, 0.9f};
    l.intensity  = 1.0f;

    // Player ship at field centre.
    spawn_player(glm::vec3(0.f));

    // Enemy ship — spawn at offset from player.
    spawn_enemy(glm::vec3(50.f, 10.f, -50.f));
    s_match_state.enemies_remaining = 1;

    // 200 asteroids placed randomly throughout the field.
    asteroid_field_init();

    // Camera entity — positioned directly behind/above the player.
    entt::entity cam_e = engine_create_entity();
    auto& cam_t        = engine_add_component<Transform>(cam_e);
    cam_t.position     = glm::vec3(0.f,
                                   constants::cam_offset_up,
                                   constants::cam_offset_back);
    engine_add_component<Camera>(cam_e);

    camera_rig_init(cam_e);
}

void game_tick(float dt) {
    player_update(dt);          // 1. flight controls — mouse look, velocity align, thrust
    engine_tick(dt);            // 2. physics, collision, entity cleanup
    containment_update();       // 3. boundary reflection + speed cap
    enemy_ai_update(dt);        // 4. seek player, fire plasma on cooldown
    weapon_update(dt);          // 5. fire processing, cooldown advancement
    projectile_update(dt);      // 6. lifetime check, despawn expired
    damage_resolve();           // 7. collision + weapon damage → shield/HP
    enemy_death_update();       // T021: detect dead enemies, spawn explosion
    vfx_cleanup();              // expire short-lived VFX entities
    match_state_update(dt);     // T024: phase transitions, win/loss, auto-restart
    handle_restart_quit_input(); // T026: manual restart (Enter) and quit (Esc)

    // Strip roll from player Transform — physics integration can bake in
    // unwanted rotation from collision impulses.  Returns the clean forward
    // vector for camera positioning.
    const glm::vec3 clean_forward = strip_player_roll();

    camera_rig_update(dt, clean_forward);   // 9. follow player with offset + lag
    render_submit();            // 10. enqueue_draw for all visible entities
    hud_render();               // 11. ImGui widgets + overlays
}

void game_shutdown() {
    // entity cleanup is handled by engine_shutdown (called in main after this)
}
