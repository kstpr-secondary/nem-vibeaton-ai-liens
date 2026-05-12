#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "convex_hull.h"
#include "raycast.h"

#include <glm/glm.hpp>

#include <array>
#include <cmath>
#include <random>
#include <vector>

using Catch::Approx;

// ---------------------------------------------------------------------------
// compute_convex_hull — unit cube (8 vertices)
// ---------------------------------------------------------------------------

TEST_CASE("compute_convex_hull: 8 unit-cube vertices produces valid hull", "[convex_hull]") {
    // Unit cube centered at origin, vertices at ±0.5 on each axis.
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,   // 0
         0.5f, -0.5f, -0.5f,   // 1
        -0.5f,  0.5f, -0.5f,   // 2
         0.5f,  0.5f, -0.5f,   // 3
        -0.5f, -0.5f,  0.5f,   // 4
         0.5f, -0.5f,  0.5f,   // 5
        -0.5f,  0.5f,  0.5f,   // 6
         0.5f,  0.5f,  0.5f,   // 7
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);

    REQUIRE(!hull.vertices.empty());
    // The 26-direction sampler should return all 8 vertices of the cube.
    REQUIRE(hull.vertices.size() == Approx(8.0));

    // A cube has 12 triangular faces (2 per square face).
    REQUIRE(hull.faces.size() == Approx(12.0));
    REQUIRE(hull.face_normals.size() == Approx(12.0));

    // Half extents should be approximately {0.5, 0.5, 0.5}.
    REQUIRE(hull.half_extents.x == Approx(0.5f).epsilon(0.05f));
    REQUIRE(hull.half_extents.y == Approx(0.5f).epsilon(0.05f));
    REQUIRE(hull.half_extents.z == Approx(0.5f).epsilon(0.05f));

    // All face normals should be unit length and aligned with ±X, ±Y, or ±Z.
    for (size_t i = 0; i < hull.face_normals.size(); ++i) {
        float len = glm::length(hull.face_normals[i]);
        REQUIRE(len == Approx(1.0f).epsilon(0.01f));

        // Each normal should be close to a cardinal direction.
        auto& n = hull.face_normals[i];
        bool ok = (std::fabs(n.x) > 0.9f) || (std::fabs(n.y) > 0.9f) || (std::fabs(n.z) > 0.9f);
        REQUIRE(ok);
    }
}

// ---------------------------------------------------------------------------
// compute_convex_hull — degenerate coplanar vertices
// ---------------------------------------------------------------------------

