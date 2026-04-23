#include "renderer.h"
#include "sokol_app.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// App state — meshes uploaded once on first frame (GL context must be ready)
// ---------------------------------------------------------------------------

struct AppState {
    bool               initialized    = false;
    bool               use_lambertian = false; // toggled by L key
    RendererMeshHandle sphere;
    RendererMeshHandle cube;
};

static AppState g_app;

static void on_input(const void* event, void* /*user_data*/) {
    const sapp_event* e = static_cast<const sapp_event*>(event);
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN && e->key_code == SAPP_KEYCODE_L) {
        g_app.use_lambertian = !g_app.use_lambertian;
        printf("[debug] shading mode: %s\n",
               g_app.use_lambertian ? "Lambertian" : "Unlit");
    }
}

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

    // Set directional light (used by Lambertian pass; stored every frame so it's
    // always valid if the user toggles mid-run)
    DirectionalLight light;
    light.direction[0] = 1.0f; light.direction[1] = -1.0f; light.direction[2] = -0.5f;
    light.color[0]     = 1.0f; light.color[1]     =  1.0f; light.color[2]     =  1.0f;
    light.intensity    = 1.5f;
    renderer_set_directional_light(light);

    // ≥10 draw calls: 6 spheres + 6 cubes at varied positions/scales/colors
    struct Item {
        RendererMeshHandle mesh;
        glm::vec3          pos;
        float              scale;
        float              rgb[3];
    };

    const Item items[] = {
        { g_app.sphere, { -6.0f,  0.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 0.2f } },
        { g_app.sphere, { -3.0f,  2.0f,  0.0f }, 0.8f, { 0.2f, 1.0f, 0.2f } },
        { g_app.sphere, {  0.0f,  0.0f,  0.0f }, 1.2f, { 0.2f, 0.2f, 1.0f } },
        { g_app.sphere, {  3.0f, -2.0f,  0.0f }, 0.6f, { 1.0f, 1.0f, 0.2f } },
        { g_app.sphere, {  6.0f,  1.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 1.0f } },
        { g_app.sphere, { -4.0f,  4.0f, -3.0f }, 1.3f, { 0.2f, 1.0f, 1.0f } },
        { g_app.cube,   { -5.0f, -2.0f,  2.0f }, 1.0f, { 0.9f, 0.5f, 0.1f } },
        { g_app.cube,   { -2.0f,  0.0f,  2.0f }, 0.8f, { 0.1f, 0.9f, 0.5f } },
        { g_app.cube,   {  1.0f,  2.0f,  2.0f }, 1.0f, { 0.5f, 0.1f, 0.9f } },
        { g_app.cube,   {  4.0f,  0.0f,  2.0f }, 1.5f, { 0.8f, 0.8f, 0.1f } },
        { g_app.cube,   {  7.0f, -1.0f,  2.0f }, 0.7f, { 0.1f, 0.8f, 0.8f } },
        { g_app.cube,   {  2.0f, -3.0f, -2.0f }, 1.1f, { 0.8f, 0.1f, 0.8f } },
    };

    for (const auto& item : items) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), item.pos),
                                     glm::vec3(item.scale));
        float transform[16];
        std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

        Material mat;
        if (g_app.use_lambertian) {
            mat = renderer_make_lambertian_material(item.rgb);
        } else {
            float rgba[4] = { item.rgb[0], item.rgb[1], item.rgb[2], 1.0f };
            mat = renderer_make_unlit_material(rgba);
        }
        renderer_enqueue_draw(item.mesh, transform, mat);
    }

    renderer_end_frame();
}

int main() {
    RendererConfig cfg;
    cfg.title   = "R-M1/M2 Debug [L=toggle Unlit/Lambertian]";
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
