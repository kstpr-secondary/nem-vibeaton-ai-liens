#include "shield_vfx.h"
#include "camera_rig.h"
#include "components.h"
#include "constants.h"
#include <engine.h>
#include <renderer.h>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// shield_vfx_update — sync shield spheres to owners, update uniforms per frame
// ---------------------------------------------------------------------------

void shield_vfx_update(float /*dt*/) {
    auto& reg  = engine_registry();

   // Find the camera entity to get real camera world position for view_pos_w.
        glm::vec3 camera_pos = glm::vec3(0.f);
        auto cam_view = reg.view<Camera, Transform>();
        for (auto ce : cam_view) {
            const auto& ct = cam_view.get<Transform>(ce);
            camera_pos = ct.position;
            break;
        }

    auto view = reg.view<ShieldSphere, Transform, EntityMaterial>();

    for (auto e : view) {
        const auto& ss  = view.get<ShieldSphere>(e);
        auto&       t   = view.get<Transform>(e);
        auto&       em  = view.get<EntityMaterial>(e);

        // Owner may have been destroyed; fall back to identity.
        if (!reg.valid(ss.owner)) {
            t.scale = glm::vec3(0.f);
            continue;
        }

        const auto* owner_sh = reg.all_of<Shield>(ss.owner)
            ? &reg.get<Shield>(ss.owner) : nullptr;

        // Get owner's transform for position sync.
        const auto* owner_t = reg.all_of<Transform>(ss.owner)
            ? &reg.get<Transform>(ss.owner) : nullptr;

        if (owner_t) {
            t.position = owner_t->position;
        }

        // Scale stays at sphere radius (don't animate).
        t.scale = glm::vec3(ss.radius);
        t.rotation = glm::quat(1.f, 0.f, 0.f, 0.f);

        // Compute alpha from shield health ratio.
        // Spec 3.7.3: if no Shield component → alpha = 0 (invisible).
        float shield_alpha = 0.f;
        if (owner_sh && owner_sh->max > 0.f) {
            shield_alpha = (owner_sh->current / owner_sh->max) * constants::shield_max_alpha;
        }

        // Update the uniform blob.
        ShieldFSParams* p = material_uniforms_as<ShieldFSParams>(em.mat);
        p->view_pos_w.x   = camera_pos.x;
        p->view_pos_w.y   = camera_pos.y;
        p->view_pos_w.z   = camera_pos.z;
        p->shield_color.a = shield_alpha;

        // Hide sphere when shield is effectively depleted.
        if (shield_alpha < 0.01f) {
            t.scale = glm::vec3(0.f);
        }
    }
}
