#include "debug_draw.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

bool debug_draw_compute_billboard(
    LineQuadCommand*  cmd,
    const float       p0[3],
    const float       p1[3],
    float             width,
    const float       color[4],
    const float       view[16])
{
    glm::vec3 a(p0[0], p0[1], p0[2]);
    glm::vec3 b(p1[0], p1[1], p1[2]);

    glm::vec3 dir = b - a;
    float seg_len = glm::length(dir);
    if (seg_len < 1e-6f) return false;
    dir /= seg_len;  // normalize

    // Extract camera world-space position from inverse of view matrix.
    // For V = [R|t], V^-1 last column = world-space camera position.
    glm::mat4 view_mat = glm::make_mat4(view);  // column-major
    glm::mat4 view_inv = glm::inverse(view_mat);
    glm::vec3 cam_pos  = glm::vec3(view_inv[3]);  // last column xyz

    glm::vec3 to_cam = cam_pos - a;
    glm::vec3 cross_result = glm::cross(dir, to_cam);
    float cross_len = glm::length(cross_result);
    if (cross_len < 1e-6f) {
        // Camera is co-linear with segment — fallback: use world up or right
        glm::vec3 fallback = (glm::abs(dir.y) < 0.9f)
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);
        cross_result = glm::cross(dir, fallback);
        cross_len    = glm::length(cross_result);
        if (cross_len < 1e-6f) return false;
    }
    glm::vec3 right = (cross_result / cross_len) * (width * 0.5f);

    // 4 billboard corners (counter-clockwise winding):
    //   v0 = p0 - right   v1 = p0 + right
    //   v3 = p1 - right   v2 = p1 + right
    // Triangle indices: (0,1,2)  (0,2,3)
    glm::vec3 corners[4] = {
        a - right,  // 0
        a + right,  // 1
        b + right,  // 2
        b - right,  // 3
    };

    for (int i = 0; i < 4; ++i) {
        cmd->verts[i].position[0] = corners[i].x;
        cmd->verts[i].position[1] = corners[i].y;
        cmd->verts[i].position[2] = corners[i].z;
        cmd->verts[i].color[0]    = color ? color[0] : 1.0f;
        cmd->verts[i].color[1]    = color ? color[1] : 1.0f;
        cmd->verts[i].color[2]    = color ? color[2] : 1.0f;
        cmd->verts[i].color[3]    = color ? color[3] : 1.0f;
    }

    return true;
}

