#include "convex_hull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static const float kHullEpsilon = 1e-6f;

static glm::vec3 v3(const float* p, size_t i) {
    return glm::vec3(p[i * 3], p[i * 3 + 1], p[i * 3 + 2]);
}

// Signed volume of tetrahedron (o, a, b, c) * 6.
static float tetra_vol6(const glm::vec3& o, const glm::vec3& a,
                        const glm::vec3& b, const glm::vec3& c) {
    return glm::dot(c - o, glm::cross(a - o, b - o));
}

// Normal of triangle (a, b, c). Returns zero vector if degenerate.
static glm::vec3 tri_normal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    return glm::cross(b - a, c - a);
}

// Check if two vertices are the same (within epsilon).
static bool verts_equal(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(a - b) < kHullEpsilon;
}

// ---------------------------------------------------------------------------
// Hull cache
// ---------------------------------------------------------------------------

// unique_ptr keeps managed ConvexHull objects at a stable address — map rehash
// never moves them, so const ConvexHull* pointers stored in components stay valid.
static std::unordered_map<std::string, std::unique_ptr<ConvexHull>>& hull_cache_store() {
    static std::unordered_map<std::string, std::unique_ptr<ConvexHull>> store;
    return store;
}

void hull_cache_insert(const std::string& path, ConvexHull hull) {
    hull_cache_store()[path] = std::make_unique<ConvexHull>(std::move(hull));
}

const ConvexHull* hull_cache_get(const std::string& path) {
    auto& store = hull_cache_store();
    auto it = store.find(path);
    return it != store.end() ? it->second.get() : nullptr;
}

// ---------------------------------------------------------------------------
// Face normal and half-extents computation
// ---------------------------------------------------------------------------

static void compute_face_normals(const ConvexHull& hull, ConvexHull& out) {
    out.vertices = hull.vertices;
    out.faces = hull.faces;
    out.face_normals.resize(hull.faces.size());
    for (size_t i = 0; i < hull.faces.size(); ++i) {
        auto& f = hull.faces[i];
        glm::vec3 n = tri_normal(hull.vertices[f[0]],
                                 hull.vertices[f[1]],
                                 hull.vertices[f[2]]);
        float len = glm::length(n);
        // Degenerate (zero-area) face: use -Z as a safe placeholder.  This path
        // should never fire on valid gift-wrapped hulls; it guards against corrupt input.
        out.face_normals[i] = (len > 1e-8f) ? (n / len) : glm::vec3(0, 0, -1);
    }
}

static void compute_half_extents(const std::vector<glm::vec3>& verts, glm::vec3& half_extents) {
    if (verts.empty()) { half_extents = glm::vec3(0); return; }
    float mn_x = verts[0].x, mx_x = verts[0].x;
    float mn_y = verts[0].y, mx_y = verts[0].y;
    float mn_z = verts[0].z, mx_z = verts[0].z;
    for (size_t i = 1; i < verts.size(); ++i) {
        const auto& v = verts[i];
        if (v.x < mn_x) mn_x = v.x; if (v.x > mx_x) mx_x = v.x;
        if (v.y < mn_y) mn_y = v.y; if (v.y > mx_y) mx_y = v.y;
        if (v.z < mn_z) mn_z = v.z; if (v.z > mx_z) mx_z = v.z;
    }
    half_extents = glm::vec3((mx_x - mn_x) * 0.5f,
                             (mx_y - mn_y) * 0.5f,
                             (mx_z - mn_z) * 0.5f);
}

// ---------------------------------------------------------------------------
// compute_convex_hull — 26-direction sampling + gift-wrapping
// ---------------------------------------------------------------------------