TEST_CASE("compute_convex_hull: coplanar vertices returns empty hull", "[convex_hull]") {
    // Four points all on the XY plane (z = 0).
    std::array<float, 12> verts = {{
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(hull.vertices.empty());
}

// ---------------------------------------------------------------------------
// compute_convex_hull — too few vertices
// ---------------------------------------------------------------------------

TEST_CASE("compute_convex_hull: fewer than 4 vertices returns empty", "[convex_hull]") {
    std::array<float, 9> verts = {{
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(hull.vertices.empty());
}

TEST_CASE("compute_convex_hull: null pointer returns empty", "[convex_hull]") {
    ConvexHull hull = compute_convex_hull(nullptr, 0);
    REQUIRE(hull.vertices.empty());
}

// ---------------------------------------------------------------------------
// hull_cache — insert and retrieve
// ---------------------------------------------------------------------------

TEST_CASE("hull_cache: insert then get returns same hull", "[convex_hull][cache]") {
    // Clear any prior state by getting a unique path first.
    HullContact dummy = hull_vs_aabb_sat({}, glm::vec3(0), glm::vec3(0), glm::vec3(0));
    (void)dummy;

    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f, 0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, 0.5f,  0.5f,  0.5f,
    }};

    ConvexHull expected = compute_convex_hull(verts.data(), verts.size() / 3);
    hull_cache_insert("test_cube", std::move(expected));

    const ConvexHull* got = hull_cache_get("test_cube");
    REQUIRE(got != nullptr);
    REQUIRE(!got->vertices.empty());
    REQUIRE(got->vertices.size() == Approx(8.0));

    // Non-existent path returns nullptr.
    const ConvexHull* missing = hull_cache_get("nonexistent_path_xyz");
    REQUIRE(missing == nullptr);
}

// ---------------------------------------------------------------------------
// hull_vs_aabb_sat — overlap
// ---------------------------------------------------------------------------

TEST_CASE("hull_vs_aabb_sat: hull and AABB overlapping", "[convex_hull][sat]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Hull at origin, box centered at (0.25, 0, 0) with half-extent 0.5 — overlaps.
    HullContact c = hull_vs_aabb_sat(hull, glm::vec3(0), glm::vec3(0.25f, 0, 0), glm::vec3(0.5f));
    REQUIRE(c.hit);
    REQUIRE(c.depth > 0.f);
    REQUIRE(glm::length(c.normal) == Approx(1.f).epsilon(0.01f));
}

TEST_CASE("hull_vs_aabb_sat: hull and AABB separated", "[convex_hull][sat]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Hull at origin, box far away — no overlap.
    HullContact c = hull_vs_aabb_sat(hull, glm::vec3(0), glm::vec3(5.f, 0, 0), glm::vec3(0.5f));
    REQUIRE_FALSE(c.hit);
}

TEST_CASE("hull_vs_aabb_sat: empty hull returns no hit", "[convex_hull][sat]") {
    ConvexHull empty;  // vertices.empty()
    HullContact c = hull_vs_aabb_sat(empty, glm::vec3(0), glm::vec3(0), glm::vec3(1));
    REQUIRE_FALSE(c.hit);
}

// ---------------------------------------------------------------------------
// ray_vs_hull — direct hit
// ---------------------------------------------------------------------------

TEST_CASE("ray_vs_hull: direct hit along Z axis", "[convex_hull][ray]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Ray from z=-3 pointing +Z toward the hull centered at origin.
    RayHit h = ray_vs_hull({0.f, 0.f, -3.f}, {0.f, 0.f, 1.f}, 10.f, hull, glm::vec3(0));
    REQUIRE(h.hit);
    REQUIRE(h.distance == Approx(2.5f).epsilon(0.05f));  // hits at z = -0.5
}

TEST_CASE("ray_vs_hull: ray misses to the side", "[convex_hull][ray]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Ray at x=5 (outside hull on X), pointing +Z.
    RayHit h = ray_vs_hull({5.f, 0.f, -3.f}, {0.f, 0.f, 1.f}, 10.f, hull, glm::vec3(0));
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_hull: ray pointing away misses", "[convex_hull][ray]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Ray from z=-3 pointing -Z (away from hull).
    RayHit h = ray_vs_hull({0.f, 0.f, -3.f}, {0.f, 0.f, -1.f}, 10.f, hull, glm::vec3(0));
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_hull: max_distance too short", "[convex_hull][ray]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Hull start is at z=-0.5 (distance 2.5 from ray origin at z=-3), but max_dist=1.
    RayHit h = ray_vs_hull({0.f, 0.f, -3.f}, {0.f, 0.f, 1.f}, 1.f, hull, glm::vec3(0));
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_hull: empty hull returns no hit", "[convex_hull][ray]") {
    ConvexHull empty;
    RayHit h = ray_vs_hull({0.f, 0.f, -3.f}, {0.f, 0.f, 1.f}, 10.f, empty, glm::vec3(0));
    REQUIRE_FALSE(h.hit);
}

TEST_CASE("ray_vs_hull: ray starting inside hull hits at distance 0", "[convex_hull][ray]") {
    // Interior-origin: tmin stays at 0; origin passes inside-verification → hit=true, distance=0.
    std::array<float, 24> verts = {{
        -1.f, -1.f, -1.f,  1.f, -1.f, -1.f,
        -1.f,  1.f, -1.f,  1.f,  1.f, -1.f,
        -1.f, -1.f,  1.f,  1.f, -1.f,  1.f,
        -1.f,  1.f,  1.f,  1.f,  1.f,  1.f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    RayHit h = ray_vs_hull({0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 10.f, hull, glm::vec3(0));
    REQUIRE(h.hit);
    REQUIRE(h.distance == Approx(0.f));
}

// ---------------------------------------------------------------------------
// compute_convex_hull — tetrahedron
// ---------------------------------------------------------------------------

TEST_CASE("compute_convex_hull: tetrahedron produces 4 vertices and 4 faces", "[convex_hull]") {
    // Regular tetrahedron inscribed in a cube of side 2 centered at origin.
    std::array<float, 12> verts = {{
         1.f,  1.f,  1.f,
        -1.f, -1.f,  1.f,
        -1.f,  1.f, -1.f,
         1.f, -1.f, -1.f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());
    REQUIRE(hull.vertices.size() == 4);
    REQUIRE(hull.faces.size() == 4);
    REQUIRE(hull.face_normals.size() == 4);
    for (const auto& n : hull.face_normals) {
        REQUIRE(glm::length(n) == Approx(1.f).epsilon(0.01f));
    }
}

// ---------------------------------------------------------------------------
// compute_convex_hull — 1000 unit-sphere samples
// ---------------------------------------------------------------------------

TEST_CASE("compute_convex_hull: 1000 unit-sphere samples produce ≤26 vertices all on sphere", "[convex_hull]") {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    std::vector<float> positions;
    positions.reserve(1000 * 3);
    size_t generated = 0;
    while (generated < 1000) {
        float x = dist(rng), y = dist(rng), z = dist(rng);
        float len = std::sqrt(x * x + y * y + z * z);
        if (len < 1e-4f) continue;
        positions.push_back(x / len);
        positions.push_back(y / len);
        positions.push_back(z / len);
        ++generated;
    }

    ConvexHull hull = compute_convex_hull(positions.data(), 1000);
    REQUIRE(!hull.vertices.empty());
    REQUIRE(hull.vertices.size() <= 26);
    for (const auto& v : hull.vertices) {
        REQUIRE(glm::length(v) == Approx(1.f).epsilon(0.01f));
    }
}

// ---------------------------------------------------------------------------
// hull_vs_aabb_sat — large box enclosing hull
// ---------------------------------------------------------------------------

TEST_CASE("hull_vs_aabb_sat: large box enclosing hull returns hit with unit normal", "[convex_hull][sat]") {
    std::array<float, 24> verts = {{
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    }};

    ConvexHull hull = compute_convex_hull(verts.data(), verts.size() / 3);
    REQUIRE(!hull.vertices.empty());

    // Large box centered at hull origin fully contains the hull.
    HullContact c = hull_vs_aabb_sat(hull, glm::vec3(0), glm::vec3(0), glm::vec3(5.f));
    REQUIRE(c.hit);
    REQUIRE(c.depth > 0.f);
    REQUIRE(glm::length(c.normal) == Approx(1.f).epsilon(0.01f));
}
