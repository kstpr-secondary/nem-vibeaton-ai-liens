#include "renderer.h"
#include "paths.h"
#include "sokol_app.h"

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

// ---------------------------------------------------------------------------
// App state — everything initialized once on first frame (GL context ready)
// ---------------------------------------------------------------------------

enum class ShadingOverride { Mixed, AllUnlit, AllLambertian };

struct AppState {
    bool               initialized = false;
    ShadingOverride    override    = ShadingOverride::Mixed;
    float              time        = 0.0f;
    bool               culling_on  = true;

    // Meshes
    RendererMeshHandle sphere_8;      // low-poly (transparent)
    RendererMeshHandle sphere_12;     // medium poly (Lambertian / Blinn-Phong)
    RendererMeshHandle sphere_16;     // high poly (Blinn-Phong textured)
    RendererMeshHandle cube;

    // Textures
    RendererTextureHandle skybox_tex;
    RendererTextureHandle textures[8];  // 8 texture files
    int                   tex_count;
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
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN && e->key_code == SAPP_KEYCODE_C) {
        g_app.culling_on = !g_app.culling_on;
        printf("[debug] frustum culling: %s\n", g_app.culling_on ? "ON" : "OFF");
    }
}

// ---------------------------------------------------------------------------
// Texture file list
// ---------------------------------------------------------------------------

static const char* tex_files[] = {
    "stone1.jpg",
    "stone2.jpg",
    "concrete.jpg",
    "wood.jpg",
    "sand.jpg",
    "gold_sparkly.jpg",
    "round_stone.jpg",
    "asteroid.png",
};

// ---------------------------------------------------------------------------
// Scene generation helpers
// ---------------------------------------------------------------------------

static void spawn_shape(RendererMeshHandle mesh, const glm::vec3& pos, float scale,
                        const float rgb[3], float alpha, ShadingModel shading,
                        RendererTextureHandle tex = {}) {
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(scale));
    float transform[16];
    std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

    Material mat;
    if (shading == ShadingModel::Unlit) {
        mat = renderer_make_unlit_material(rgb);
    } else if (shading == ShadingModel::Lambertian) {
        mat = renderer_make_lambertian_material(rgb);
    } else { // BlinnPhong
        mat = renderer_make_blinnphong_material(rgb, 64.0f, tex);
    }
    mat.alpha = alpha;
    renderer_enqueue_draw(mesh, transform, mat);
}

// Position on a ring at given radius, with vertical offset
static glm::vec3 ring_pos(float radius, float angle, float y_offset) {
    return glm::vec3(radius * cosf(angle), y_offset, radius * sinf(angle));
}

