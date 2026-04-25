#include "renderer.h"
#include "paths.h"
#include "sokol_app.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <cmath>

// ---------------------------------------------------------------------------
// App state — everything initialized once on first frame (GL context ready)
// ---------------------------------------------------------------------------

enum class ShadingOverride { Mixed, AllUnlit, AllLambertian };

struct AppState {
    bool               initialized = false;
    ShadingOverride    override    = ShadingOverride::Mixed;
    float              time        = 0.0f;

    RendererMeshHandle sphere;
    RendererMeshHandle cube;
    RendererMeshHandle transparent_sphere;
    RendererTextureHandle skybox_tex;
};

static AppState g_app;

static void on_input(const void* event, void*) {
    const sapp_event* e = static_cast<const sapp_event*>(event);
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN && e->key_code == SAPP_KEYCODE_L) {
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

static void on_frame(float dt, void*) {
    if (!g_app.initialized) {
        g_app.sphere        = renderer_make_sphere_mesh(1.0f, 16);
        g_app.cube          = renderer_make_cube_mesh(1.0f);
        g_app.transparent_sphere = renderer_make_sphere_mesh(0.5f, 8);

        const char* skybox_names[6] = {
            "px.png", "nx.png",
            "py.png", "ny.png",
            "pz.png", "nz.png"
        };

        // The cubemap face order for sg_image (SG_IMAGETYPE_CUBE) is:
        // +X, -X, +Y, -Y, +Z, -Z
        const void* faces[6] = {};
        int w = 0, h = 0, ch = 0;
        for (int i = 0; i < 6; ++i) {
            char path[512];
            snprintf(path, sizeof(path), "%s/skybox/%s", ASSET_ROOT, skybox_names[i]);
            printf("[info] loading skybox face %d: %s\n", i, path);
            unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
            if (data) {
                faces[i] = data;
                printf("[info]   loaded OK (%dx%d, %d ch)\n", w, h, ch);
            } else {
                printf("[warn]   FAILED to load: %s\n", path);
                faces[i] = nullptr;
            }
        }

        g_app.skybox_tex = renderer_upload_cubemap(faces, w, h, 4);
        for (int i = 0; i < 6; ++i) {
            if (faces[i]) stbi_image_free(const_cast<unsigned char*>(static_cast<const unsigned char*>(faces[i])));
        }

        if (renderer_handle_valid(g_app.skybox_tex)) {
            renderer_set_skybox(g_app.skybox_tex);
            printf("[info] skybox loaded (%d x %d)\n", w, h);
        } else {
            printf("[warn] skybox load failed\n");
        }

        g_app.initialized = true;
    }

    g_app.time += dt;

    // Orbiting camera: radius=22, height=7, period=12 s
    const float orbit_radius = 22.0f;
    const float orbit_speed  = 6.2831853f / 12.0f;
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

    // Directional light: dir={1,-1,-0.5}, white, intensity=1.5
    DirectionalLight light;
    light.direction[0] = 1.0f; light.direction[1] = -1.0f; light.direction[2] = -0.5f;
    light.color[0]     = 1.0f; light.color[1]     =  1.0f; light.color[2]     =  1.0f;
    light.intensity    = 1.5f;
    renderer_set_directional_light(light);

    // ---- Opaque geometry (spheres Lambertian, cubes Unlit) ----
    struct Item {
        RendererMeshHandle mesh;
        glm::vec3          pos;
        float              scale;
        float              rgb[3];
        bool               lambertian;
    };

    const Item items[] = {
        { g_app.sphere,      { -6.0f,  0.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 0.2f }, true  },
        { g_app.sphere,      { -3.0f,  2.0f,  0.0f }, 0.8f, { 0.2f, 1.0f, 0.2f }, true  },
        { g_app.sphere,      {  0.0f,  0.0f,  0.0f }, 1.2f, { 0.2f, 0.2f, 1.0f }, true  },
        { g_app.sphere,      {  3.0f, -2.0f,  0.0f }, 0.6f, { 1.0f, 1.0f, 0.2f }, true  },
        { g_app.sphere,      {  6.0f,  1.0f,  0.0f }, 1.0f, { 1.0f, 0.2f, 1.0f }, true  },
        { g_app.sphere,      { -4.0f,  4.0f, -3.0f }, 1.3f, { 0.2f, 1.0f, 1.0f }, true  },
        { g_app.cube,        { -5.0f, -2.0f,  2.0f }, 1.0f, { 0.9f, 0.5f, 0.1f }, false },
        { g_app.cube,        { -2.0f,  0.0f,  2.0f }, 0.8f, { 0.1f, 0.9f, 0.5f }, false },
        { g_app.cube,        {  1.0f,  2.0f,  2.0f }, 1.0f, { 0.5f, 0.1f, 0.9f }, false },
        { g_app.cube,        {  4.0f,  0.0f,  2.0f }, 1.5f, { 0.8f, 0.8f, 0.1f }, false },
        { g_app.cube,        {  7.0f, -1.0f,  2.0f }, 0.7f, { 0.1f, 0.8f, 0.8f }, false },
        { g_app.cube,        {  2.0f, -3.0f, -2.0f }, 1.1f, { 0.8f, 0.1f, 0.8f }, false },
    };

    for (const auto& item : items) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), item.pos),
                                      glm::vec3(item.scale));
        float transform[16];
        std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

        bool use_lam;
        switch (g_app.override) {
            case ShadingOverride::AllUnlit:       use_lam = false;             break;
            case ShadingOverride::AllLambertian:  use_lam = true;              break;
            default: /* Mixed */                  use_lam = item.lambertian;   break;
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

    // ---- Transparent geometry (small spheres with alpha < 1.0) ----
    struct TransItem {
        RendererMeshHandle mesh;
        glm::vec3          pos;
        float              scale;
        float              rgb[3];
        float              alpha;
    };

    const TransItem trans_items[] = {
        { g_app.transparent_sphere, { -1.0f,  3.0f, -4.0f }, 0.5f, { 1.0f, 0.3f, 0.8f }, 0.45f },
        { g_app.transparent_sphere, {  2.0f, -1.0f, -5.0f }, 0.4f, { 0.3f, 0.8f, 1.0f }, 0.55f },
        { g_app.transparent_sphere, {  5.0f,  2.0f, -3.0f }, 0.6f, { 1.0f, 0.7f, 0.2f }, 0.40f },
        { g_app.transparent_sphere, { -7.0f, -1.0f, -2.0f }, 0.35f, { 0.8f, 1.0f, 0.3f }, 0.50f },
    };

    for (const auto& item : trans_items) {
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), item.pos),
                                      glm::vec3(item.scale));
        float transform[16];
        std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

        float rgba[4] = { item.rgb[0], item.rgb[1], item.rgb[2], item.alpha };
        Material mat = renderer_make_unlit_material(rgba);
        renderer_enqueue_draw(item.mesh, transform, mat);
    }

    // ---- Laser line-quads (3 beams with alpha=0.6) ----
    struct LaserBeam {
        float p0[3];
        float p1[3];
        float width;
        float color[4];
    };

    const LaserBeam lasers[] = {
        // Beam 1: from top-left to bottom-right, passing through origin area
        { { -8.0f, 6.0f, -2.0f }, { 8.0f, -4.0f,  2.0f }, 0.08f, { 1.0f, 0.1f, 0.3f, 0.6f } },
        // Beam 2: horizontal sweep
        { { -7.0f, 0.0f,  0.0f }, { 7.0f, 0.0f,  0.0f }, 0.10f, { 0.1f, 0.9f, 0.8f, 0.6f } },
        // Beam 3: vertical beam
        { {  0.0f, -6.0f, -3.0f }, { 0.0f, 6.0f, -3.0f }, 0.06f, { 1.0f, 0.9f, 0.1f, 0.6f } },
    };

    for (const auto& laser : lasers) {
        renderer_enqueue_line_quad(laser.p0, laser.p1, laser.width, laser.color);
    }

    // ---- ImGui HUD: FPS + draw count ----
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
    ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
    {
        float fps = ImGui::GetIO().Framerate;
        int   draw_count = renderer_get_draw_count();
        char buf[256];
        snprintf(buf, sizeof(buf), "FPS: %.1f\nDraws: %d", fps, draw_count);
        ImGui::TextUnformatted(buf);
    }
    ImGui::End();

    renderer_end_frame();
}

int main() {
    RendererConfig cfg;
    cfg.title   = "R-M3 MVP [L=Mixed/AllUnlit/AllLambertian]";
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
