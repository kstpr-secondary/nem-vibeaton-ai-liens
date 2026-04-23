#include "camera.h"
#include "engine.h"
#include "math_utils.h"

#include <sokol_app.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

void camera_update() {
    auto& reg = engine_registry();
    auto view = reg.view<CameraActive, Transform, Camera>();

    for (auto e : view) {
        auto& t   = view.get<Transform>(e);
        auto& cam = view.get<Camera>(e);

        glm::mat4 v = make_view_matrix(t);

        float w = sapp_widthf();
        float h = sapp_heightf();
        float aspect = (h > 0.f) ? (w / h) : 1.f;
        glm::mat4 p = glm::perspective(glm::radians(cam.fov), aspect, cam.near_plane, cam.far_plane);

        RendererCamera rc{};
        // glm::value_ptr gives column-major float pointer — matches renderer layout
        const float* vp = glm::value_ptr(v);
        const float* pp = glm::value_ptr(p);
        for (int i = 0; i < 16; ++i) rc.view[i]       = vp[i];
        for (int i = 0; i < 16; ++i) rc.projection[i] = pp[i];

        renderer_set_camera(rc);
        return;  // only the first CameraActive entity is used
    }

    fprintf(stderr, "[ENGINE] camera_update: no CameraActive entity found\n");
}
