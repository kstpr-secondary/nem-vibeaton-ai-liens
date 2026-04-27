// sokol IMPL macros — define BEFORE any sokol or project header include.
// This TU owns the implementation; all other TUs include sokol headers
// without the IMPL define.
#define SOKOL_GFX_IMPL
#define SOKOL_APP_IMPL
#define SOKOL_GLUE_IMPL
#define SOKOL_TIME_IMPL
#define SOKOL_LOG_IMPL
#define SOKOL_IMGUI_IMPL

// Sokol headers first — this defines SOKOL_GFX_INCLUDED so that downstream
// project headers (texture.h, skybox.h) skip their forward declarations.
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "sokol_log.h"
#include "imgui.h"
#include "util/sokol_imgui.h"

// Project headers — their sokol forward-decl guards see SOKOL_GFX_INCLUDED.
#include "renderer.h"
#include "debug_draw.h"
#include "texture.h"
#include "skybox.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>

// Generated shader headers — must come AFTER glm (some use @ctype glm mappings).
#include "shaders/magenta.glsl.h"
#include "shaders/unlit.glsl.h"
#include "shaders/line_quad.glsl.h"
#include "shaders/blinnphong.glsl.h"
#include "pipeline_unlit.h"
#include "pipeline_lambertian.h"
#include "pipeline_blinnphong.h"
#include "mesh_builders.h"

// ---------------------------------------------------------------------------
// Internal command types
// ---------------------------------------------------------------------------

struct DrawCommand {
    RendererMeshHandle mesh;
    float              world_transform[16];
    Material           material;
};

// Pre-computed (billboarded) corners — 4 verts, 6 indices shared via offset math
// LineQuadVertex and LineQuadCommand are defined in debug_draw.h

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
    bool                  light_set     = false;            // reset each begin_frame
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
    // Texture GPU resources are managed by texture.cpp (see texture_get / texture_store_insert)

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

    // Dummy 1×1 white texture + sampler + view for untextured BlinnPhong draws.
    sg_image dummy_blinnphong_tex = {};
    sg_view dummy_blinnphong_view = {};
    sg_sampler dummy_blinnphong_smp = {};
};

RendererState state;

// Line-quad static buffers — created lazily on first draw, destroyed in shutdown.
static sg_buffer line_quad_vbuf = {};
static sg_buffer line_quad_ibuf = {};
static bool      line_quad_bufs_init = false;

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

// Transparent pipeline: same unlit shader, alpha blend enabled, depth write OFF.
void make_transparent_pipeline() {
    sg_shader shd = sg_make_shader(unlit_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: transparent shader creation failed — using magenta fallback\n");
        state.pipeline_transparent = state.pipeline_magenta;
        return;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    desc.layout.buffers[0].stride              = sizeof(Vertex);
    desc.layout.attrs[ATTR_unlit_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_unlit_position].offset  = offsetof(Vertex, position);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;  // depth write OFF for correct transparency
    desc.cull_mode           = SG_CULLMODE_BACK;

    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;

    desc.label = "transparent-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: transparent pipeline creation failed — using magenta fallback\n");
        state.pipeline_transparent = state.pipeline_magenta;
        return;
    }
    state.pipeline_transparent = pip;
}

// Line-quad pipeline: position+color layout, alpha blend, depth write OFF.
// Draws pre-billboarded quads from line_quad_queue.
void make_line_quad_pipeline() {
    sg_shader shd = sg_make_shader(line_quad_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad shader creation failed — using magenta fallback\n");
        state.pipeline_line_quad = state.pipeline_magenta;
        return;
    }

    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    desc.layout.buffers[0].stride = sizeof(LineQuadVertex);
    desc.layout.attrs[ATTR_line_quad_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_line_quad_position].offset  = offsetof(LineQuadVertex, position);
    desc.layout.attrs[ATTR_line_quad_color].format    = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[ATTR_line_quad_color].offset     = offsetof(LineQuadVertex, color);

    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;  // depth write OFF (blended on top)
    desc.cull_mode           = SG_CULLMODE_NONE;  // billboards: both faces visible

    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;

    desc.label = "line-quad-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad pipeline creation failed — using magenta fallback\n");
        state.pipeline_line_quad = state.pipeline_magenta;
        return;
    }
    state.pipeline_line_quad = pip;
}

