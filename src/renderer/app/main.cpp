#include "renderer.h"
#include "sokol_app.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>

static void on_input(const void* event, void* /*user_data*/) {
    const sapp_event* e = static_cast<const sapp_event*>(event);
    printf("[input] event type: %d\n", static_cast<int>(e->type));
}

// ---------------------------------------------------------------------------
// App state — meshes uploaded once on first frame (GL context must be ready)
// ---------------------------------------------------------------------------

struct AppState {
    bool               initialized = false;
    RendererMeshHandle sphere;
    RendererMeshHandle cube;
};

static AppState g_app;

static void on_frame(float /*dt*/, void* /*user_data*/) {
    if (!g_app.initialized) {
        g_app.sphere      = renderer_make_sphere_mesh(1.0f, 16);
        g_app.cube        = renderer_make_cube_mesh(1.0f);
        g_app.initialized = true;
    }

    // Fixed perspective camera: 45° FOV, 16:9, near=0.1, far=100
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 6.0f, 20.0f),
                                 glm::vec3(0.0f, 0.0f,  0.0f),
                                 glm::vec3(0.0f, 1.0f,  0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      1280.0f / 720.0f,
                                      0.1f, 100.0f);
    RendererCamera cam;
    std::memcpy(cam.view,       glm::value_ptr(view), sizeof(cam.view));
    std::memcpy(cam.projection, glm::value_ptr(proj), sizeof(cam.projection));

    renderer_begin_frame();
    renderer_set_camera(cam);

    // ≥10 draw calls: 6 spheres + 6 cubes at varied positions/scales/colors
    struct Item {
        RendererMeshHandle mesh;
        glm::vec3          pos;
        float              scale;
        float              rgba[4];
    };

    const Item items[] = {
        { g_app.sphere, { -6.0f,  0.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 0.2f, 1.0f } },
        { g_app.sphere, { -3.0f,  2.0f,  0.0f }, 0.8f, { 0.2f, 1.0f, 0.2f, 1.0f } },
        { g_app.sphere, {  0.0f,  0.0f,  0.0f }, 1.2f, { 0.2f, 0.2f, 1.0f, 1.0f } },
        { g_app.sphere, {  3.0f, -2.0f,  0.0f }, 0.6f, { 1.0f, 1.0f, 0.2f, 1.0f } },
        { g_app.sphere, {  6.0f,  1.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 1.0f, 1.0f } },
        { g_app.sphere, { -4.0f,  4.0f, -3.0f }, 1.3f, { 0.2f, 1.0f, 1.0f, 1.0f } },
        { g_app.cube,   { -5.0f, -2.0f,  2.0f }, 1.0f, { 0.9f, 0.5f, 0.1f, 1.0f } },
        { g_app.cube,   { -2.0f,  0.0f,  2.0f }, 0.8f, { 0.1f, 0.9f, 0.5f, 1.0f } },
        { g_app.cube,   {  1.0f,  2.0f,  2.0f }, 1.0f, { 0.5f, 0.1f, 0.9f, 1.0f } },
        { g_app.cube,   {  4.0f,  0.0f,  2.0f }, 1.5f, { 0.8f, 0.8f, 0.1f, 1.0f } },
        { g_app.cube,   {  7.0f, -1.0f,  2.0f }, 0.7f, { 0.1f, 0.8f, 0.8f, 1.0f } },
        { g_app.cube,   {  2.0f, -3.0f, -2.0f }, 1.1f, { 0.8f, 0.1f, 0.8f, 1.0f } },
    };

    for (const auto& item : items) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), item.pos),
                                     glm::vec3(item.scale));
        float transform[16];
        std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

        Material mat = renderer_make_unlit_material(item.rgba);
        renderer_enqueue_draw(item.mesh, transform, mat);
    }

    renderer_end_frame();
}

int main() {
    RendererConfig cfg;
    cfg.title   = "R-M1 Unlit Forward";
    cfg.clear_r = 0.05f;
    cfg.clear_g = 0.05f;
    cfg.clear_b = 0.10f;
    cfg.clear_a = 1.0f;

    renderer_init(cfg);
    renderer_set_frame_callback(on_frame, nullptr);
    renderer_set_input_callback(on_input, nullptr);
    renderer_run();
    return 0;
}
