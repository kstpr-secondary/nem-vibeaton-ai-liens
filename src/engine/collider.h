#pragma once

#include "components.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

// ---------------------------------------------------------------------------
// World-space AABB
// ---------------------------------------------------------------------------

struct WorldAABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Compute world AABB from entity position and local half-extents.
// Rotation and scale are ignored per MVP spec.
inline WorldAABB compute_world_aabb(const glm::vec3& position, const glm::vec3& half_extents) {
    return { position - half_extents, position + half_extents };
}

inline bool aabb_overlap(const WorldAABB& a, const WorldAABB& b) {
    return (a.max.x > b.min.x && a.min.x < b.max.x) &&
           (a.max.y > b.min.y && a.min.y < b.max.y) &&
           (a.max.z > b.min.z && a.min.z < b.max.z);
}

// ---------------------------------------------------------------------------
// Collision pair — output of brute-force detection
// ---------------------------------------------------------------------------

struct CollisionPair {
    entt::entity a, b;
    glm::vec3 normal;  // minimum-penetration axis, from b toward a
    float     depth;   // penetration depth along normal
    glm::vec3 point;   // midpoint of overlap region
};

// Compute contact for an overlapping pair.
// Returns depth <= 0 if not overlapping (caller should discard).
CollisionPair compute_contact(entt::entity ea, const WorldAABB& a,
                              entt::entity eb, const WorldAABB& b);

// O(N²) brute-force all-pairs detection.
// Reads all Transform + Collider entities from engine_registry().
// Skips Static-vs-Static pairs.
std::vector<CollisionPair> detect_collisions();