// Called by sokol_app after the GL context is ready.
// renderer_init() must have been called first to populate state.config.
void renderer_internal_init() {
    sg_desc gfx_desc = {};
    gfx_desc.environment = sglue_environment();
    gfx_desc.logger.func = slog_func;
    gfx_desc.buffer_pool_size = 2048;
    sg_setup(&gfx_desc);

    stm_setup();

    state.pass_action.colors[0].load_action   = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value.r = state.config.clear_r;
    state.pass_action.colors[0].clear_value.g = state.config.clear_g;
    state.pass_action.colors[0].clear_value.b = state.config.clear_b;
    state.pass_action.colors[0].clear_value.a = state.config.clear_a;

    // Dummy 1×1 white texture for untextured BlinnPhong draws.
    {
        unsigned char white_pixel[4] = {255, 255, 255, 255};
        sg_image_desc img_desc = {};
        img_desc.width          = 1;
        img_desc.height         = 1;
        img_desc.pixel_format   = SG_PIXELFORMAT_RGBA8;
        img_desc.data.mip_levels[0] = SG_RANGE(white_pixel);
        img_desc.label = "dummy-blinnphong-white";
        state.dummy_blinnphong_tex = sg_make_image(&img_desc);

        sg_view_desc vd = {};
        vd.texture.image  = state.dummy_blinnphong_tex;
        state.dummy_blinnphong_view = sg_make_view(&vd);

        sg_sampler_desc smp_desc = {};
        smp_desc.min_filter      = SG_FILTER_LINEAR;
        smp_desc.mag_filter      = SG_FILTER_LINEAR;
        smp_desc.wrap_u          = SG_WRAP_CLAMP_TO_EDGE;
        smp_desc.wrap_v          = SG_WRAP_CLAMP_TO_EDGE;
        smp_desc.label           = "dummy-blinnphong-sampler";
        state.dummy_blinnphong_smp = sg_make_sampler(&smp_desc);
    }

    make_magenta_pipeline();
    state.pipeline_unlit       = create_pipeline_unlit(state.pipeline_magenta);
    state.pipeline_lambertian  = create_pipeline_lambertian(state.pipeline_magenta);
    state.pipeline_blinnphong  = create_pipeline_blinnphong(state.pipeline_magenta);
    state.pipeline_skybox      = skybox_create_pipeline(state.pipeline_magenta);
    skybox_init_resources();
    make_transparent_pipeline();
    make_line_quad_pipeline();

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
    skybox_shutdown();        // destroy skybox VBO/IBO/sampler before sg_shutdown
    mesh_store_shutdown();    // destroy GPU mesh buffers before sg_shutdown
    texture_store_shutdown(); // destroy GPU textures before sg_shutdown
    if (line_quad_vbuf.id != 0) { sg_destroy_buffer(line_quad_vbuf); line_quad_vbuf = {}; }
    if (line_quad_ibuf.id != 0) { sg_destroy_buffer(line_quad_ibuf); line_quad_ibuf = {}; }
    line_quad_bufs_init = false;
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
    state.light_set       = false;

    simgui_frame_desc_t simgui_fd = {};
    simgui_fd.width      = sapp_width();
    simgui_fd.height     = sapp_height();
    simgui_fd.delta_time = sapp_frame_duration();
    simgui_fd.dpi_scale  = sapp_dpi_scale();
    simgui_new_frame(&simgui_fd);

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

    // Uniform structs matching unlit.glsl layout (std140, @ctype glm types).
    struct UnlitVSParams { glm::mat4 mvp; };
    struct UnlitFSParams { glm::vec4 base_color; };

    // Lambertian uniform structs matching lambertian.glsl std140 layout.
    struct LambertianVSParams {
        glm::mat4 mvp;
        glm::mat4 model;
    };
    struct LambertianFSParams {
        float light_dir[3];
        float _pad0;
        float light_color[3];
        float light_intensity;
        float base_color[4];
    };

    // --- (5) Skybox pass (LAST — renders at far plane behind everything) ----
    if (state.skybox_handle.id != 0) {
        sg_image cubemap_img = texture_get(state.skybox_handle.id);
        if (sg_query_image_state(cubemap_img) == SG_RESOURCESTATE_VALID) {
            glm::mat4 view_mat = glm::make_mat4(state.camera.view);
            glm::mat4 proj_mat = glm::make_mat4(state.camera.projection);
            draw_skybox_pass(state.pipeline_skybox, cubemap_img,
                             glm::value_ptr(proj_mat), glm::value_ptr(view_mat));
        }
    }

    // --- (1) Opaque draws (Unlit + Lambertian) ------------------------------
    if (state.draw_count > 0) {
        // Unlit pass
        bool bound = false;
        for (int i = 0; i < state.draw_count; ++i) {
            const DrawCommand& cmd = state.draw_queue[i];
            if (cmd.material.shading_model != ShadingModel::Unlit) continue;
            if (cmd.material.alpha < 1.0f) continue; // transparent → pass 3

            fprintf(stderr, "[RENDERER] unlit draw: mesh_id=%u idx_count=%u\n",
                    cmd.mesh.id, mesh_index_count_get(cmd.mesh.id));
            if (!bound) { sg_apply_pipeline(state.pipeline_unlit); bound = true; }

            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::mat4 mvp   = state.vp * model;

            sg_bindings bind       = {};
            bind.vertex_buffers[0] = mesh_vbuf_get(cmd.mesh.id);
            bind.index_buffer      = mesh_ibuf_get(cmd.mesh.id);
            sg_apply_bindings(&bind);

            UnlitVSParams vs_p = { mvp };
            sg_range vs_range  = SG_RANGE(vs_p);
            sg_apply_uniforms(0, &vs_range);

            UnlitFSParams fs_p = { glm::vec4(cmd.material.base_color[0],
                                             cmd.material.base_color[1],
                                             cmd.material.base_color[2],
                                             cmd.material.base_color[3]) };
            sg_range fs_range  = SG_RANGE(fs_p);
            sg_apply_uniforms(1, &fs_range);

            sg_draw(0, static_cast<int>(mesh_index_count_get(cmd.mesh.id)), 1);
        }

        // Lambertian pass
        if (!state.light_set) {
            int lam_count = 0;
            for (int i = 0; i < state.draw_count; ++i)
                if (state.draw_queue[i].material.shading_model == ShadingModel::Lambertian) ++lam_count;
            if (lam_count > 0)
                printf("[renderer] WARNING: %d Lambertian draw(s) queued but no directional light set\n", lam_count);
        }

        bound = false;
        for (int i = 0; i < state.draw_count; ++i) {
            const DrawCommand& cmd = state.draw_queue[i];
            if (cmd.material.shading_model != ShadingModel::Lambertian) continue;
            if (cmd.material.alpha < 1.0f) continue; // transparent → pass 3

            if (!bound) { sg_apply_pipeline(state.pipeline_lambertian); bound = true; }

            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::mat4 mvp   = state.vp * model;

            sg_bindings bind       = {};
            bind.vertex_buffers[0] = mesh_vbuf_get(cmd.mesh.id);
            bind.index_buffer      = mesh_ibuf_get(cmd.mesh.id);
            sg_apply_bindings(&bind);

            LambertianVSParams vs_p;
            vs_p.mvp   = mvp;
            vs_p.model = model;
            sg_range vs_range = SG_RANGE(vs_p);
            sg_apply_uniforms(0, &vs_range);

            LambertianFSParams fs_p;
            fs_p.light_dir[0]    = state.light.direction[0];
            fs_p.light_dir[1]    = state.light.direction[1];
            fs_p.light_dir[2]    = state.light.direction[2];
            fs_p._pad0           = 0.0f;
            fs_p.light_color[0]  = state.light.color[0];
            fs_p.light_color[1]  = state.light.color[1];
            fs_p.light_color[2]  = state.light.color[2];
            fs_p.light_intensity = state.light.intensity;
            fs_p.base_color[0]   = cmd.material.base_color[0];
            fs_p.base_color[1]   = cmd.material.base_color[1];
            fs_p.base_color[2]   = cmd.material.base_color[2];
            fs_p.base_color[3]   = cmd.material.base_color[3];
            sg_range fs_range    = SG_RANGE(fs_p);
            sg_apply_uniforms(1, &fs_range);

            sg_draw(0, static_cast<int>(mesh_index_count_get(cmd.mesh.id)), 1);
        }
    }

    // --- (2b) Opaque Blinn-Phong draws --------------------------------------
    {
        struct BlinnPhongVSParams { glm::mat4 mvp; glm::mat4 model; glm::mat4 normal_mat; };
        struct BlinnPhongFSParams {
            glm::vec4 base_color;
            glm::vec4 light_dir_ws;
            glm::vec4 light_color_inten;
            glm::vec4 view_pos_w;
            glm::vec4 spec_shin;
            glm::vec4 flags;
        };

        bool bound = false;
        for (int i = 0; i < state.draw_count; ++i) {
            const DrawCommand& cmd = state.draw_queue[i];
            if (cmd.material.shading_model != ShadingModel::BlinnPhong) continue;
            if (cmd.material.alpha < 1.0f) continue;

            if (!bound) { sg_apply_pipeline(state.pipeline_blinnphong); bound = true; }

            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::mat4 mvp   = state.vp * model;
            glm::mat3 normal_mat = glm::inverse(glm::mat3(model));

            sg_bindings bind       = {};
            bind.vertex_buffers[0] = mesh_vbuf_get(cmd.mesh.id);
            bind.index_buffer      = mesh_ibuf_get(cmd.mesh.id);

            if (cmd.material.texture.id != 0) {
                bind.views[VIEW_albedo_tex] = texture_get_view(cmd.material.texture.id);
                sg_sampler tex_smp = texture_get_sampler(cmd.material.texture.id);
                if (tex_smp.id != 0) {
                    bind.samplers[SMP_smp] = tex_smp;
                }
            } else {
                // No texture — bind dummy white texture so sokol validation passes.
                bind.views[VIEW_albedo_tex]   = state.dummy_blinnphong_view;
                bind.samplers[SMP_smp]        = state.dummy_blinnphong_smp;
            }

            sg_apply_bindings(&bind);

            BlinnPhongVSParams vs_p;
            vs_p.mvp        = mvp;
            vs_p.model      = model;
            vs_p.normal_mat = normal_mat;
            sg_range vs_range = SG_RANGE(vs_p);
            sg_apply_uniforms(UB_blinnphong_vs_params, &vs_range);

            float light_dir_norm[3] = { state.light.direction[0], state.light.direction[1], state.light.direction[2] };
            glm::vec3 light_dir_from = glm::vec3(-light_dir_norm[0], -light_dir_norm[1], -light_dir_norm[2]);

            glm::mat4 cam_view  = glm::make_mat4(state.camera.view);
            glm::vec3 cam_world = glm::vec3(cam_view[3][0], cam_view[3][1], cam_view[3][2]);

            BlinnPhongFSParams fs_p;
            fs_p.base_color         = glm::vec4(cmd.material.base_color[0], cmd.material.base_color[1], cmd.material.base_color[2], cmd.material.base_color[3]);
            fs_p.light_dir_ws       = glm::vec4(light_dir_from.x, light_dir_from.y, light_dir_from.z, 0.0f);
            fs_p.light_color_inten  = glm::vec4(state.light.color[0], state.light.color[1], state.light.color[2], state.light.intensity);
            fs_p.view_pos_w         = glm::vec4(cam_world.x, cam_world.y, cam_world.z, 0.0f);
            fs_p.spec_shin          = glm::vec4(1.0f, 1.0f, 1.0f, cmd.material.shininess);
            fs_p.flags              = glm::vec4(cmd.material.texture.id != 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
            sg_range fs_range = SG_RANGE(fs_p);
            sg_apply_uniforms(UB_blinnphong_fs_params, &fs_range);

            sg_draw(0, static_cast<int>(mesh_index_count_get(cmd.mesh.id)), 1);
        }
    }

    // --- (3) Transparent draws (alpha < 1.0) --------------------------------
    {
        bool bound = false;
        for (int i = 0; i < state.draw_count; ++i) {
            const DrawCommand& cmd = state.draw_queue[i];
            if (cmd.material.alpha >= 1.0f) continue;

            if (!bound) { sg_apply_pipeline(state.pipeline_transparent); bound = true; }

            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::mat4 mvp   = state.vp * model;

            sg_bindings bind       = {};
            bind.vertex_buffers[0] = mesh_vbuf_get(cmd.mesh.id);
            bind.index_buffer      = mesh_ibuf_get(cmd.mesh.id);
            sg_apply_bindings(&bind);

            UnlitVSParams vs_p = { mvp };
            sg_range vs_range  = SG_RANGE(vs_p);
            sg_apply_uniforms(0, &vs_range);

            UnlitFSParams fs_p = { glm::vec4(cmd.material.base_color[0],
                                             cmd.material.base_color[1],
                                             cmd.material.base_color[2],
                                             cmd.material.base_color[3]) };
            sg_range fs_range  = SG_RANGE(fs_p);
            sg_apply_uniforms(1, &fs_range);

            sg_draw(0, static_cast<int>(mesh_index_count_get(cmd.mesh.id)), 1);
        }
    }

   // --- (4) Line quad draws ------------------------------------------------
      if (state.line_quad_count > 0) {
          sg_apply_pipeline(state.pipeline_line_quad);

          if (!line_quad_bufs_init) {
              static uint32_t line_quad_indices[6] = { 0, 1, 2, 0, 2, 3 };

              const int max_verts = 256 * 4;
              sg_buffer_desc vdesc = {};
              vdesc.size           = sizeof(LineQuadVertex) * max_verts;
              vdesc.usage.vertex_buffer = true;
              vdesc.usage.stream_update   = true;
              vdesc.label               = "line-quad-vbuf";
              line_quad_vbuf = sg_make_buffer(&vdesc);

              sg_buffer_desc idesc = {};
              idesc.size           = sizeof(line_quad_indices);
              idesc.usage.index_buffer  = true;
              idesc.usage.immutable     = true;
              idesc.data                = SG_RANGE(line_quad_indices);
              idesc.label               = "line-quad-ibuf";
              line_quad_ibuf = sg_make_buffer(&idesc);

              line_quad_bufs_init = true;
          }

          // Upload all quad vertices in a single call (one update per frame).
          {
              static LineQuadVertex all_verts[256 * 4];
              for (int i = 0; i < state.line_quad_count; ++i) {
                  const LineQuadCommand& cmd = state.line_quad_queue[i];
                  std::memcpy(all_verts + i * 4, cmd.verts, sizeof(cmd.verts));
              }
              sg_range vrange;
             vrange.ptr = all_verts;
             vrange.size = sizeof(LineQuadVertex) * state.line_quad_count * 4;
              sg_update_buffer(line_quad_vbuf, &vrange);
          }

          line_quad_vs_params_t vs_p = { state.vp };
          sg_range vp_range = SG_RANGE(vs_p);
          sg_apply_uniforms(0, &vp_range);

          sg_bindings bind = {};
          bind.vertex_buffers[0] = line_quad_vbuf;
          bind.index_buffer      = line_quad_ibuf;
          sg_apply_bindings(&bind);

          for (int i = 0; i < state.line_quad_count; ++i) {
               sg_draw(i * 4, 4, 1);
           }
       }


    simgui_render();
    sg_end_pass();
    sg_commit();
    state.frame_active = false;
}

int renderer_get_draw_count() {
    return state.draw_count;
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
    float& dx = state.light.direction[0];
    float& dy = state.light.direction[1];
    float& dz = state.light.direction[2];
    float len = sqrtf(dx*dx + dy*dy + dz*dz);
    if (len < 1e-6f) {
        dx = 0.0f; dy = -1.0f; dz = 0.0f;  // fallback: straight down
    } else {
        dx /= len; dy /= len; dz /= len;
    }
    state.light_set = true;
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
    if (state.line_quad_count >= 256) {
        printf("[renderer] line_quad queue full — command dropped\n");
        return;
    }
    LineQuadCommand& cmd = state.line_quad_queue[state.line_quad_count];
    if (!debug_draw_compute_billboard(&cmd, p0, p1, width, color, state.camera.view)) {
        return;  // degenerate segment — skip silently
    }
    ++state.line_quad_count;
}

// renderer_upload_texture_2d, renderer_upload_texture_from_file, renderer_upload_cubemap
// are implemented in texture.cpp (own texture store — see texture.h).

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m;
    m.shading_model = ShadingModel::Unlit;
    if (rgba) {
        m.base_color[0] = rgba[0]; m.base_color[1] = rgba[1];
        m.base_color[2] = rgba[2]; m.base_color[3] = rgba[3];
        m.alpha         = rgba[3];
    }
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m;
    m.shading_model = ShadingModel::Lambertian;
    if (rgb) {
        m.base_color[0] = rgb[0]; m.base_color[1] = rgb[1];
        m.base_color[2] = rgb[2]; m.base_color[3] = 1.0f;
    }
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                            RendererTextureHandle texture) {
    Material m;
    m.shading_model = ShadingModel::BlinnPhong;
    m.shininess     = shininess;
    m.texture       = texture;
    if (rgb) {
        m.base_color[0] = rgb[0]; m.base_color[1] = rgb[1];
        m.base_color[2] = rgb[2]; m.base_color[3] = 1.0f;
    } else {
        m.base_color[0] = 1.0f; m.base_color[1] = 1.0f; m.base_color[2] = 1.0f; m.base_color[3] = 1.0f;
    }
    return m;
}
