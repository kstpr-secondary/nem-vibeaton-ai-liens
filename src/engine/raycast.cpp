#include "raycast.h"
#include "engine.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

// ---------------------------------------------------------------------------
// Ray-vs-AABB slab method (T021)
// ---------------------------------------------------------------------------

RayHit ray_vs_aabb(const glm::vec3& origin, const glm::vec3& dir,
                   float max_distance, const WorldAABB& aabb) {
    RayHit result{};

    float tmin = 0.f;
    float tmax = max_distance;
    int   hit_axis = -1;
    float hit_sign = 0.f;

    for (int i = 0; i < 3; ++i) {
        if (std::fabs(dir[i]) < 1e-8f) {
            // Parallel to this slab — miss if outside
            if (origin[i] < aabb.min[i] || origin[i] > aabb.max[i])
                return result;
            continue;
        }

        float inv_d = 1.f / dir[i];
        float t1    = (aabb.min[i] - origin[i]) * inv_d;
        float t2    = (aabb.max[i] - origin[i]) * inv_d;

        // t1 = entry t, t2 = exit t
        float sign = -1.f;  // normal faces away from ray for min-face entry
        if (inv_d < 0.f) {
            std::swap(t1, t2);
            sign = 1.f;     // entered from max face; normal points outward (+)
        }

        if (t1 > tmin) {
            tmin     = t1;
            hit_axis = i;
            hit_sign = sign;
        }
        tmax = std::min(tmax, t2);

        if (tmin > tmax) return result;  // miss
    }

    result.hit      = true;
    result.distance = tmin;
    result.point    = origin + dir * tmin;
    if (hit_axis >= 0) {
        result.normal           = glm::vec3(0.f);
        result.normal[hit_axis] = hit_sign;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Public query API (T021)
// ---------------------------------------------------------------------------

std::optional<RaycastHit> engine_raycast(const glm::vec3& origin,
                                          const glm::vec3& direction,
                                          float            max_distance) {
    auto& reg = engine_registry();
    std::optional<RaycastHit> best;

    // entt does not pass empty tag types (Interactable) to .each() lambdas
    reg.view<Transform, Collider, Interactable>().each(
        [&](auto e, const Transform& t, const Collider& c) {
            WorldAABB aabb = compute_world_aabb(t.position, c.half_extents);
            RayHit    h    = ray_vs_aabb(origin, direction, max_distance, aabb);
            if (!h.hit) return;
            if (!best || h.distance < best->distance) {
                best = RaycastHit{e, h.point, h.normal, h.distance};
            }
        });

    return best;
}

std::vector<entt::entity> engine_overlap_aabb(const glm::vec3& center,
                                               const glm::vec3& half_extents) {
    auto& reg = engine_registry();
    WorldAABB query = compute_world_aabb(center, half_extents);
    std::vector<entt::entity> results;

    reg.view<Transform, Collider, Interactable>().each(
        [&](auto e, const Transform& t, const Collider& c) {
            WorldAABB aabb = compute_world_aabb(t.position, c.half_extents);
            if (aabb_overlap(query, aabb))
                results.push_back(e);
        });

    return results;
}
