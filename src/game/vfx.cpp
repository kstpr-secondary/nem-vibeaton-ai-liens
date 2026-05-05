#include "vfx.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <renderer.h>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// vfx_update — animate all active VFX entities (scale + alpha)
// ---------------------------------------------------------------------------

void vfx_update(float /*dt*/) {
    auto& reg  = engine_registry();
    auto  view = reg.view<Lifetime, VFXData, Transform, EntityMaterial>();

    for (auto e : view) {
        const auto& lt = view.get<Lifetime>(e);
        const auto& vd = view.get<VFXData>(e);
        auto&       tr = view.get<Transform>(e);
        auto&       em = view.get<EntityMaterial>(e);

        double age = engine_now() - lt.spawn_time;
        float  t   = age / static_cast<double>(lt.lifetime);
        if (t > 1.0f) t = 1.0f;

        float sc = vd.initial_scale + (vd.final_scale - vd.initial_scale) * t;
        tr.scale = glm::vec3(sc);

        float alpha = vd.initial_alpha + (vd.final_alpha - vd.initial_alpha) * t;

        auto* p = material_uniforms_as<UnlitFSParams>(em.mat);
        p->color = {vd.r, vd.g, vd.b, alpha};
    }
}

// ---------------------------------------------------------------------------
// Spawn helpers
// ---------------------------------------------------------------------------

void spawn_plasma_impact(const glm::vec3& position) {
    static const float s_rgba[4] = {1.0f, 0.5f, 0.1f, 0.9f};
    auto  e    = engine_spawn_sphere(position, 1.0f,
                          renderer_make_unlit_material(s_rgba));

    Material& mat = engine_get_component<EntityMaterial>(e).mat;
    mat.pipeline.blend       = BlendMode::AlphaBlend;
    mat.pipeline.depth_write = false;
    mat.pipeline.render_queue = 2;

    auto& lt = engine_add_component<Lifetime>(e);
    lt.spawn_time = engine_now();
    lt.lifetime   = constants::plasma_impact_duration;

    auto& vd = engine_add_component<VFXData>(e);
    vd.initial_scale = 1.0f;
    vd.final_scale   = 4.0f;
    vd.initial_alpha = 0.9f;
    vd.final_alpha   = 0.0f;
    vd.r             = 1.0f;
    vd.g             = 0.5f;
    vd.b             = 0.1f;
}

void spawn_laser_impact(const glm::vec3& position) {
    static const float s_rgba[4] = {1.0f, 0.2f, 0.05f, 1.0f};
    auto  e    = engine_spawn_sphere(position, 1.0f,
                          renderer_make_unlit_material(s_rgba));

    Material& mat = engine_get_component<EntityMaterial>(e).mat;
    mat.pipeline.blend       = BlendMode::AlphaBlend;
    mat.pipeline.depth_write = false;
    mat.pipeline.render_queue = 2;

    auto& lt = engine_add_component<Lifetime>(e);
    lt.spawn_time = engine_now();
    lt.lifetime   = constants::laser_impact_duration;

    auto& vd = engine_add_component<VFXData>(e);
    vd.initial_scale = 0.5f;
    vd.final_scale   = 3.5f;
    vd.initial_alpha = 1.0f;
    vd.final_alpha   = 0.0f;
    vd.r             = 1.0f;
    vd.g             = 0.2f;
    vd.b             = 0.05f;
}

void spawn_shield_impact(const glm::vec3& position) {
    static const float s_rgba[4] = {0.5f, 0.7f, 1.0f, 0.9f};
    auto  e    = engine_spawn_sphere(position, 1.0f,
                          renderer_make_unlit_material(s_rgba));

    Material& mat = engine_get_component<EntityMaterial>(e).mat;
    mat.pipeline.blend       = BlendMode::AlphaBlend;
    mat.pipeline.depth_write = false;
    mat.pipeline.render_queue = 2;

    auto& lt = engine_add_component<Lifetime>(e);
    lt.spawn_time = engine_now();
    lt.lifetime   = constants::shield_impact_duration;

    auto& vd = engine_add_component<VFXData>(e);
    vd.initial_scale = 1.5f;
    vd.final_scale   = 3.0f;
    vd.initial_alpha = 0.9f;
    vd.final_alpha   = 0.0f;
    vd.r             = 0.5f;
    vd.g             = 0.7f;
    vd.b             = 1.0f;
}
