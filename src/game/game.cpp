#include "game.h"
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
    // T010: spawn player, 200 asteroids, camera entity, set directional light
}

void game_tick(float dt) {
    engine_tick(dt);
    // T014: containment_update, player_update, camera_rig_update, damage_resolve
    // T022: weapon_update, projectile_update, enemy_ai_update
    // T027: match_state_update, hud_render
    render_submit();
}

void game_shutdown() {
    // entity cleanup is handled by engine_shutdown (called in main after this)
}