static void on_frame(float dt, void*) {
    if (!g_app.initialized) {
        // ---- Meshes ----
        g_app.sphere_8  = renderer_make_sphere_mesh(0.5f, 8);   // low-poly, transparent
        g_app.sphere_12 = renderer_make_sphere_mesh(1.0f, 12);  // medium poly
        g_app.sphere_16 = renderer_make_sphere_mesh(1.0f, 16);  // high poly, textured
        g_app.cube      = renderer_make_cube_mesh(1.0f);

        // ---- Skybox ----
        const char* skybox_names[6] = {
            "px.png", "nx.png",
            "py.png", "ny.png",
            "pz.png", "nz.png"
        };
        const void* faces[6] = {};
        int w = 0, h = 0, ch = 0;
        for (int i = 0; i < 6; ++i) {
            char path[512];
            snprintf(path, sizeof(path), "%s/skybox/%s", ASSET_ROOT, skybox_names[i]);
            unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
            if (data) { faces[i] = data; }
        }
        g_app.skybox_tex = renderer_upload_cubemap(faces, w, h, 4);
        for (int i = 0; i < 6; ++i) {
            if (faces[i]) stbi_image_free(const_cast<unsigned char*>(static_cast<const unsigned char*>(faces[i])));
        }
        if (renderer_handle_valid(g_app.skybox_tex)) {
            renderer_set_skybox(g_app.skybox_tex);
        }

        // ---- Texture files ----
        g_app.tex_count = 0;
        for (int i = 0; i < 8 && g_app.tex_count < 8; ++i) {
            char path[512];
            snprintf(path, sizeof(path), "%s/textures/%s", ASSET_ROOT, tex_files[i]);
            g_app.textures[g_app.tex_count] = renderer_upload_texture_from_file(path);
            if (renderer_handle_valid(g_app.textures[g_app.tex_count])) {
                printf("[info] loaded texture[%d]: %s\n", g_app.tex_count, tex_files[i]);
                ++g_app.tex_count;
            } else {
                printf("[warn] failed to load: %s\n", tex_files[i]);
            }
        }

        g_app.initialized = true;
    }

    g_app.time += dt;

    // ---- Camera: orbit radius=30, height=10, period=20 s ----
    const float orbit_radius = 30.0f;
    const float orbit_speed  = 6.2831853f / 20.0f;
    float angle = g_app.time * orbit_speed;
    glm::vec3 eye(orbit_radius * cosf(angle), 10.0f, orbit_radius * sinf(angle));

    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
                                       1280.0f / 720.0f,
                                       0.1f, 300.0f);
    RendererCamera cam;
    std::memcpy(cam.view,       glm::value_ptr(view), sizeof(cam.view));
    std::memcpy(cam.projection, glm::value_ptr(proj), sizeof(cam.projection));

    renderer_begin_frame();
    renderer_set_camera(cam);
    renderer_set_culling_enabled(g_app.culling_on);

    // Directional light
    DirectionalLight light;
    light.direction[0] = 1.0f; light.direction[1] =  2.0f; light.direction[2] = -0.5f;
    light.color[0]     = 1.0f; light.color[1]     =  1.0f; light.color[2]     =  1.0f;
    light.intensity    = 1.8f;
    renderer_set_directional_light(light);

    // =====================================================================
    // SECTION 1: Center display — 8 showcase shapes (1 per material)
    // =====================================================================
    {
        const float center_y = 0.0f;

        // 1. Unlit red cube (center)
        {
            float rgb[3] = {1.0f, 0.2f, 0.2f};
            spawn_shape(g_app.cube, glm::vec3(0.0f, center_y, 0.0f), 1.5f, rgb, 1.0f, ShadingModel::Unlit);
        }

        // 2. Lambertian green sphere (left)
        {
            float rgb[3] = {0.2f, 1.0f, 0.3f};
            spawn_shape(g_app.sphere_12, glm::vec3(-5.0f, center_y, 0.0f), 1.0f, rgb, 1.0f, ShadingModel::Lambertian);
        }

        // 3. Lambertian blue sphere (right)
        {
            float rgb[3] = {0.2f, 0.4f, 1.0f};
            spawn_shape(g_app.sphere_12, glm::vec3(5.0f, center_y, 0.0f), 1.0f, rgb, 1.0f, ShadingModel::Lambertian);
        }

        // 4. Blinn-Phong textured sphere (front-left) — uses first available texture
        {
            float rgb[3] = {1.0f, 1.0f, 1.0f};
            RendererTextureHandle tex = {};
            if (g_app.tex_count > 0) tex = g_app.textures[0];
            spawn_shape(g_app.sphere_16, glm::vec3(-2.5f, center_y, 4.0f), 1.2f, rgb, 1.0f, ShadingModel::BlinnPhong, tex);
        }

        // 5. Blinn-Phong textured sphere (front-right) — second texture, high shininess
        {
            float rgb[3] = {1.0f, 1.0f, 1.0f};
            RendererTextureHandle tex = {};
            if (g_app.tex_count > 1) tex = g_app.textures[1];
            spawn_shape(g_app.sphere_16, glm::vec3(2.5f, center_y, 4.0f), 1.2f, rgb, 1.0f, ShadingModel::BlinnPhong, tex);
        }

        // 6. Blinn-Phong untextured gold sphere (back-left) — very shiny
        {
            float rgb[3] = {1.0f, 0.85f, 0.2f};
            spawn_shape(g_app.sphere_16, glm::vec3(-4.0f, center_y, -5.0f), 1.1f, rgb, 1.0f, ShadingModel::BlinnPhong, {});
        }

        // 7. Transparent red sphere (floating above)
        {
            float rgb[3] = {1.0f, 0.3f, 0.3f};
            spawn_shape(g_app.sphere_8, glm::vec3(0.0f, center_y + 4.0f, 2.0f), 0.6f, rgb, 0.35f, ShadingModel::Unlit);
        }

        // 8. Transparent blue sphere (floating above)
        {
            float rgb[3] = {0.3f, 0.6f, 1.0f};
            spawn_shape(g_app.sphere_8, glm::vec3(0.0f, center_y + 5.5f, -2.0f), 0.5f, rgb, 0.45f, ShadingModel::Unlit);
        }
    }

    // =====================================================================
    // SECTION 2: Ring of Lambertian spheres — 36 shapes at medium distance
    // =====================================================================
    {
        const float radius = 18.0f;
        int count = 0;
        for (int i = 0; i < 36; ++i) {
            float a = (float)i / 36.0f * 6.2831853f;
            float y = (static_cast<float>(i % 3) - 1.0f) * 2.5f;

            // Color varies by index
            float r = 0.3f + 0.7f * static_cast<float>((i * 7) % 11) / 11.0f;
            float g = 0.3f + 0.7f * static_cast<float>((i * 13) % 11) / 11.0f;
            float b = 0.3f + 0.7f * static_cast<float>((i * 19) % 11) / 11.0f;
            float rgb[3] = {r, g, b};

            spawn_shape(g_app.sphere_12, ring_pos(radius, a, y), 0.6f + (i % 5) * 0.1f,
                        rgb, 1.0f, ShadingModel::Lambertian);
            ++count;
        }
        printf("[info] Section 2: %d Lambertian spheres at radius %.1f\n", count, radius);
    }

    // =====================================================================
    // SECTION 3: Ring of Blinn-Phong textured spheres — 24 shapes far out
    // =====================================================================
    {
        const float radius = 25.0f;
        int count = 0;
        for (int i = 0; i < 24; ++i) {
            float a = (float)i / 24.0f * 6.2831853f;
            float y = (static_cast<float>(i % 2)) * 3.0f;

            // Cycle through textures
            RendererTextureHandle tex = {};
            if (g_app.tex_count > 0) {
                tex = g_app.textures[i % g_app.tex_count];
            }

            float rgb[3] = {1.0f, 1.0f, 1.0f}; // white base color — texture drives appearance

            spawn_shape(g_app.sphere_16, ring_pos(radius, a, y), 0.8f,
                        rgb, 1.0f, ShadingModel::BlinnPhong, tex);
            ++count;
        }
        printf("[info] Section 3: %d Blinn-Phong textured spheres at radius %.1f\n", count, radius);
    }

    // =====================================================================
    // SECTION 4: Ring of transparent spheres — 20 shapes
    // =====================================================================
    {
        const float radius = 12.0f;
        int count = 0;
        for (int i = 0; i < 20; ++i) {
            float a = (float)i / 20.0f * 6.2831853f;
            float y = static_cast<float>(i % 4) * 1.5f - 2.0f;

            // Varying alpha: 0.15 to 0.55
            float alpha = 0.15f + static_cast<float>(i % 8) * 0.05f;

            // Rainbow-ish colors
            float hue = (float)i / 20.0f;
            float r = sinf(hue * 6.28f);
            float g = sinf(hue * 6.28f + 2.094f);
            float b = sinf(hue * 6.28f + 4.188f);
            float rgb[3] = {fabsf(r), fabsf(g), fabsf(b)};

            spawn_shape(g_app.sphere_8, ring_pos(radius, a, y), 0.4f + (i % 3) * 0.1f,
                        rgb, alpha, ShadingModel::Unlit);
            ++count;
        }
        printf("[info] Section 4: %d transparent spheres at radius %.1f\n", count, radius);
    }

    // =====================================================================
    // SECTION 5: Distant cubes — 20 unlit shapes at far range (culling test)
    // =====================================================================
    {
        const float radius = 40.0f;
        int count = 0;
        for (int i = 0; i < 20; ++i) {
            float a = (float)i / 20.0f * 6.2831853f;
            float y = static_cast<float>(i % 3 - 1) * 4.0f;

            // Muted earthy colors
            float v = 0.3f + static_cast<float>((i * 3) % 7) * 0.1f;
            float rgb[3] = {v, v * 0.8f, v * 0.6f};

            spawn_shape(g_app.cube, ring_pos(radius, a, y), 0.8f + (i % 4) * 0.2f,
                        rgb, 1.0f, ShadingModel::Unlit);
            ++count;
        }
        printf("[info] Section 5: %d unlit cubes at radius %.1f\n", count, radius);
    }

    // =====================================================================
    // SECTION 6: Scatter of mixed shapes — ~50 random positions
    // =====================================================================
    {
        srand(12345);
        int count = 0;

        for (int i = 0; i < 50; ++i) {
            float r = 8.0f + static_cast<float>(rand() % 35) * 0.8f;
            float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 6.2831853f;
            float phi = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.5f;

            glm::vec3 pos(
                r * cosf(phi) * cosf(theta),
                r * sinf(phi) * 0.6f,
                r * cosf(phi) * sinf(theta)
            );

            float scale = 0.3f + static_cast<float>(rand() % 25) * 0.1f;

            // Pick material type
            int type = rand() % 5;
            float r_c = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float g_c = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float b_c = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float rgb[3] = {r_c, g_c, b_c};

            if (type < 2) {
                // Lambertian sphere
                spawn_shape(g_app.sphere_12, pos, scale, rgb, 1.0f, ShadingModel::Lambertian);
            } else if (type == 2) {
                // Unlit cube
                spawn_shape(g_app.cube, pos, scale * 0.8f, rgb, 1.0f, ShadingModel::Unlit);
            } else if (type == 3) {
                // Transparent sphere
                float alpha = 0.2f + static_cast<float>(rand() % 30) * 0.02f;
                spawn_shape(g_app.sphere_8, pos, scale * 0.6f, rgb, alpha, ShadingModel::Unlit);
            } else {
                // Blinn-Phong sphere (untextured)
                float shininess = 32.0f + static_cast<float>(rand() % 4) * 64.0f;
                glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(scale));
                float transform[16];
                std::memcpy(transform, glm::value_ptr(model), sizeof(transform));

                Material mat = renderer_make_blinnphong_material(rgb, shininess, {});
                renderer_enqueue_draw(g_app.sphere_12, transform, mat);
            }
            ++count;
        }
        printf("[info] Section 6: %d random mixed shapes\n", count);
    }

    // =====================================================================
    // SECTION 7: Extra distant ring — 30 Blinn-Phong spheres at very far range
    //          Tests frustum culling edge cases
    // =====================================================================
    {
        const float radius = 55.0f;
        int count = 0;
        for (int i = 0; i < 30; ++i) {
            float a = (float)i / 30.0f * 6.2831853f;
            float y = static_cast<float>(i % 5 - 2) * 2.0f;

            // Darker colors at distance
            float brightness = 0.4f + static_cast<float>((i * 5) % 7) * 0.1f;
            float rgb[3] = {brightness, brightness * 0.8f, brightness * 0.6f};

            // Cycle textures
            RendererTextureHandle tex = {};
            if (g_app.tex_count > 0) {
                tex = g_app.textures[i % g_app.tex_count];
            }

            spawn_shape(g_app.sphere_12, ring_pos(radius, a, y), 0.5f,
                        rgb, 1.0f, ShadingModel::BlinnPhong, tex);
            ++count;
        }
        printf("[info] Section 7: %d Blinn-Phong spheres at radius %.1f (far)\n", count, radius);
    }

    // =====================================================================
    // SECTION 8: Near-field cubes — 10 close cubes for parallax test
    // =====================================================================
    {
        int count = 0;
        for (int i = 0; i < 10; ++i) {
            float a = (float)i / 10.0f * 3.1415926f; // half-circle in front
            float r = 6.0f + static_cast<float>(i % 3) * 1.5f;
            glm::vec3 pos(r * cosf(a), (static_cast<float>(i % 3) - 1) * 2.0f, r * sinf(a));

            float v = 0.5f + static_cast<float>(i % 4) * 0.15f;
            float rgb[3] = {v, v * 0.7f, v * 0.9f};

            spawn_shape(g_app.cube, pos, 0.6f, rgb, 1.0f, ShadingModel::Unlit);
            ++count;
        }
        printf("[info] Section 8: %d near-field cubes\n", count);
    }

    // =====================================================================
    // Total shape count
    // =====================================================================
    int total_shapes = 8 + 36 + 24 + 20 + 20 + 50 + 30 + 10; // = 208
    printf("[info] Total shapes in scene: %d\n", total_shapes);

    // ---- Laser line-quads (subtle ambient lines) ----
    {
        struct LaserBeam { float p0[3]; float p1[3]; float width; float color[4]; };
        const LaserBeam lasers[] = {
            { {-15.0f, 8.0f, -10.0f}, {15.0f, -2.0f, 10.0f}, 0.04f, {0.3f, 0.1f, 0.5f, 0.3f} },
            { {-10.0f, -3.0f, 12.0f}, {10.0f,  3.0f, -8.0f}, 0.05f, {0.1f, 0.4f, 0.5f, 0.25f} },
        };
        for (const auto& laser : lasers) {
            renderer_enqueue_line_quad(laser.p0, laser.p1, laser.width, laser.color);
        }
    }

    // ---- ImGui HUD ----
    {
        int submitted = renderer_get_draw_count();
        int triangles = renderer_get_triangle_count();
        int culled    = g_app.culling_on ? renderer_get_culled_count() : 0;

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
        {
            float fps = ImGui::GetIO().Framerate;
            char buf[512];
            snprintf(buf, sizeof(buf),
                "FPS:          %.1f\n"
                "Draws:        %d\n"
                "Triangles:    %d\n"
                "Culled:       %d\n"
                "Frustum cull: %s [C]\n"
                "Shading mode: %s [L]",
                fps, submitted, triangles, culled,
                g_app.culling_on ? "ON" : "OFF",
                g_app.override == ShadingOverride::Mixed ? "Mixed" :
                g_app.override == ShadingOverride::AllUnlit ? "AllUnlit" : "AllLambertian");
            ImGui::TextUnformatted(buf);
        }
        ImGui::End();
    }

    renderer_end_frame();
}

int main() {
    RendererConfig cfg;
    cfg.title   = "R-M6 Diverse Scene [L=shade C=cull]";
    cfg.clear_r = 0.02f;
    cfg.clear_g = 0.02f;
    cfg.clear_b = 0.05f;
    cfg.clear_a = 1.0f;

    renderer_init(cfg);
    renderer_set_frame_callback(on_frame, nullptr);
    renderer_set_input_callback(on_input, nullptr);
    renderer_run();
    return 0;
}
