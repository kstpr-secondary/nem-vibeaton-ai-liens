#include "collider.h"
#include "engine.h"

#include <algorithm>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Contact computation
// ---------------------------------------------------------------------------

CollisionPair compute_contact(entt::entity ea, const WorldAABB& a,
                              entt::entity eb, const WorldAABB& b) {
    CollisionPair out{ea, eb, {}, 0.f, {}};

    float ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    float oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    // Minimum penetration axis → contact normal (from b toward a)
    glm::vec3 ca = (a.min + a.max) * 0.5f;
    glm::vec3 cb = (b.min + b.max) * 0.5f;

    if (ox <= oy && ox <= oz) {
        out.depth  = ox;
        out.normal = glm::vec3(ca.x > cb.x ? 1.f : -1.f, 0.f, 0.f);
    } else if (oy <= ox && oy <= oz) {
        out.depth  = oy;
        out.normal = glm::vec3(0.f, ca.y > cb.y ? 1.f : -1.f, 0.f);
    } else {
        out.depth  = oz;
        out.normal = glm::vec3(0.f, 0.f, ca.z > cb.z ? 1.f : -1.f);
    }

    // Contact point at midpoint of overlap region
    glm::vec3 ov_min = glm::max(a.min, b.min);
    glm::vec3 ov_max = glm::min(a.max, b.max);
    out.point = (ov_min + ov_max) * 0.5f;

    return out;
}

// ---------------------------------------------------------------------------
// O(N²) brute-force all-pairs detection
// ---------------------------------------------------------------------------

std::vector<CollisionPair> detect_collisions() {
    auto& reg = engine_registry();

    struct Entry {
        entt::entity e;
        WorldAABB    aabb;
        bool         is_static;
    };

    std::vector<Entry> entries;
    entries.reserve(128);

    reg.view<Transform, Collider>().each([&](auto e, const Transform& t, const Collider& c) {
        entries.push_back({e, compute_world_aabb(t.position, c.half_extents),
                           reg.all_of<Static>(e)});
    });

    std::vector<CollisionPair> pairs;
    for (size_t i = 0; i < entries.size(); ++i) {
        for (size_t j = i + 1; j < entries.size(); ++j) {
            if (entries[i].is_static && entries[j].is_static) continue;
            if (!aabb_overlap(entries[i].aabb, entries[j].aabb)) continue;
            pairs.push_back(compute_contact(entries[i].e, entries[i].aabb,
                                            entries[j].e, entries[j].aabb));
        }
    }
    return pairs;
}
