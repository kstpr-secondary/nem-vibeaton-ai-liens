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
#define SOKOL_IMGUI_IMPL
#include "imgui.h"
#include "util/sokol_imgui.h"

#include "renderer.h"
#include "shaders/magenta.glsl.h"
#include "pipeline_unlit.h"
#include "mesh_builders.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
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
    glm::mat4             vp            = glm::mat4(1.0f); // projection × view; identity until set_camera is called
    bool                  camera_set    = false;            // reset each begin_frame
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

    // Mesh GPU resources are managed by mesh_builders.cpp (see mesh_vbuf_get / mesh_ibuf_get)

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

// Called from the internal sokol init callback (GL context exists at that point).
// Stores the result in state.pipeline_magenta — all other pipeline helpers fall back to it.
void make_magenta_pipeline() {
    sg_shader shd = sg_make_shader(magenta_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] FATAL: magenta fallback shader creation failed\n");
        return;
    }
    sg_pipeline_desc desc = {};
    // Stride matches the full Vertex layout so this pipeline is compatible with every mesh buffer.
    desc.layout.buffers[0].stride              = sizeof(Vertex);
    desc.layout.attrs[ATTR_magenta_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_magenta_position].offset = 0;
    desc.shader                                = shd;
    desc.index_type                            = SG_INDEXTYPE_UINT32;
    desc.depth.compare                         = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled                   = true;
    desc.cull_mode                             = SG_CULLMODE_BACK;
    desc.label                                 = "magenta-error-pipeline";
    state.pipeline_magenta = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(state.pipeline_magenta) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] FATAL: magenta fallback pipeline creation failed\n");
    }
}

// Called by sokol_app after the GL context is ready.
// renderer_init() must have been called first to populate state.config.
void renderer_internal_init() {
    sg_desc gfx_desc = {};
    gfx_desc.environment = sglue_environment();
    gfx_desc.logger.func = slog_func;
    sg_setup(&gfx_desc);

    stm_setup();

    state.pass_action.colors[0].load_action   = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value.r = state.config.clear_r;
    state.pass_action.colors[0].clear_value.g = state.config.clear_g;
    state.pass_action.colors[0].clear_value.b = state.config.clear_b;
    state.pass_action.colors[0].clear_value.a = state.config.clear_a;

    make_magenta_pipeline();
    state.pipeline_unlit = create_pipeline_unlit(state.pipeline_magenta);

    simgui_desc_t simgui_desc = {};
    simgui_desc.sample_count = sapp_sample_count();
    simgui_setup(&simgui_desc);

    state.initialized = true;
}

// Called by sokol_app for every input/window event.
// Passes the event to ImGui first, then forwards to the registered InputCallback.
void renderer_internal_event(const sapp_event* e) {
    simgui_handle_event(e);
    if (state.input_cb) {
        state.input_cb(e, state.input_userdata);
    }
}

// Called by sokol_app once per frame. Computes dt and forwards to the registered FrameCallback.
// Full begin/end_frame orchestration is added in later tasks (R-024, R-039).
void renderer_internal_frame() {
    static uint64_t last_tick = 0;
    uint64_t now = stm_now();
    float dt = (last_tick == 0) ? 0.0f : static_cast<float>(stm_sec(stm_diff(now, last_tick)));
    last_tick = now;

    if (state.frame_cb) {
        state.frame_cb(dt, state.frame_userdata);
    }
}

// Called by sokol_app on window close / sapp_quit(). Delegates to renderer_shutdown().
void renderer_internal_cleanup() {
    renderer_shutdown();
}

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

void renderer_run() {
    sapp_desc desc = {};
    desc.init_cb    = renderer_internal_init;
    desc.frame_cb   = renderer_internal_frame;
    desc.cleanup_cb = renderer_internal_cleanup;
    desc.event_cb   = renderer_internal_event;
    desc.width      = state.config.width;
    desc.height     = state.config.height;
    desc.window_title = state.config.title;
    desc.gl.major_version = 3;
    desc.gl.minor_version = 3;
    desc.logger.func = slog_func;
    sapp_run(&desc);
}

void renderer_shutdown() {
    simgui_shutdown();
    mesh_store_shutdown(); // destroy GPU mesh buffers before sg_shutdown
    sg_shutdown();
    sapp_quit();

    state.frame_active = false;

    // Invalidate cached pipeline handles (GPU resources already freed by sg_shutdown)
    state.pipeline_magenta     = {};
    state.pipeline_unlit       = {};
    state.pipeline_lambertian  = {};
    state.pipeline_blinnphong  = {};
    state.pipeline_transparent = {};
    state.pipeline_line_quad   = {};
    state.pipeline_skybox      = {};
}

void renderer_begin_frame() {
    assert(!state.frame_active && "renderer_begin_frame called while frame already active");
    state.frame_active    = true;
    state.draw_count      = 0;
    state.line_quad_count = 0;
    state.camera_set      = false;
    sg_pass pass = {};
    pass.action    = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
}

void renderer_end_frame() {
    if (!state.camera_set) {
        printf("[renderer] WARNING: renderer_set_camera not called before end_frame — using identity VP\n");
        state.vp = glm::mat4(1.0f);
    }
}

void renderer_set_camera(const RendererCamera& camera) {
    state.camera     = camera;
    glm::mat4 view   = glm::make_mat4(camera.view);
    glm::mat4 proj   = glm::make_mat4(camera.projection);
    state.vp         = proj * view;
    state.camera_set = true;
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
    if (!state.frame_active) return;
    if (!renderer_handle_valid(mesh)) return;
    if (state.draw_count >= 1024) {
        printf("[renderer] draw queue full — command dropped\n");
        return;
    }
    DrawCommand& cmd = state.draw_queue[state.draw_count++];
    cmd.mesh     = mesh;
    cmd.material = material;
    for (int i = 0; i < 16; ++i) cmd.world_transform[i] = world_transform[i];
}

void renderer_enqueue_line_quad(const float p0[3], const float p1[3],
                                float width, const float color[4]) {
    if (!state.frame_active) return;
    if (width <= 0.0f) return;
    // reject zero-length segment
    float dx = p1[0] - p0[0], dy = p1[1] - p0[1], dz = p1[2] - p0[2];
    if (dx*dx + dy*dy + dz*dz == 0.0f) return;
    if (state.line_quad_count >= 256) {
        printf("[renderer] line_quad queue full — command dropped\n");
        return;
    }
    // Store raw endpoints + attributes; billboard geometry computed later in end_frame
    LineQuadCommand& cmd = state.line_quad_queue[state.line_quad_count++];
    // Encode p0 in verts[0].position and p1 in verts[1].position; width in verts[0].color[3]
    cmd.verts[0].position[0] = p0[0]; cmd.verts[0].position[1] = p0[1]; cmd.verts[0].position[2] = p0[2];
    cmd.verts[1].position[0] = p1[0]; cmd.verts[1].position[1] = p1[1]; cmd.verts[1].position[2] = p1[2];
    for (int i = 0; i < 4; ++i) {
        cmd.verts[0].color[i] = color ? color[i] : 1.0f;
        cmd.verts[1].color[i] = color ? color[i] : 1.0f;
        cmd.verts[2].color[i] = color ? color[i] : 1.0f;
        cmd.verts[3].color[i] = color ? color[i] : 1.0f;
    }
    // width stored in verts[2].position[0] for later billboard expansion
    cmd.verts[2].position[0] = width;
    cmd.verts[2].position[1] = 0.0f;
    cmd.verts[2].position[2] = 0.0f;
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