// Gift-wrapping pivot: for directed edge (a→b), find vertex c such that all
// other candidates lie on the non-positive side of the plane through
// pts[a], pts[b], pts[c]  (outward normal = cross(pts[b]-pts[a], pts[c]-pts[a])).
// Returns -1 only if pts has fewer than 3 entries.
static int gw_pivot(const std::vector<glm::vec3>& pts, int a, int b) {
    int best = -1;
    for (int i = 0; i < (int)pts.size(); ++i) {
        if (i == a || i == b) continue;
        if (best < 0) { best = i; continue; }
        // If pts[i] is outside the current tentative face (a,b,best), update best.
        glm::vec3 n = glm::cross(pts[b] - pts[a], pts[best] - pts[a]);
        if (glm::dot(n, pts[i] - pts[a]) > 1e-8f) best = i;
    }
    return best;
}

// Normalized 26-direction set, computed once at first call.
static const std::vector<glm::vec3>& get_directions26() {
    static const std::vector<glm::vec3> dirs = []() {
        struct Dir3 { float x, y, z; };
        static const Dir3 raw[] = {
            { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1},
            { 1,  1,  1}, {-1,  1,  1}, { 1, -1,  1}, { 1,  1, -1},
            {-1, -1,  1}, {-1,  1, -1}, { 1, -1, -1}, {-1, -1, -1},
            { 1,  1,  0}, {-1, -1,  0}, { 1, -1,  0}, {-1,  1,  0},
            { 1,  0,  1}, {-1,  0, -1}, { 1,  0, -1}, {-1,  0,  1},
            { 0,  1,  1}, { 0, -1, -1}, { 0,  1, -1}, { 0, -1,  1},
        };
        const float is3 = 1.0f / std::sqrt(3.0f);
        const float is2 = 1.0f / std::sqrt(2.0f);
        std::vector<glm::vec3> d;
        d.reserve(26);
        for (size_t i = 0;  i < 6;  ++i) d.emplace_back(raw[i].x,        raw[i].y,        raw[i].z       );
        for (size_t i = 6;  i < 14; ++i) d.emplace_back(raw[i].x * is3,  raw[i].y * is3,  raw[i].z * is3 );
        for (size_t i = 14; i < 26; ++i) d.emplace_back(raw[i].x * is2,  raw[i].y * is2,  raw[i].z * is2 );
        return d;
    }();
    return dirs;
}

