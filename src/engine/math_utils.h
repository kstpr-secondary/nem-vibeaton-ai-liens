#pragma once

#include "components.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// TRS model matrix from a Transform component.
inline glm::mat4 make_model_matrix(const Transform& t) {
    glm::mat4 T = glm::translate(glm::mat4(1.f), t.position);
    glm::mat4 R = glm::mat4_cast(t.rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.f), t.scale);
    return T * R * S;
}

// View matrix from a camera's Transform (inverse of the TRS model matrix).
// Camera scale is assumed to be {1,1,1}; only T and R are significant.
inline glm::mat4 make_view_matrix(const Transform& t) {
    return glm::inverse(make_model_matrix(t));
}
