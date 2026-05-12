#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// ConvexHull — triangulated convex surface with face normals and local AABB.
// ---------------------------------------------------------------------------

struct ConvexHull {
    std::vector<glm::vec3> vertices;                  // local-space vertex positions
    std::vector<std::array<int, 3>> faces;             // vertex indices forming each triangle
    std::vector<glm::vec3> face_normals;               // precomputed outward unit normals (one per face)
    std::vector<std::pair<int,int>> unique_edges;      // precomputed undirected edge list for SAT
    glm::vec3 half_extents;                            // local-space AABB half-size of the vertex set
};

// Hull cache — stores one hull per asset path, shared across entity instances.
void hull_cache_insert(const std::string& path, ConvexHull hull);
const ConvexHull* hull_cache_get(const std::string& path);

// ---------------------------------------------------------------------------
// Convex-hull construction from raw mesh positions (float3, tightly packed).
// ---------------------------------------------------------------------------

// Build a convex hull from raw vertex positions using 26-direction extremal
// sampling followed by 3D gift-wrapping.  Returns an empty ConvexHull (vertices
// == empty) on degenerate input — callers detect this via `hull.vertices.empty()`.
ConvexHull compute_convex_hull(const float* positions, size_t vertex_count);

// ---------------------------------------------------------------------------
// Hull-vs-AABBSAT intersection (in-header for performance-critical use).
// ---------------------------------------------------------------------------

struct HullContact {
    bool hit;
    glm::vec3 normal;   // direction from hull toward box
    float depth;        // penetration depth along normal
    glm::vec3 point;    // midpoint of overlap region (world space)
};

HullContact hull_vs_aabb_sat(const ConvexHull& hull, const glm::vec3& hull_pos,
                              const glm::vec3& box_center, const glm::vec3& box_half);
