#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// Handle types — opaque GPU resource references
// ---------------------------------------------------------------------------

struct RendererMeshHandle    { uint32_t id = 0; };
struct RendererTextureHandle { uint32_t id = 0; };

inline bool renderer_handle_valid(RendererMeshHandle h)    { return h.id != 0; }
inline bool renderer_handle_valid(RendererTextureHandle h) { return h.id != 0; }

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

struct RendererConfig {
    int         width     = 1280;
    int         height    = 720;
    float       clear_r   = 0.05f;
    float       clear_g   = 0.05f;
    float       clear_b   = 0.10f;
    float       clear_a   = 1.0f;
    const char* title     = "renderer_app";
};

// ---------------------------------------------------------------------------
// Vertex layout — universal for all mesh types
// ---------------------------------------------------------------------------

struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[3];
};

// ---------------------------------------------------------------------------
// Shading model
// ---------------------------------------------------------------------------

enum class ShadingModel : uint8_t {
    Unlit      = 0,
    Lambertian = 1,
    BlinnPhong = 2,
};

// ---------------------------------------------------------------------------
// Material — inline value type, passed per draw call
// ---------------------------------------------------------------------------

struct Material {
    ShadingModel          shading_model = ShadingModel::Unlit;
    float                 base_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    RendererTextureHandle texture        = {};
    float                 shininess      = 32.0f;
    float                 alpha          = 1.0f;
};

// ---------------------------------------------------------------------------
// Directional light — one per frame maximum
// ---------------------------------------------------------------------------

struct DirectionalLight {
    float direction[3];
    float color[3];
    float intensity;
};

// ---------------------------------------------------------------------------
// Camera — view/projection pair, set once per frame
// ---------------------------------------------------------------------------

struct RendererCamera {
    float view[16];
    float projection[16];
};

// ---------------------------------------------------------------------------
// Input callback
// ---------------------------------------------------------------------------

using InputCallback = void(*)(const void* sapp_event, void* user_data);

// ---------------------------------------------------------------------------
// Frame callback (v1.1) — consumer injects per-frame logic
// ---------------------------------------------------------------------------

using FrameCallback = void(*)(float dt, void* user_data);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void renderer_init(const RendererConfig& config);
void renderer_set_frame_callback(FrameCallback cb, void* user_data);
void renderer_set_input_callback(InputCallback cb, void* user_data);
void renderer_run();
void renderer_shutdown();

// ---------------------------------------------------------------------------
// Per-frame API
// ---------------------------------------------------------------------------

void renderer_begin_frame();
void renderer_end_frame();

// ---------------------------------------------------------------------------
// Scene setup (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera& camera);
void renderer_set_directional_light(const DirectionalLight& light);
void renderer_set_skybox(RendererTextureHandle cubemap);

// ---------------------------------------------------------------------------
// Draw submission (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_enqueue_draw(
    RendererMeshHandle  mesh,
    const float         world_transform[16],
    const Material&     material
);

void renderer_enqueue_line_quad(
    const float p0[3],
    const float p1[3],
    float       width,
    const float color[4]
);

// ---------------------------------------------------------------------------
// Mesh builders
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions);
RendererMeshHandle renderer_make_cube_mesh(float half_extent);

RendererMeshHandle renderer_upload_mesh(
    const Vertex*   vertices,
    uint32_t        vertex_count,
    const uint32_t* indices,
    uint32_t        index_count
);

// ---------------------------------------------------------------------------
// Texture upload
// ---------------------------------------------------------------------------

RendererTextureHandle renderer_upload_texture_2d(
    const void* pixels,
    int         width,
    int         height,
    int         channels
);

RendererTextureHandle renderer_upload_texture_from_file(const char* path);

RendererTextureHandle renderer_upload_cubemap(
    const void* faces[6],
    int         face_width,
    int         face_height,
    int         channels
);

// ---------------------------------------------------------------------------
// Material helpers
// ---------------------------------------------------------------------------

Material renderer_make_unlit_material(const float rgba[4]);
Material renderer_make_lambertian_material(const float rgb[3]);
Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                           RendererTextureHandle texture = {});