ConvexHull compute_convex_hull(const float* positions, size_t vertex_count) {
    ConvexHull result;

    if (!positions || vertex_count < 4) return result;

    // --- Step 1: Sample 26 fixed directions, collect unique candidates ---
    const auto& directions = get_directions26();

    // Find furthest vertex in each direction.
    std::vector<size_t> best_idx(directions.size(), 0);
    for (size_t d = 0; d < directions.size(); ++d) {
        float best_dot = -1e30f;
        for (size_t i = 0; i < vertex_count; ++i) {
            float dot = glm::dot(directions[d], v3(positions, i));
            if (dot > best_dot) { best_dot = dot; best_idx[d] = i; }
        }
    }

    // Collect unique vertices.
    std::vector<size_t> candidates;
    for (size_t idx : best_idx) {
        bool found = false;
        for (size_t c : candidates) {
            if (verts_equal(v3(positions, c), v3(positions, idx))) { found = true; break; }
        }
        if (!found) candidates.push_back(idx);
    }

    size_t n = candidates.size();
    if (n < 4) return result;

    std::vector<glm::vec3> cv(n);
    for (size_t i = 0; i < n; ++i) cv[i] = v3(positions, candidates[i]);

    // --- Step 1b: Degeneracy check — scan all C(n,4) combinations ---
    // Checking only cv[0..3] would false-positive if those four happen to be
    // coplanar while later candidates are not.  Scan all quads instead.
    bool found_nondegenerate = false;
    for (size_t i = 0; i < n && !found_nondegenerate; ++i)
        for (size_t j = i + 1; j < n && !found_nondegenerate; ++j)
            for (size_t k = j + 1; k < n && !found_nondegenerate; ++k)
                for (size_t l = k + 1; l < n; ++l) {
                    if (std::abs(tetra_vol6(cv[i], cv[j], cv[k], cv[l])) >= 1e-9f) {
                        found_nondegenerate = true;
                        break;
                    }
                }
    if (!found_nondegenerate) {
        fprintf(stderr, "[ENGINE] convex_hull: degenerate mesh (all candidates coplanar), hull disabled\n");
        return result;
    }

    // --- Step 2: 3D Gift-Wrapping (Jarvis March) ---
    // Seed: lowest-Y vertex (guaranteed on the hull boundary).
    int seed = 0;
    for (int i = 1; i < (int)n; ++i)
        if (cv[i].y < cv[seed].y) seed = i;

    // Find v1 via a downward-pivot so seed→v1 is guaranteed to be a hull boundary
    // edge — the arbitrary "pick index 0 or 1" choice can pick an interior diagonal
    // when the seed vertex has no edge toward those indices on the hull surface.
    int v1 = -1;
    {
        const glm::vec3 down(0.f, -1.f, 0.f);
        for (int i = 0; i < (int)n; ++i) {
            if (i == seed) continue;
            if (v1 < 0) { v1 = i; continue; }
            glm::vec3 ncur = glm::cross(down, cv[v1] - cv[seed]);
            if (glm::dot(ncur, cv[i] - cv[seed]) > 1e-8f) v1 = i;
        }
    }
    if (v1 < 0) return result;
    int v2 = gw_pivot(cv, seed, v1);
    if (v2 < 0) return result;

    using HEdge = std::pair<int, int>;
    std::set<HEdge>         closed;
    std::set<HEdge>         open_set;
    std::vector<HEdge>      open_queue;
    std::vector<std::array<int, 3>> raw_faces;

    auto add_face_gw = [&](int a, int b, int c) {
        raw_faces.push_back({a, b, c});
        closed.insert({a, b}); closed.insert({b, c}); closed.insert({c, a});
        HEdge twins[3] = {{b, a}, {c, b}, {a, c}};
        for (const auto& e : twins) {
            if (!closed.count(e) && !open_set.count(e)) {
                open_set.insert(e);
                open_queue.push_back(e);
            }
        }
    };

    add_face_gw(seed, v1, v2);

    while (!open_queue.empty()) {
        HEdge e = open_queue.back();
        open_queue.pop_back();
        if (closed.count(e)) continue;
        int d = gw_pivot(cv, e.first, e.second);
        if (d < 0) continue;
        add_face_gw(e.first, e.second, d);
    }

    if (raw_faces.empty()) return result;

    // --- Step 3: Build final ConvexHull ---
    ConvexHull tmp;
    tmp.vertices = cv;
    tmp.faces = raw_faces;
    compute_face_normals(tmp, result);
    compute_half_extents(result.vertices, result.half_extents);

    // Precompute unique undirected edges once so hull_vs_aabb_sat avoids per-call rebuild.
    {
        std::set<std::pair<int,int>> seen;
        for (const auto& f : result.faces) {
            for (int j = 0; j < 3; ++j) {
                int a = f[j], b = f[(j + 1) % 3];
                if (a > b) std::swap(a, b);
                if (seen.insert({a, b}).second)
                    result.unique_edges.push_back({a, b});
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// hull_vs_aabb_sat — SAT between ConvexHull and AABB
// ---------------------------------------------------------------------------

static constexpr float kSATMin = 1e-6f;

HullContact hull_vs_aabb_sat(const ConvexHull& hull, const glm::vec3& hull_pos,
                              const glm::vec3& box_center, const glm::vec3& box_half) {
    HullContact contact{};

    if (hull.vertices.empty() || hull.face_normals.empty()) return contact;

    // Hull ≤ 26 vertices by construction — stack array avoids heap allocation per call.
    size_t nv = hull.vertices.size();
    glm::vec3 wv[26];
    for (size_t i = 0; i < nv; ++i) wv[i] = hull.vertices[i] + hull_pos;

    // Project hull vertices onto an axis, return {min, max}.
    auto project_hull = [&](const glm::vec3& axis) {
        float mn = 1e30f, mx = -1e30f;
        for (size_t i = 0; i < nv; ++i) {
            float d = glm::dot(wv[i], axis);
            if (d < mn) mn = d;
            if (d > mx) mx = d;
        }
        return std::make_pair(mn, mx);
    };

    // Track minimum overlap axis for contact info.
    float min_overlap = 1e30f;
    glm::vec3 best_axis(0, 0, 0);

    auto test_and_track = [&](const glm::vec3& axis) -> bool {
        if (glm::length(axis) < kSATMin) return true;
        float pmin_h, pmax_h;
        { auto a = project_hull(axis); pmin_h = a.first; pmax_h = a.second; }
        // AABB projection via center±radius — O(1) vs. 8-corner iteration.
        float r    = std::fabs(axis.x) * box_half.x
                   + std::fabs(axis.y) * box_half.y
                   + std::fabs(axis.z) * box_half.z;
        float c    = glm::dot(box_center, axis);
        float pmin_b = c - r, pmax_b = c + r;
        if (pmax_h < pmin_b || pmax_b < pmin_h) return false;
        float overlap = std::min(pmax_h, pmax_b) - std::max(pmin_h, pmin_b);
        if (overlap < min_overlap) {
            min_overlap = overlap;
            best_axis = axis;
        }
        return true;
    };

    // --- Axis set 1: AABB face normals (+X, +Y, +Z) ---
    glm::vec3 axes_aabb[] = {{1,0,0}, {0,1,0}, {0,0,1}};
    for (auto& ax : axes_aabb) {
        if (!test_and_track(ax)) return contact;
    }

    // --- Axis set 2: hull face normals ---
    for (size_t i = 0; i < hull.face_normals.size(); ++i) {
        if (!test_and_track(hull.face_normals[i])) return contact;
    }

    // --- Axis set 3: edge x axis (each unique hull edge x {+X,+Y,+Z}) ---
    // hull.unique_edges is precomputed in compute_convex_hull — no per-call rebuild.
    glm::vec3 unit_axes[] = {{1,0,0}, {0,1,0}, {0,0,1}};
    for (const auto& [ea, eb] : hull.unique_edges) {
        glm::vec3 edge_dir = wv[eb] - wv[ea];
        for (auto& ua : unit_axes) {
            glm::vec3 cross = glm::cross(edge_dir, ua);
            float cl = glm::length(cross);
            if (cl < kSATMin) continue;
            cross /= cl;
            if (!test_and_track(cross)) return contact;
        }
    }

    // --- Compute contact info from minimum-overlap axis ---
    contact.hit = true;
    // Normal: direction from hull center toward box center.
    glm::vec3 hcenter(0, 0, 0);
    for (size_t i = 0; i < nv; ++i) hcenter += wv[i];
    hcenter /= (float)nv;
    glm::vec3 to_box = box_center - hcenter;
    float dot_tb = glm::dot(to_box, best_axis);
    contact.normal = (dot_tb >= 0.f) ? best_axis : -best_axis;

    // Depth: minimum overlap.
    contact.depth = min_overlap;

    // Point: midpoint of overlap interval, projected back to world.
    float pmin_h, pmax_h, pmin_b, pmax_b;
    { auto a = project_hull(contact.normal); pmin_h = a.first; pmax_h = a.second; }
    {
        float r  = std::fabs(contact.normal.x) * box_half.x
                 + std::fabs(contact.normal.y) * box_half.y
                 + std::fabs(contact.normal.z) * box_half.z;
        float c  = glm::dot(box_center, contact.normal);
        pmin_b = c - r; pmax_b = c + r;
    }
    float mid = (std::max(pmin_h, pmin_b) + std::min(pmax_h, pmax_b)) * 0.5f;
    contact.point = hcenter + contact.normal * (mid - glm::dot(hcenter, contact.normal));

    return contact;
}
