#include "game.h"
#include "asteroid_field.h"
#include "camera_rig.h"
#include "constants.h"
#include "damage.h"
#include "player.h"
#include "spawn.h"
#include <engine.h>
#include <renderer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Directional light — configured at init, submitted every frame in render_submit.
// T010 may overwrite direction/color before renderer_run starts.
static DirectionalLight s_light = {
    {-0.4f, -0.8f, -0.4f},  // direction (normalised in world space)
    {1.0f,  0.95f, 0.9f },  // warm white
    1.0f                     // intensity
};

// ---------------------------------------------------------------------------
// render_submit — called each frame from game_tick
// ---------------------------------------------------------------------------

static void render_submit() {
    renderer_set_directional_light(s_light);

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
// Lifecycle
// ---------------------------------------------------------------------------

void game_init() {
    // Directional light — warm sun from upper-left.
    s_light = {{-0.4f, -0.8f, -0.4f}, {1.0f, 0.95f, 0.9f}, 1.0f};

    // Player ship at field centre.
    spawn_player(glm::vec3(0.f));

    // 200 asteroids placed randomly throughout the field.
    asteroid_field_init();

    // Camera entity — positioned directly behind/above the player so the
    // camera_rig lerp starts from a sensible state rather than the origin.
    entt::entity cam_e = engine_create_entity();
    auto& cam_t        = engine_add_component<Transform>(cam_e);
    cam_t.position     = glm::vec3(0.f,
                                   constants::cam_offset_up,
                                   constants::cam_offset_back);
    engine_add_component<Camera>(cam_e);

    camera_rig_init(cam_e);

    // T021: spawn_enemy() added here when Phase 4 begins.
}

void game_tick(float dt) {
    engine_tick(dt);          // physics, collision resolution, entity cleanup

    containment_update();     // reflect out-of-bounds entities, cap asteroid speed
    player_update(dt);        // flight controls, boost activation, shield/boost regen
    // T022: enemy_ai_update(dt)
    // T022: weapon_update(dt)
    // T022: projectile_update(dt)
    damage_resolve();         // kinetic-energy collision damage → shield/HP cascade
    // T027: match_state_update(dt)
    camera_rig_update(dt);    // smooth-follow camera behind player
    render_submit();          // enqueue draw calls for all visible entities
    // T027: hud_render()
}

void game_shutdown() {
    // entity cleanup is handled by engine_shutdown (called in main after this)
}
