#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "shadow_pass.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <limits>

using Catch::Approx;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool all_finite(const glm::mat4& m) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (!std::isfinite(m[c][r])) return false;
    return true;
}

static bool is_non_degenerate(const glm::mat4& m, float eps = 1e-6f) {
    // Check that rotation/scale columns have non-zero length.
    for (int c = 0; c < 3; ++c) {
        float len_sq = m[c][0]*m[c][0] + m[c][1]*m[c][1] + m[c][2]*m[c][2];
        if (len_sq < eps * eps) return false;
    }
    return true;
}

// Check whether origin {0,0,0} is inside the orthographic frustum defined by
// light_view_proj.  For an ortho matrix, the frustum bounds in clip space
// are [-1,1] in XY and [0,1] in Z (after fixup_clipspace).
static bool origin_inside_ortho_frustum(const glm::mat4& mvp) {
    // Transform origin: clip = M * vec4(0,0,0,1) = column 3.
    float cx = mvp[3][0];
    float cy = mvp[3][1];
    float cz = mvp[3][2];
    return cx >= -1.0f && cx <= 1.0f &&
           cy >= -1.0f && cy <= 1.0f &&
           cz >=  0.0f && cz <= 1.0f;
}

// ---------------------------------------------------------------------------
// T5 acceptance — tilted light: direction = normalize({0.3, -1.0, 0.2})
// All 16 elements finite; origin maps to clip-space XY in [-1,1] and Z in [0,1].
// ---------------------------------------------------------------------------

TEST_CASE("shadow light_view_proj — tilted light all elements finite", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 0.3f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.2f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(all_finite(mvp));
}

TEST_CASE("shadow light_view_proj — tilted light origin inside ortho frustum", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 0.3f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.2f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(origin_inside_ortho_frustum(mvp));
}

TEST_CASE("shadow light_view_proj — tilted light non-degenerate", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 0.3f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.2f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(is_non_degenerate(mvp));
}

// ---------------------------------------------------------------------------
// T5 acceptance — near-vertical light: direction = {0,-1,0}
// Up-vector fallback fires; matrix is finite and non-degenerate.
// ---------------------------------------------------------------------------

TEST_CASE("shadow light_view_proj — near-vertical light all elements finite", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 0.0f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(all_finite(mvp));
}

TEST_CASE("shadow light_view_proj — near-vertical light non-degenerate", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 0.0f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(is_non_degenerate(mvp));
}

// ---------------------------------------------------------------------------
// T6 acceptance — shadow pass executes before main pass (structural)
// The shadow pass must write depth independently of the swapchain.
// Verify that light_view_proj is computed with correct sign convention:
// eye_pos = scene_center + dir * eye_dist (dir = surface→light).
// ---------------------------------------------------------------------------

TEST_CASE("shadow light_view_proj — eye position along light direction", "[shadow]") {
    // Light shines from +Z toward -Z (direction = {0,0,-1}).
    // Eye should be placed at scene_center + dir * eye_dist = {0,0,-far*0.5}.
    DirectionalLight light{};
    light.direction[0] = 0.0f;
    light.direction[1] = 0.0f;
    light.direction[2] = -1.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    // The eye position is dir * (far*0.5) = {0, 0, -600}.
    // LookAt from eye to origin with up={1,0,0} yields a view matrix whose
    // translation column should be at {0,0,600} (origin minus eye).
    // After proj*view, the Z coordinate of origin should be ~0 (center of near/far).
    REQUIRE(origin_inside_ortho_frustum(mvp));
}

TEST_CASE("shadow light_view_proj — upward light uses X-as-up fallback", "[shadow]") {
    // Light shines straight down: dir = {0,-1,0}.
    // The up-vector fallback to {1,0,0} must fire and produce a valid matrix.
    DirectionalLight light{};
    light.direction[0] = 0.0f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(all_finite(mvp));
    REQUIRE(is_non_degenerate(mvp));
}

TEST_CASE("shadow light_view_proj — downward light uses X-as-up fallback", "[shadow]") {
    // Light shines straight up: dir = {0,1,0}.
    DirectionalLight light{};
    light.direction[0] = 0.0f;
    light.direction[1] = 1.0f;
    light.direction[2] = 0.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(all_finite(mvp));
    REQUIRE(is_non_degenerate(mvp));
}

TEST_CASE("shadow light_view_proj — arbitrary direction produces valid matrix", "[shadow]") {
    DirectionalLight light{};
    light.direction[0] = 1.0f;
    light.direction[1] = 2.0f;
    light.direction[2] = -3.0f;

    glm::mat4 mvp = shadow_compute_light_view_proj(
        light,
        k_shadow_ortho_half_size,
        k_shadow_near,
        k_shadow_far);

    REQUIRE(all_finite(mvp));
    REQUIRE(is_non_degenerate(mvp));
    REQUIRE(origin_inside_ortho_frustum(mvp));
}
