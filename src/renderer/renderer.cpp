// sokol IMPL macros — must appear in exactly this one TU.
// All other TUs include the headers without the IMPL define.
#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_TIME_IMPL
#define SOKOL_LOG_IMPL
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "sokol_log.h"
#define SIMGUI_IMPL
#include "util/sokol_imgui.h"

#include "renderer.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Internal command types
// ---------------------------------------------------------------------------

struct DrawCommand {
    RendererMeshHandle mesh;
    float              world_transform[16];
    Material           material;
};

struct LineQuadVertex {
    float position[3];
    float color[4];
};

// Pre-computed (billboarded) corners — 4 verts, 6 indices shared via offset math
struct LineQuadCommand {
    LineQuadVertex verts[4];
};

// ---------------------------------------------------------------------------
// Renderer state — file-private singleton
// ---------------------------------------------------------------------------

namespace {

struct RendererState {
    // Draw queues
    DrawCommand     draw_queue[1024];
    int             draw_count       = 0;
    LineQuadCommand line_quad_queue[256];
    int             line_quad_count  = 0;

    // Scene state (set per-frame via public API)
    RendererCamera        camera        = {};
    DirectionalLight      light         = {};
    RendererTextureHandle skybox_handle = {};

    // Pipelines
    sg_pipeline pipeline_magenta;
    sg_pipeline pipeline_unlit;
    sg_pipeline pipeline_lambertian;
    sg_pipeline pipeline_blinnphong;
    sg_pipeline pipeline_transparent;
    sg_pipeline pipeline_line_quad;
    sg_pipeline pipeline_skybox;

    // Mesh GPU resources — index 0 reserved (invalid handle id == 0)
    sg_buffer mesh_vbufs[512]       = {};
    sg_buffer mesh_ibufs[512]       = {};
    uint32_t  mesh_index_counts[512] = {};
    uint32_t  next_mesh_id           = 1;

    // Texture GPU resources — index 0 reserved
    sg_image texture_table[256] = {};
    uint32_t next_texture_id    = 1;

    // Lifecycle flags
    bool frame_active = false;
    bool initialized  = false;

    // Registered callbacks
    InputCallback input_cb       = nullptr;
    void*         input_userdata = nullptr;
    FrameCallback frame_cb       = nullptr;
    void*         frame_userdata = nullptr;

    // Stored config (set by renderer_init before GL context exists)
    RendererConfig config = {};

    // Pass action — clear color populated from config in the GL init callback
    sg_pass_action pass_action = {};
};

RendererState state;

} // namespace

// ---------------------------------------------------------------------------
// Public API stubs — implementations follow in R-008 through R-025
// ---------------------------------------------------------------------------

void renderer_init(const RendererConfig& config) {
    state.config = config;
}

void renderer_set_frame_callback(FrameCallback cb, void* user_data) {
    state.frame_cb       = cb;
    state.frame_userdata = user_data;
}

void renderer_set_input_callback(InputCallback cb, void* user_data) {
    state.input_cb       = cb;
    state.input_userdata = user_data;
}

void renderer_run() {}

void renderer_shutdown() {}

void renderer_begin_frame() {}

void renderer_end_frame() {}

void renderer_set_camera(const RendererCamera& camera) {
    state.camera = camera;
}

void renderer_set_directional_light(const DirectionalLight& light) {
    state.light = light;
}

void renderer_set_skybox(RendererTextureHandle cubemap) {
    state.skybox_handle = cubemap;
}

void renderer_enqueue_draw(RendererMeshHandle mesh,
                           const float        world_transform[16],
                           const Material&    material) {
    (void)mesh; (void)world_transform; (void)material;
}

void renderer_enqueue_line_quad(const float p0[3], const float p1[3],
                                float width, const float color[4]) {
    (void)p0; (void)p1; (void)width; (void)color;
}

RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions) {
    (void)radius; (void)subdivisions; return {};
}

RendererMeshHandle renderer_make_cube_mesh(float half_extent) {
    (void)half_extent; return {};
}

RendererMeshHandle renderer_upload_mesh(const Vertex* vertices, uint32_t vertex_count,
                                        const uint32_t* indices, uint32_t index_count) {
    (void)vertices; (void)vertex_count; (void)indices; (void)index_count; return {};
}

RendererTextureHandle renderer_upload_texture_2d(const void* pixels,
                                                 int width, int height, int channels) {
    (void)pixels; (void)width; (void)height; (void)channels; return {};
}

RendererTextureHandle renderer_upload_texture_from_file(const char* path) {
    (void)path; return {};
}

RendererTextureHandle renderer_upload_cubemap(const void* faces[6],
                                              int face_width, int face_height, int channels) {
    (void)faces; (void)face_width; (void)face_height; (void)channels; return {};
}

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m;
    m.shading_model = ShadingModel::Unlit;
    if (rgba) {
        m.base_color[0] = rgba[0]; m.base_color[1] = rgba[1];
        m.base_color[2] = rgba[2]; m.base_color[3] = rgba[3];
    }
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m;
    m.shading_model = ShadingModel::Lambertian;
    if (rgb) {
        m.base_color[0] = rgb[0]; m.base_color[1] = rgb[1];
        m.base_color[2] = rgb[2];
    }
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                           RendererTextureHandle texture) {
    Material m;
    m.shading_model = ShadingModel::BlinnPhong;
    if (rgb) {
        m.base_color[0] = rgb[0]; m.base_color[1] = rgb[1];
        m.base_color[2] = rgb[2];
    }
    m.shininess = shininess;
    m.texture   = texture;
    return m;
}
