#pragma once

#include "collider.h"
#include <glm/glm.hpp>

// Result from a single ray-vs-AABB slab test.
// Exposed here so test_collision.cpp can unit-test the math directly.
struct RayHit {
    bool      hit      = false;
    float     distance = 0.f;
    glm::vec3 normal   = {0.f, 0.f, 0.f};  // outward surface normal at hit
    glm::vec3 point    = {0.f, 0.f, 0.f};  // world-space hit point
};

// Slab-method ray-vs-AABB test.
// direction need not be normalised but must not be the zero vector.
RayHit ray_vs_aabb(const glm::vec3& origin,
                   const glm::vec3& direction,
                   float            max_distance,
                   const WorldAABB& aabb);
