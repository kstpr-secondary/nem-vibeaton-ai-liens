#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "math_utils.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

using Catch::Approx;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool mat4_approx_eq(const glm::mat4& a, const glm::mat4& b, float eps = 1e-5f) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (std::fabs(a[c][r] - b[c][r]) > eps) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Model matrix — TRS composition
// ---------------------------------------------------------------------------

TEST_CASE("identity transform produces identity model matrix", "[math]") {
    Transform t{};
    REQUIRE(make_model_matrix(t) == glm::mat4(1.f));
}

TEST_CASE("translation only — correct column-3 in model matrix", "[math]") {
    Transform t{};
    t.position = {3.f, -2.f, 7.f};
    glm::mat4 m = make_model_matrix(t);
    // GLM mat4 is column-major: m[col][row]
    REQUIRE(m[3][0] == Approx(3.f));
    REQUIRE(m[3][1] == Approx(-2.f));
    REQUIRE(m[3][2] == Approx(7.f));
    REQUIRE(m[3][3] == Approx(1.f));
    // Rotation/scale block is identity
    REQUIRE(m[0][0] == Approx(1.f));
    REQUIRE(m[1][1] == Approx(1.f));
    REQUIRE(m[2][2] == Approx(1.f));
}

TEST_CASE("scale only — correct diagonal in model matrix", "[math]") {
    Transform t{};
    t.scale = {2.f, 3.f, 4.f};
    glm::mat4 m = make_model_matrix(t);
    REQUIRE(m[0][0] == Approx(2.f));
    REQUIRE(m[1][1] == Approx(3.f));
    REQUIRE(m[2][2] == Approx(4.f));
    // No translation
    REQUIRE(m[3][0] == Approx(0.f));
    REQUIRE(m[3][1] == Approx(0.f));
    REQUIRE(m[3][2] == Approx(0.f));
}

TEST_CASE("identity quaternion produces identity rotation block", "[math]") {
    Transform t{};
    // rotation is identity quat by default
    glm::mat4 m = make_model_matrix(t);
    REQUIRE(m[0][0] == Approx(1.f));
    REQUIRE(m[1][1] == Approx(1.f));
    REQUIRE(m[2][2] == Approx(1.f));
    REQUIRE(m[0][1] == Approx(0.f));
    REQUIRE(m[0][2] == Approx(0.f));
    REQUIRE(m[1][0] == Approx(0.f));
}

TEST_CASE("90 deg rotation around Y axis", "[math]") {
    Transform t{};
    // 90 degrees around Y: cos(45°), sin(45°)*Y = (cos45, 0, sin45, 0) in wxyz
    float half = glm::radians(45.f);
    t.rotation = glm::quat(std::cos(half), 0.f, std::sin(half), 0.f);
    glm::mat4 m = make_model_matrix(t);
    // After 90° Y rotation: X→-Z, Z→X
    // m[0] (what X becomes) should point to -Z: (0, 0, -1, 0)
    REQUIRE(m[0][0] == Approx(0.f).margin(1e-5f));
    REQUIRE(m[0][2] == Approx(-1.f).margin(1e-5f));
    // m[2] (what Z becomes) should point to X: (1, 0, 0, 0)
    REQUIRE(m[2][0] == Approx(1.f).margin(1e-5f));
    REQUIRE(m[2][2] == Approx(0.f).margin(1e-5f));
}

TEST_CASE("TRS composition: translate + scale, no rotation", "[math]") {
    Transform t{};
    t.position = {1.f, 2.f, 3.f};
    t.scale    = {2.f, 2.f, 2.f};
    glm::mat4 m = make_model_matrix(t);
    // Scale in diagonal
    REQUIRE(m[0][0] == Approx(2.f));
    REQUIRE(m[1][1] == Approx(2.f));
    REQUIRE(m[2][2] == Approx(2.f));
    // Translation preserved
    REQUIRE(m[3][0] == Approx(1.f));
    REQUIRE(m[3][1] == Approx(2.f));
    REQUIRE(m[3][2] == Approx(3.f));
}

// ---------------------------------------------------------------------------
// View matrix — inverse of TRS
// ---------------------------------------------------------------------------

TEST_CASE("identity transform produces identity view matrix", "[math]") {
    Transform t{};
    REQUIRE(mat4_approx_eq(make_view_matrix(t), glm::mat4(1.f)));
}

TEST_CASE("camera at z=5 — view matrix translates by -5 on Z", "[math]") {
    Transform t{};
    t.position = {0.f, 0.f, 5.f};
    glm::mat4 v = make_view_matrix(t);
    REQUIRE(v[3][2] == Approx(-5.f));
    REQUIRE(v[3][0] == Approx(0.f));
    REQUIRE(v[3][1] == Approx(0.f));
    REQUIRE(v[3][3] == Approx(1.f));
}

TEST_CASE("camera at arbitrary position — view translation is negated", "[math]") {
    Transform t{};
    t.position = {3.f, -1.f, 7.f};
    glm::mat4 v = make_view_matrix(t);
    REQUIRE(v[3][0] == Approx(-3.f));
    REQUIRE(v[3][1] == Approx(1.f));
    REQUIRE(v[3][2] == Approx(-7.f));
}

TEST_CASE("model and view are inverses of each other", "[math]") {
    Transform t{};
    t.position = {1.f, 2.f, 3.f};
    // 45° around Y
    float half = glm::radians(22.5f);
    t.rotation = glm::quat(std::cos(half), 0.f, std::sin(half), 0.f);

    glm::mat4 m = make_model_matrix(t);
    glm::mat4 v = make_view_matrix(t);
    glm::mat4 product = v * m;  // should be identity
    REQUIRE(mat4_approx_eq(product, glm::mat4(1.f)));
}
