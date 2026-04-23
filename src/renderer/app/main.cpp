#include "renderer.h"
#include "sokol_app.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>
#include <cmath>

// ---------------------------------------------------------------------------
// App state — meshes uploaded once on first frame (GL context must be ready)
// ---------------------------------------------------------------------------

enum class ShadingOverride { Mixed, AllUnlit, AllLambertian };

struct AppState {
    bool               initialized = false;
    ShadingOverride    override    = ShadingOverride::Mixed; // L key cycles
    float              time        = 0.0f;                   // accumulated seconds
    RendererMeshHandle sphere;
    RendererMeshHandle cube;
};

static AppState g_app;

static void on_input(const void* event, void* /*user_data*/) {
    const sapp_event* e = static_cast<const sapp_event*>(event);
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN && e->key_code == SAPP_KEYCODE_L) {
        // Cycle: Mixed → AllUnlit → AllLambertian → Mixed
        switch (g_app.override) {
            case ShadingOverride::Mixed:
                g_app.override = ShadingOverride::AllUnlit;
                printf("[debug] shading override: AllUnlit\n");
                break;
            case ShadingOverride::AllUnlit:
                g_app.override = ShadingOverride::AllLambertian;
                printf("[debug] shading override: AllLambertian\n");
                break;
            case ShadingOverride::AllLambertian:
                g_app.override = ShadingOverride::Mixed;
                printf("[debug] shading override: Mixed\n");
                break;
        }
    }
}

static void on_frame(float dt, void* /*user_data*/) {
    if (!g_app.initialized) {
        g_app.sphere      = renderer_make_sphere_mesh(1.0f, 16);
        g_app.cube        = renderer_make_cube_mesh(1.0f);
        g_app.initialized = true;
    }

    g_app.time += dt;

    // Orbiting camera: radius=22, height=7, period=12 s
    const float orbit_radius = 22.0f;
    const float orbit_speed  = 6.2831853f / 12.0f; // 2π / period
    float angle = g_app.time * orbit_speed;
    glm::vec3 eye(orbit_radius * cosf(angle), 7.0f, orbit_radius * sinf(angle));

    glm::mat4 view = glm::lookAt(eye,
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      1280.0f / 720.0f,
                                      0.1f, 200.0f);
    RendererCamera cam;
    std::memcpy(cam.view,       glm::value_ptr(view), sizeof(cam.view));
    std::memcpy(cam.projection, glm::value_ptr(proj), sizeof(cam.projection));

    renderer_begin_frame();
    renderer_set_camera(cam);

    // Directional light: dir={1,-1,-0.5}, white, intensity=1.5 (R-033 spec)
    DirectionalLight light;
    light.direction[0] = 1.0f; light.direction[1] = -1.0f; light.direction[2] = -0.5f;
    light.color[0]     = 1.0f; light.color[1]     =  1.0f; light.color[2]     =  1.0f;
    light.intensity    = 1.5f;
    renderer_set_directional_light(light);

    // Mixed scene: spheres default Lambertian, cubes default Unlit.
    // 'true' = prefers Lambertian when override == Mixed.
    struct Item {
        RendererMeshHandle mesh;
        glm::vec3          pos;
        float              scale;
        float              rgb[3];
        bool               lambertian; // base shading preference
    };

    const Item items[] = {
        { g_app.sphere, { -6.0f,  0.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 0.2f }, true  },
        { g_app.sphere, { -3.0f,  2.0f,  0.0f }, 0.8f, { 0.2f, 1.0f, 0.2f }, true  },
        { g_app.sphere, {  0.0f,  0.0f,  0.0f }, 1.2f, { 0.2f, 0.2f, 1.0f }, true  },
        { g_app.sphere, {  3.0f, -2.0f,  0.0f }, 0.6f, { 1.0f, 1.0f, 0.2f }, true  },
        { g_app.sphere, {  6.0f,  1.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 1.0f }, true  },
        { g_app.sphere, { -4.0f,  4.0f, -3.0f }, 1.3f, { 0.2f, 1.0f, 1.0f }, true  },
        { g_app.cube,   { -5.0f, -2.0f,  2.0f }, 1.0f, { 0.9f, 0.5f, 0.1f }, false },
        { g_app.cube,   { -2.0f,  0.0f,  2.0f }, 0.8f, { 0.1f, 0.9f, 0.5f }, false },
        { g_app.cube,   {  1.0f,  2.0f,  2.0f }, 1.0f, { 0.5f, 0.1f, 0.9f }, false },
        { g_app.cube,   {  4.0f,  0.0f,  2.0f }, 1.5f, { 0.8f, 0.8f, 0.1f }, false },
        { g_app.cube,   {  7.0f, -1.0f,  2.0f }, 0.7f, { 0.1f, 0.8f, 0.8f }, false },
        { g_app.cube,   {  2.0f, -3.0f, -2.0f }, 1.1f, { 0.8f, 0.1f, 0.8f }, false },
    };

    for (const auto& item : items) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), item.pos),
                                     glm::vec3(item.scale));
        float transform[16];
        std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

        bool use_lam;
        switch (g_app.override) {
            case ShadingOverride::AllUnlit:       use_lam = false;            break;
            case ShadingOverride::AllLambertian:  use_lam = true;             break;
            default: /* Mixed */                  use_lam = item.lambertian;  break;
        }

        Material mat;
        if (use_lam) {
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
    cfg.title   = "R-M2 Lambertian [L=Mixed/AllUnlit/AllLambertian]";
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
