#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "collider.h"
#include "raycast.h"

#include <glm/glm.hpp>

using Catch::Approx;

// ---------------------------------------------------------------------------
// World AABB computation
// ---------------------------------------------------------------------------

TEST_CASE("compute_world_aabb: position + half_extents", "[collision]") {
    WorldAABB box = compute_world_aabb({1.f, 2.f, 3.f}, {0.5f, 1.f, 1.5f});
    REQUIRE(box.min.x == Approx(0.5f));
    REQUIRE(box.min.y == Approx(1.f));
    REQUIRE(box.min.z == Approx(1.5f));
    REQUIRE(box.max.x == Approx(1.5f));
    REQUIRE(box.max.y == Approx(3.f));
    REQUIRE(box.max.z == Approx(4.5f));
}

TEST_CASE("compute_world_aabb: origin at zero", "[collision]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE(box.min.x == Approx(-1.f));
    REQUIRE(box.max.x == Approx( 1.f));
}

// ---------------------------------------------------------------------------
// AABB-vs-AABB overlap
// ---------------------------------------------------------------------------

TEST_CASE("aabb_overlap: face-touching boxes return false (strict inequality)", "[collision]") {
    // a: min=(-1,-1,-1) max=(1,1,1)   b: min=(1,-1,-1) max=(3,1,1) — share exactly one face
    WorldAABB a = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    WorldAABB b = compute_world_aabb({2.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE_FALSE(aabb_overlap(a, b));
}

TEST_CASE("aabb_overlap: interpenetrating boxes returns true", "[collision]") {
    WorldAABB a = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    WorldAABB b = compute_world_aabb({0.5f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE(aabb_overlap(a, b));
}

TEST_CASE("aabb_overlap: separated on X returns false", "[collision]") {
    WorldAABB a = compute_world_aabb({-5.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    WorldAABB b = compute_world_aabb({ 5.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE_FALSE(aabb_overlap(a, b));
}

TEST_CASE("aabb_overlap: separated on Y returns false", "[collision]") {
    WorldAABB a = compute_world_aabb({0.f, -5.f, 0.f}, {1.f, 1.f, 1.f});
    WorldAABB b = compute_world_aabb({0.f,  5.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE_FALSE(aabb_overlap(a, b));
}

TEST_CASE("aabb_overlap: separated on Z returns false", "[collision]") {
    WorldAABB a = compute_world_aabb({0.f, 0.f, -5.f}, {1.f, 1.f, 1.f});
    WorldAABB b = compute_world_aabb({0.f, 0.f,  5.f}, {1.f, 1.f, 1.f});
    REQUIRE_FALSE(aabb_overlap(a, b));
}

TEST_CASE("aabb_overlap: box contained inside another returns true", "[collision]") {
    WorldAABB outer = compute_world_aabb({0.f, 0.f, 0.f}, {5.f, 5.f, 5.f});
    WorldAABB inner = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    REQUIRE(aabb_overlap(outer, inner));
}

// ---------------------------------------------------------------------------
// Ray-vs-AABB slab method
// ---------------------------------------------------------------------------

TEST_CASE("ray_vs_aabb: direct hit along Z axis", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Ray from z = -5 pointing +Z toward the box
    RayHit h = ray_vs_aabb({0.f, 0.f, -5.f}, {0.f, 0.f, 1.f}, 100.f, box);
    REQUIRE(h.hit);
    REQUIRE(h.distance == Approx(4.f));           // hits min-Z face at z = -1
    REQUIRE(h.point.z  == Approx(-1.f));
    REQUIRE(h.normal.z == Approx(-1.f));          // outward normal of min-Z face
    REQUIRE(h.normal.x == Approx(0.f));
    REQUIRE(h.normal.y == Approx(0.f));
}

TEST_CASE("ray_vs_aabb: ray misses to the side", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    RayHit h = ray_vs_aabb({5.f, 0.f, -5.f}, {0.f, 0.f, 1.f}, 100.f, box);
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_aabb: ray pointing away misses", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    RayHit h = ray_vs_aabb({0.f, 0.f, -5.f}, {0.f, 0.f, -1.f}, 100.f, box);
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_aabb: max_distance too short misses", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Box starts at z=-1, but max_distance is only 3 (ray from z=-5)
    RayHit h = ray_vs_aabb({0.f, 0.f, -5.f}, {0.f, 0.f, 1.f}, 3.f, box);
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_aabb: hit along X axis, correct normal", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Ray from x = -5 pointing +X
    RayHit h = ray_vs_aabb({-5.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 100.f, box);
    REQUIRE(h.hit);
    REQUIRE(h.distance == Approx(4.f));
    REQUIRE(h.normal.x == Approx(-1.f));
    REQUIRE(h.normal.y == Approx(0.f));
    REQUIRE(h.normal.z == Approx(0.f));
}

TEST_CASE("ray_vs_aabb: diagonal ray hits correctly", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Diagonal ray: should hit if aimed at box
    RayHit h = ray_vs_aabb({-3.f, 0.f, -3.f}, {1.f, 0.f, 1.f}, 100.f, box);
    REQUIRE(h.hit);
    REQUIRE(h.distance > 0.f);
}

TEST_CASE("ray_vs_aabb: ray parallel to face outside slab misses", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Ray at y = 5 (outside box on Y), pointing along X
    RayHit h = ray_vs_aabb({-5.f, 5.f, 0.f}, {1.f, 0.f, 0.f}, 100.f, box);
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_aabb: ray parallel to face inside slab hits", "[collision][ray]") {
    WorldAABB box = compute_world_aabb({0.f, 0.f, 0.f}, {1.f, 1.f, 1.f});
    // Ray at y=0 (inside Y slab), z=0 (inside Z slab), pointing from x=-5 → +X
    RayHit h = ray_vs_aabb({-5.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 100.f, box);
    REQUIRE(h.hit);
}
