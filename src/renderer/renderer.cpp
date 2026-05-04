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
#include <cstring>
#include <algorithm>

// Generated shader headers — must come AFTER glm (some use @ctype glm mappings).
#include "shaders/magenta.glsl.h"
#include "shaders/unlit.glsl.h"
#include "shaders/line_quad.glsl.h"
#include "shaders/blinnphong.glsl.h"
#include "shaders/lambertian.glsl.h"
#include "pipeline_unlit.h"
#include "pipeline_lambertian.h"
#include "pipeline_blinnphong.h"
#include "mesh_internal.h"

// ---------------------------------------------------------------------------
// Internal command types
// ---------------------------------------------------------------------------

struct DrawCommand {
    RendererMeshHandle mesh;
    float              world_transform[16];
    Material           material;
};

// ---------------------------------------------------------------------------
// Renderer state — file-private singleton
// ---------------------------------------------------------------------------

namespace {

// ---------------------------------------------------------------------------
// Pipeline cache (Section 3.2.1)
// ---------------------------------------------------------------------------

struct PipelineCacheEntry {
    uint32_t     shader_id;   // RendererShaderHandle::id
    PipelineState state;
    sg_pipeline  pipeline;
};

static constexpr int k_pipeline_cache_max = 64;
static PipelineCacheEntry pipeline_cache[k_pipeline_cache_max];
static int                pipeline_cache_count = 0;

// ---------------------------------------------------------------------------
// RendererState
// ---------------------------------------------------------------------------

struct RendererState {
    // Draw queues
    DrawCommand     draw_queue[1024];
    int             draw_count       = 0;
    LineQuadCommand line_quad_queue[256];
    int             line_quad_count  = 0;

    // Triangle count and cull count — updated at end of end_frame().
    int triangle_count      = 0;
    int prev_triangle_count = 0;
    int cull_count          = 0;
    int prev_cull_count     = 0;

    // Frustum culling toggle
    bool culling_enabled = true;

    // Scene state (set per-frame via public API)
    RendererCamera        camera        = {};
    glm::mat4             vp            = glm::mat4(1.0f);
    bool                  camera_set    = false;
    DirectionalLight      light         = {};
    bool                  light_set     = false;
    RendererTextureHandle skybox_handle = {};

    // Pipeline cache (replaces 7 named sg_pipeline fields)
    int pipeline_cache_count_ = 0;

    // Built-in shader handles — populated in renderer_internal_init()
    RendererShaderHandle builtin_shaders[3];

    // sg_shader handles indexed by shader ID (0=magenta sentinel, 1-3=built-ins, 4+=custom)
    sg_shader shader_handles[256];

    // Time uniform (Section 3.2.5)
    float current_time = 0.0f;

    // Line-quad pipelines + blend-mode storage
    sg_pipeline pipeline_line_quad;
    sg_pipeline pipeline_line_quad_additive;
    BlendMode   line_quad_blend[256]; // per-command blend mode

    // Skybox
    sg_pipeline pipeline_skybox;

    // Mesh GPU resources are managed by mesh_builders.cpp
    // Texture GPU resources are managed by texture.cpp

    // Lifecycle flags
    bool frame_active = false;
    bool initialized  = false;

    // Registered callbacks
    InputCallback input_cb       = nullptr;
    void*         input_userdata = nullptr;
    FrameCallback frame_cb       = nullptr;
    void*         frame_userdata = nullptr;

    // Stored config
    RendererConfig config = {};

    // Pass action
    sg_pass_action pass_action = {};

    // Dummy 1×1 white texture for untextured BlinnPhong draws.
    sg_image dummy_blinnphong_tex = {};
    sg_view dummy_blinnphong_view = {};
    sg_sampler dummy_blinnphong_smp = {};

    // Line-quad static buffers
    sg_buffer line_quad_vbuf = {};
    sg_buffer line_quad_ibuf = {};
    bool      line_quad_bufs_init = false;
};

RendererState state;

// ---------------------------------------------------------------------------
// Pipeline cache helper (Section 3.2.1)
// ---------------------------------------------------------------------------

static sg_pipeline get_or_create_pipeline(RendererShaderHandle sh, PipelineState ps) {
    // Linear scan for existing entry
    for (int i = 0; i < pipeline_cache_count; ++i) {
        const auto& entry = pipeline_cache[i];
        if (entry.shader_id == sh.id &&
            entry.state.blend == ps.blend &&
            entry.state.cull == ps.cull &&
            entry.state.depth_write == ps.depth_write &&
            entry.state.render_queue == ps.render_queue) {
            return entry.pipeline;
        }
    }

    // Not found — need to create a new pipeline.
    // Look up the sg_shader handle by ID from our stored handles array.
    sg_shader shd = {};
    if (sh.id >= 1 && sh.id < 256) {
        shd = state.shader_handles[sh.id];
    }

    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] WARNING: shader lookup failed for pipeline id=%u — using magenta fallback\n", sh.id);
        return pipeline_cache[0].pipeline; // fallback to magenta sentinel
    }

    // Build pipeline descriptor
    sg_pipeline_desc desc = {};
    desc.shader = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;

    // Vertex layout — standard Vertex struct with all 4 attributes
    desc.layout.buffers[0].stride = sizeof(Vertex);
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // position (offset 0)
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // normal   (offset 12)
    desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2; // uv       (offset 24)
    desc.layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT3; // tangent  (offset 28)

    // Set blend mode
    switch (ps.blend) {
        case BlendMode::Additive:
            desc.colors[0].blend.enabled          = true;
            desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_ONE;
            desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE;
            desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
            desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
            break;
        case BlendMode::AlphaBlend:
            desc.colors[0].blend.enabled          = true;
            desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
            desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
            desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
            break;
        case BlendMode::Cutout:
        case BlendMode::Opaque:
        default:
            desc.colors[0].blend.enabled = false;
            break;
    }

    // Depth state
    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = ps.depth_write;

    // Cull mode
    switch (ps.cull) {
        case CullMode::Front:     desc.cull_mode = SG_CULLMODE_FRONT; break;
        case CullMode::Off:       desc.cull_mode = SG_CULLMODE_NONE;   break;
        case CullMode::Back:
        default:                  desc.cull_mode = SG_CULLMODE_BACK;  break;
    }

    desc.label = "cached-pipeline";

    sg_pipeline pip = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] WARNING: pipeline creation failed — using magenta fallback\n");
        pip = pipeline_cache[0].pipeline; // fallback to magenta sentinel
    }

    // Store in cache
    if (pipeline_cache_count < k_pipeline_cache_max) {
        pipeline_cache[pipeline_cache_count].shader_id   = sh.id;
        pipeline_cache[pipeline_cache_count].state       = ps;
        pipeline_cache[pipeline_cache_count].pipeline     = pip;
        ++pipeline_cache_count;
    } else {
        printf("[renderer] WARNING: pipeline cache full (%d) — reusing last entry\n", k_pipeline_cache_max);
    }

    return pip;
}

// ---------------------------------------------------------------------------
// Uniform dispatcher — applies the correct uniform struct per shader type
// (Each builtin shader declares different uniform block layouts.)
// ---------------------------------------------------------------------------

static void apply_draw_uniforms(RendererShaderHandle shader,
                                const Material& mat,
                                const glm::mat4& model_mat,
                                const glm::mat4& mvp) {
    // --- Binding 0: vertex shader uniforms ---
    if (shader.id == 1) {
        // Unlit: only mat4 mvp (64 bytes)
        struct UnlitVS { glm::mat4 mvp; };
        UnlitVS uvs = { mvp };
        sg_range r = { &uvs, sizeof(uvs) };
        sg_apply_uniforms(0, &r);
    } else {
        // BlinnPhong / Lambertian: mat4 mvp + model + normal_mat (192 bytes)
        glm::mat4 normal_mat = glm::transpose(glm::inverse(model_mat));
        struct VSParams { glm::mat4 mvp; glm::mat4 model; glm::mat4 normal_mat; };
        VSParams vs_p = { mvp, model_mat, normal_mat };
        sg_range r = { &vs_p, sizeof(vs_p) };
        sg_apply_uniforms(0, &r);
    }

    // --- Binding 1: fragment shader uniforms ---
    if (shader.id == 2) {
        // BlinnPhong: 6×vec4 = 96 bytes. Patch light fields into local copy.
        auto* fs = material_uniforms_as<BlinnPhongFSParams>(const_cast<Material&>(mat));
        glm::mat4 cam_view = state.camera_set ? glm::make_mat4(state.camera.view) : glm::mat4(1.0f);
        glm::mat4 cam_world = glm::inverse(cam_view);
        glm::vec3 cam_pos = glm::vec3(cam_world[3][0], cam_world[3][1], cam_world[3][2]);
        BlinnPhongFSParams patched;
        patched.base_color        = fs->base_color;
        patched.light_dir_ws      = glm::vec4(state.light.direction[0], state.light.direction[1],
                                              state.light.direction[2], 0.0f);
        patched.light_color_inten = glm::vec4(state.light.color[0], state.light.color[1],
                                              state.light.color[2], state.light.intensity);
        patched.view_pos_w        = glm::vec4(cam_pos.x, cam_pos.y, cam_pos.z, 0.0f);
        patched.spec_shin         = fs->spec_shin;
        patched.flags             = fs->flags;
        sg_range r = { &patched, sizeof(patched) };
        sg_apply_uniforms(1, &r);
    } else if (shader.id == 3) {
        // Lambertian: vec4+vec4+float+float+pad[8]+vec4 = 64 bytes
        auto* fs = material_uniforms_as<BlinnPhongFSParams>(const_cast<Material&>(mat));
        glm::mat4 cam_view = state.camera_set ? glm::make_mat4(state.camera.view) : glm::mat4(1.0f);
        glm::mat4 cam_world = glm::inverse(cam_view);
        glm::vec3 cam_pos = glm::vec3(cam_world[3][0], cam_world[3][1], cam_world[3][2]);

        struct LambertianFS {
            glm::vec4 light_dir;        // .xyz = dir surface→light
            glm::vec4 light_color;      // .rgb = color
            float     light_intensity;  // scalar
            float     _pad;             // std140 padding
            uint8_t   _pad_40[8];       // align base_color to 16-byte boundary
            glm::vec4 base_color;       // .rgba = albedo + alpha
        };
        static_assert(sizeof(LambertianFS) == 64, "LambertianFS must be exactly 64 bytes");
        LambertianFS lfs;
        lfs.light_dir         = glm::vec4(state.light.direction[0], state.light.direction[1],
                                          state.light.direction[2], 0.0f);
        lfs.light_color       = glm::vec4(state.light.color[0], state.light.color[1],
                                          state.light.color[2], 0.0f);
        lfs.light_intensity   = state.light.intensity;
        lfs._pad              = 0.0f;
        std::memset(lfs._pad_40, 0, sizeof(lfs._pad_40));
        lfs.base_color        = fs->base_color;
        sg_range r = { &lfs, sizeof(lfs) };
        sg_apply_uniforms(1, &r);
    } else if (shader.id == 1) {
        // Unlit: single vec4 base_color (16 bytes) — reuse from material
        auto* fs = material_uniforms_as<UnlitFSParams>(const_cast<Material&>(mat));
        sg_range r = { &fs->color, sizeof(fs->color) };
        sg_apply_uniforms(1, &r);
    } else {
        // Custom shader: forward material blob verbatim
        sg_range r = { mat.uniforms, mat.uniforms_size };
        sg_apply_uniforms(1, &r);
    }
}

// ---------------------------------------------------------------------------
// Magenta fallback pipeline
// ---------------------------------------------------------------------------

void make_magenta_pipeline() {
    sg_shader shd = sg_make_shader(magenta_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] FATAL: magenta fallback shader creation failed\n");
        return;
    }
    sg_pipeline_desc desc = {};
    desc.layout.buffers[0].stride              = sizeof(Vertex);
    desc.layout.attrs[ATTR_magenta_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_magenta_position].offset = 0;
    desc.shader                                = shd;
    desc.index_type                            = SG_INDEXTYPE_UINT32;
    desc.depth.compare                         = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled                   = true;
    desc.cull_mode                             = SG_CULLMODE_BACK;
    desc.label                                 = "magenta-error-pipeline";
    // Store in cache index 0 as sentinel
    pipeline_cache[0].shader_id   = 0; // sentinel: shader_id=0 means magenta fallback
    pipeline_cache[0].state       = {};
    pipeline_cache[0].pipeline     = sg_make_pipeline(&desc);
    if (sg_query_pipeline_state(pipeline_cache[0].pipeline) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] FATAL: magenta fallback pipeline creation failed\n");
    }
    pipeline_cache_count = 1;
}

// ---------------------------------------------------------------------------
// Line-quad pipelines
// ---------------------------------------------------------------------------

void make_line_quad_pipelines() {
    sg_shader shd = sg_make_shader(line_quad_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad shader creation failed\n");
        return;
    }

    // Opaque alpha-blended line-quad pipeline
    sg_pipeline_desc desc = {};
    desc.shader     = shd;
    desc.index_type = SG_INDEXTYPE_UINT32;
    desc.layout.buffers[0].stride = sizeof(LineQuadVertex);
    desc.layout.attrs[ATTR_line_quad_position].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[ATTR_line_quad_position].offset  = offsetof(LineQuadVertex, position);
    desc.layout.attrs[ATTR_line_quad_color].format    = SG_VERTEXFORMAT_FLOAT4;
    desc.layout.attrs[ATTR_line_quad_color].offset     = offsetof(LineQuadVertex, color);
    desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = false;
    desc.cull_mode           = SG_CULLMODE_NONE;
    desc.colors[0].blend.enabled          = true;
    desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    desc.label = "line-quad-pipeline";
    state.pipeline_line_quad = sg_make_pipeline(&desc);

    // Additive line-quad pipeline (Section 3.2.4)
    sg_pipeline_desc add_desc = {};
    add_desc.shader     = shd;
    add_desc.index_type = SG_INDEXTYPE_UINT32;
    add_desc.layout.buffers[0].stride = sizeof(LineQuadVertex);
    add_desc.layout.attrs[ATTR_line_quad_position].format = SG_VERTEXFORMAT_FLOAT3;
    add_desc.layout.attrs[ATTR_line_quad_position].offset  = offsetof(LineQuadVertex, position);
    add_desc.layout.attrs[ATTR_line_quad_color].format    = SG_VERTEXFORMAT_FLOAT4;
    add_desc.layout.attrs[ATTR_line_quad_color].offset     = offsetof(LineQuadVertex, color);
    add_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    add_desc.depth.write_enabled = false;
    add_desc.cull_mode           = SG_CULLMODE_NONE;
    add_desc.colors[0].blend.enabled          = true;
    add_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_ONE;
    add_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE;
    add_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    add_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ZERO;
    add_desc.label = "line-quad-additive-pipeline";
    state.pipeline_line_quad_additive = sg_make_pipeline(&add_desc);

    if (sg_query_pipeline_state(state.pipeline_line_quad) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad pipeline creation failed\n");
    }
    if (sg_query_pipeline_state(state.pipeline_line_quad_additive) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: line_quad additive pipeline creation failed\n");
    }
}

// ---------------------------------------------------------------------------
// Sokol init callback (GL context exists at this point)
// ---------------------------------------------------------------------------

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

    // Magenta fallback (pre-inserted at cache index 0, shader_id=0 sentinel)
    make_magenta_pipeline();

    // Built-in shaders — create sg_shader handles with fixed IDs 1, 2, 3
    state.shader_handles[1] = sg_make_shader(unlit_shader_desc(sg_query_backend()));
    state.shader_handles[2] = sg_make_shader(blinnphong_shader_desc(sg_query_backend()));
    state.shader_handles[3] = sg_make_shader(lambertian_shader_desc(sg_query_backend()));

    // Store handles in builtin_shaders array for API compatibility
    { RendererShaderHandle h; h.id = 1; state.builtin_shaders[(int)BuiltinShader::Unlit] = h; }
    { RendererShaderHandle h; h.id = 2; state.builtin_shaders[(int)BuiltinShader::BlinnPhong] = h; }
    { RendererShaderHandle h; h.id = 3; state.builtin_shaders[(int)BuiltinShader::Lambertian] = h; }

    // Skybox
    state.pipeline_skybox = skybox_create_pipeline(pipeline_cache[0].pipeline);
    skybox_init_resources();

    // Line-quad pipelines (opaque + additive)
    make_line_quad_pipelines();

    // ImGui
    simgui_desc_t simgui_desc = {};
    simgui_desc.sample_count = sapp_sample_count();
    simgui_setup(&simgui_desc);

    state.initialized = true;
}

// ---------------------------------------------------------------------------
// Event / frame / cleanup callbacks
// ---------------------------------------------------------------------------

void renderer_internal_event(const sapp_event* e) {
    simgui_handle_event(e);
    if (state.input_cb) {
        state.input_cb(e, state.input_userdata);
    }
}

void renderer_internal_frame() {
    static uint64_t last_tick = 0;
    uint64_t now = stm_now();
    float dt = (last_tick == 0) ? 0.0f : static_cast<float>(stm_sec(stm_diff(now, last_tick)));
    last_tick = now;

    if (state.frame_cb) {
        state.frame_cb(dt, state.frame_userdata);
    }
}

void renderer_internal_cleanup() {
    renderer_shutdown();
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
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
    skybox_shutdown();
    mesh_store_shutdown();
    texture_store_shutdown();

    // Destroy line-quad buffers
    if (state.line_quad_vbuf.id != 0) { sg_destroy_buffer(state.line_quad_vbuf); state.line_quad_vbuf = {}; }
    if (state.line_quad_ibuf.id != 0) { sg_destroy_buffer(state.line_quad_ibuf); state.line_quad_ibuf = {}; }
    state.line_quad_bufs_init = false;

    // Destroy pipeline cache entries
    for (int i = 0; i < pipeline_cache_count; ++i) {
        if (pipeline_cache[i].pipeline.id != 0) {
            sg_destroy_pipeline(pipeline_cache[i].pipeline);
            pipeline_cache[i].pipeline = {};
        }
    }
    pipeline_cache_count = 0;

    if (state.pipeline_line_quad.id != 0)       { sg_destroy_pipeline(state.pipeline_line_quad); state.pipeline_line_quad = {}; }
    if (state.pipeline_line_quad_additive.id != 0) { sg_destroy_pipeline(state.pipeline_line_quad_additive); state.pipeline_line_quad_additive = {}; }
    if (state.pipeline_skybox.id != 0)          { sg_destroy_pipeline(state.pipeline_skybox); state.pipeline_skybox = {}; }

    // Destroy dummy BlinnPhong resources
    if (state.dummy_blinnphong_tex.id != 0)     { sg_destroy_image(state.dummy_blinnphong_tex); state.dummy_blinnphong_tex = {}; }
    if (state.dummy_blinnphong_view.id != 0)    { sg_destroy_view(state.dummy_blinnphong_view); state.dummy_blinnphong_view = {}; }
    if (state.dummy_blinnphong_smp.id != 0)     { sg_destroy_sampler(state.dummy_blinnphong_smp); state.dummy_blinnphong_smp = {}; }

    sg_shutdown();
    sapp_quit();

    state.frame_active = false;
}

void renderer_begin_frame() {
    assert(!state.frame_active && "renderer_begin_frame called while frame already active");
    state.frame_active          = true;
    state.draw_count            = 0;
    state.line_quad_count       = 0;
    state.prev_triangle_count   = state.triangle_count;
    state.prev_cull_count       = state.cull_count;
    state.triangle_count        = 0;
    state.cull_count            = 0;
    state.camera_set            = false;
    state.light_set             = false;

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

// ---------------------------------------------------------------------------
// renderer_end_frame — generic draw loop with pipeline cache (Section 3.2.3)
// Rendering order: opaques+cutouts → skybox → transparents/additive → UI
// ---------------------------------------------------------------------------

void renderer_end_frame() {
    if (!state.camera_set) {
        printf("[renderer] WARNING: renderer_set_camera not called before end_frame — using identity VP\n");
        state.vp = glm::mat4(1.0f);
    }

    // --- Frustum plane extraction --------------------------------------------
    struct FrustumPlane { glm::vec4 xyzw; };
    FrustumPlane frustum_planes[6];
    {
        const auto& c = state.vp;
        frustum_planes[0] = {{ c[0].w+c[0].x, c[1].w+c[1].x, c[2].w+c[2].x, c[3].w+c[3].x }};
        frustum_planes[1] = {{ c[0].w-c[0].x, c[1].w-c[1].x, c[2].w-c[2].x, c[3].w-c[3].x }};
        frustum_planes[2] = {{ c[0].w+c[0].y, c[1].w+c[1].y, c[2].w+c[2].y, c[3].w+c[3].y }};
        frustum_planes[3] = {{ c[0].w-c[0].y, c[1].w-c[1].y, c[2].w-c[2].y, c[3].w-c[3].y }};
        frustum_planes[4] = {{ c[0].w+c[0].z, c[1].w+c[1].z, c[2].w+c[2].z, c[3].w+c[3].z }};
        frustum_planes[5] = {{ c[0].w-c[0].z, c[1].w-c[1].z, c[2].w-c[2].z, c[3].w-c[3].z }};
    }

    // --- Sort draw_queue by render_queue (stable, ascending) -----------------
    int sorted_indices[1024];
    for (int i = 0; i < state.draw_count; ++i) {
        sorted_indices[i] = i;
    }
    std::stable_sort(sorted_indices, sorted_indices + state.draw_count,
        [](int a, int b) {
            return state.draw_queue[a].material.pipeline.render_queue <
                   state.draw_queue[b].material.pipeline.render_queue;
        });

    // --- Helper: extract indices from a sorted range by render_queue value ---
    auto extract_indices = [sorted_indices](int start, int end, int* out, int max_out) -> int {
        int count = 0;
        for (int i = start; i < end && count < max_out; ++i) {
            out[count++] = sorted_indices[i];
        }
        return count;
    };

    // --- Helper: compute view-space Z for a set of draw commands ------------
    auto compute_view_z = [](const int* indices, int count, float* z_out) {
        glm::mat4 view_mat = state.camera_set ? glm::make_mat4(state.camera.view) : glm::mat4(1.0f);
        for (int i = 0; i < count; ++i) {
            const DrawCommand& cmd = state.draw_queue[indices[i]];
            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::vec3 world_pos(model[3][0], model[3][1], model[3][2]);
            z_out[i] = (view_mat * glm::vec4(world_pos, 1.0f)).z;
        }
    };

    // --- Helper: AABB frustum culling --------------------------------------
    auto compute_cull_status = [frustum_planes](const int* indices, int count, bool* culled_out) {
        glm::mat4 world_mat;
        float corners[8][3];
        float wc[8][4];
        for (int i = 0; i < count; ++i) {
            const DrawCommand& cmd = state.draw_queue[indices[i]];
            MeshAABB aabb = mesh_aabb_get(cmd.mesh.id);
            float half = aabb.half;
            if (half <= 0.0f) { culled_out[i] = false; continue; }

            world_mat = glm::make_mat4(cmd.world_transform);
            for (int c = 0; c < 8; ++c) {
                corners[c][0] = aabb.center[0] + ((c & 1) ?  half : -half);
                corners[c][1] = aabb.center[1] + ((c & 2) ?  half : -half);
                corners[c][2] = aabb.center[2] + ((c & 4) ?  half : -half);
            }
            for (int c = 0; c < 8; ++c) {
                wc[c][0] = world_mat[0][0]*corners[c][0] + world_mat[1][0]*corners[c][1] + world_mat[2][0]*corners[c][2] + world_mat[3][0];
                wc[c][1] = world_mat[0][1]*corners[c][0] + world_mat[1][1]*corners[c][1] + world_mat[2][1]*corners[c][2] + world_mat[3][1];
                wc[c][2] = world_mat[0][2]*corners[c][0] + world_mat[1][2]*corners[c][1] + world_mat[2][2]*corners[c][2] + world_mat[3][2];
                wc[c][3] = world_mat[0][3]*corners[c][0] + world_mat[1][3]*corners[c][1] + world_mat[2][3]*corners[c][2] + world_mat[3][3];
            }
            bool culled_flag = false;
            for (int p = 0; p < 6 && !culled_flag; ++p) {
                bool all_outside = true;
                for (int c = 0; c < 8; ++c) {
                    float d = frustum_planes[p].xyzw.x*wc[c][0] + frustum_planes[p].xyzw.y*wc[c][1] +
                              frustum_planes[p].xyzw.z*wc[c][2] + frustum_planes[p].xyzw.w*wc[c][3];
                    if (d >= 0.0f) { all_outside = false; break; }
                }
                if (all_outside) culled_flag = true;
            }
            culled_out[i] = culled_flag;
        }
    };

    // --- Helper: draw a batch of commands from the pipeline cache -----------
    auto draw_batch = [&](const int* indices, int count, const bool* culled, int culled_count) {
        sg_pipeline bound_pipeline = {};
        for (int i = 0; i < count; ++i) {
            if (culled && i < culled_count && culled[i]) continue;
            const DrawCommand& cmd = state.draw_queue[indices[i]];

            sg_pipeline pip = get_or_create_pipeline(cmd.material.shader, cmd.material.pipeline);
            if (pip.id != bound_pipeline.id) {
                sg_apply_pipeline(pip);
                bound_pipeline = pip;
            }

            glm::mat4 model = glm::make_mat4(cmd.world_transform);
            glm::mat4 mvp   = state.vp * model;
            apply_draw_uniforms(cmd.material.shader, cmd.material, model, mvp);

            sg_bindings bind = {};
            bind.vertex_buffers[0] = mesh_vbuf_get(cmd.mesh.id);
            bind.index_buffer      = mesh_ibuf_get(cmd.mesh.id);

            if (cmd.material.shader.id == 2) {
                // BlinnPhong texture slot
                sg_view tex_view = {};
                sg_sampler tex_smp = {};
                if (cmd.material.texture_count > 0 && cmd.material.textures[0].id != 0) {
                    uint32_t tex_id = cmd.material.textures[0].id;
                    tex_view = texture_get_view(tex_id);
                    tex_smp  = texture_get_sampler(tex_id);
                } else {
                    tex_view   = state.dummy_blinnphong_view;
                    tex_smp    = state.dummy_blinnphong_smp;
                }
                if (tex_view.id == 0 || tex_smp.id == 0) {
                    tex_view   = state.dummy_blinnphong_view;
                    tex_smp    = state.dummy_blinnphong_smp;
                }
                bind.views[VIEW_albedo_tex] = tex_view;
                bind.samplers[SMP_smp]      = tex_smp;
            } else if (cmd.material.shader.id != 1 && cmd.material.shader.id != 3) {
                // Custom shader: bind textures from material
                for (uint8_t t = 0; t < cmd.material.texture_count && t < k_material_texture_slots; ++t) {
                    if (cmd.material.textures[t].id != 0) {
                        sg_view v = texture_get_view(cmd.material.textures[t].id);
                        sg_sampler s = texture_get_sampler(cmd.material.textures[t].id);
                        if (v.id != 0 && s.id != 0) {
                            bind.views[t]    = v;
                            bind.samplers[t] = s;
                        }
                    }
                }
            }
            sg_apply_bindings(&bind);
            sg_draw(0, static_cast<int>(mesh_index_count_get(cmd.mesh.id)), 1);
        }
    };

    // --- Pass 1: Opaque objects (render_queue=0) with culling + front-to-back sort --
    int opaque_range_start = 0;
    for (int i = 0; i <= state.draw_count; ++i) {
        bool is_boundary = (i == state.draw_count) ||
            (state.draw_queue[sorted_indices[i]].material.pipeline.render_queue !=
             state.draw_queue[sorted_indices[opaque_range_start]].material.pipeline.render_queue);
        if (!is_boundary) continue;

        uint8_t queue_val = state.draw_queue[sorted_indices[opaque_range_start]].material.pipeline.render_queue;

        // --- Opaque: front-to-back sort (nearest first for early-Z rejection) --
        if (queue_val == 0) {
            int opaque_indices[1024];
            int opaque_total = extract_indices(opaque_range_start, i, opaque_indices, 1024);

            if (state.culling_enabled && opaque_total > 0) {
                float draw_z[1024];
                compute_view_z(opaque_indices, opaque_total, draw_z);
                std::sort(opaque_indices, opaque_indices + opaque_total,
                    [&draw_z](int a, int b) { return draw_z[a] > draw_z[b]; });

                bool culled[1024] = {};
                compute_cull_status(opaque_indices, opaque_total, culled);
                int total_culled = 0;
                for (int j = 0; j < opaque_total; ++j) if (culled[j]) ++total_culled;
                state.cull_count += total_culled;
                draw_batch(opaque_indices, opaque_total, culled, opaque_total);
            } else {
                draw_batch(opaque_indices, opaque_total, nullptr, 0);
            }
        }

        // --- Cutout: front-to-back sort (z-write, discards in shader) ----------
        if (queue_val == 1 && state.culling_enabled) {
            int cutout_indices[1024];
            int cutout_total = extract_indices(opaque_range_start, i, cutout_indices, 1024);

            if (cutout_total > 0) {
                float draw_z[1024];
                compute_view_z(cutout_indices, cutout_total, draw_z);
                std::sort(cutout_indices, cutout_indices + cutout_total,
                    [&draw_z](int a, int b) { return draw_z[a] > draw_z[b]; });

                bool culled[1024] = {};
                compute_cull_status(cutout_indices, cutout_total, culled);
                int total_culled = 0;
                for (int j = 0; j < cutout_total; ++j) if (culled[j]) ++total_culled;
                state.cull_count += total_culled;
                draw_batch(cutout_indices, cutout_total, culled, cutout_total);
            }
        }

        // --- Skybox pass (after opaques+cutouts so it fills z=1.0 gaps only) -----
        if (state.skybox_handle.id != 0) {
            sg_image cubemap_img = texture_get(state.skybox_handle.id);
            if (sg_query_image_state(cubemap_img) == SG_RESOURCESTATE_VALID) {
                glm::mat4 view_mat = state.camera_set ? glm::make_mat4(state.camera.view) : glm::mat4(1.0f);
                glm::mat4 proj_mat = state.camera_set ? glm::make_mat4(state.camera.projection) : glm::mat4(1.0f);
                draw_skybox_pass(state.pipeline_skybox, cubemap_img,
                                 glm::value_ptr(proj_mat), glm::value_ptr(view_mat));
            }
        }

        // --- Transparent+Additive: back-to-front sort (no z-write, z-read) -----
        if (queue_val >= 2 && queue_val <= 3) {
            int trans_indices[1024];
            int trans_total = extract_indices(opaque_range_start, i, trans_indices, 1024);

            if (trans_total > 0) {
                float trans_z[1024];
                compute_view_z(trans_indices, trans_total, trans_z);
                std::sort(trans_indices, trans_indices + trans_total,
                    [&trans_z](int a, int b) { return trans_z[a] < trans_z[b]; });

                bool culled[1024] = {};
                if (state.culling_enabled) {
                    compute_cull_status(trans_indices, trans_total, culled);
                    int total_culled = 0;
                    for (int j = 0; j < trans_total; ++j) if (culled[j]) ++total_culled;
                    state.cull_count += total_culled;
                }
                draw_batch(trans_indices, trans_total, culled, trans_total);
            }
        }

        // --- Skybox (render_queue=255): skip here, drawn below -----------------
        if (queue_val == 255) {
            // handled separately after opaque+cutout pass
        }

        opaque_range_start = i;
    }

    // --- Line quad draws (UI) ------------------------------------------------
    if (state.line_quad_count > 0) {
        if (!state.line_quad_bufs_init) {
            uint32_t line_quad_indices[256 * 6];
            for (int q = 0; q < 256; ++q) {
                uint32_t b = (uint32_t)q * 4;
                line_quad_indices[q*6+0] = b;
                line_quad_indices[q*6+1] = b+1;
                line_quad_indices[q*6+2] = b+2;
                line_quad_indices[q*6+3] = b;
                line_quad_indices[q*6+4] = b+2;
                line_quad_indices[q*6+5] = b+3;
            }

            const int max_verts = 256 * 4;
            sg_buffer_desc vdesc = {};
            vdesc.size               = sizeof(LineQuadVertex) * max_verts;
            vdesc.usage.vertex_buffer = true;
            vdesc.usage.stream_update = true;
            vdesc.label              = "line-quad-vbuf";
            state.line_quad_vbuf = sg_make_buffer(&vdesc);

            sg_buffer_desc idesc = {};
            idesc.size             = sizeof(line_quad_indices);
            idesc.usage.index_buffer = true;
            idesc.usage.immutable    = true;
            idesc.data               = { line_quad_indices, sizeof(line_quad_indices) };
            idesc.label              = "line-quad-ibuf";
            state.line_quad_ibuf = sg_make_buffer(&idesc);

            state.line_quad_bufs_init = true;
        }

       // Upload all quad vertices in a single call
        {
            static LineQuadVertex all_verts[256 * 4];
            for (int i = 0; i < state.line_quad_count; ++i) {
                const LineQuadCommand& cmd = state.line_quad_queue[i];
                std::memcpy(all_verts + i * 4, cmd.verts, sizeof(cmd.verts));
            }
            sg_range vrange;
            vrange.ptr = all_verts;
            vrange.size = sizeof(LineQuadVertex) * state.line_quad_count * 4;
            sg_update_buffer(state.line_quad_vbuf, &vrange);
        }

        line_quad_vs_params_t vs_p = { state.vp };
        sg_range vp_range = SG_RANGE(vs_p);
        sg_apply_uniforms(0, &vp_range);

        sg_bindings bind = {};
        bind.vertex_buffers[0] = state.line_quad_vbuf;
        bind.index_buffer      = state.line_quad_ibuf;

        // Route each command to the correct pipeline based on stored blend mode.
        // Additive quads first (order-independent), then opaque/alpha-blended.
        for (int i = 0; i < state.line_quad_count; ++i) {
            BlendMode bm = state.line_quad_blend[i];
            sg_pipeline pip = (bm == BlendMode::Additive) ? state.pipeline_line_quad_additive : state.pipeline_line_quad;
            sg_apply_pipeline(pip);
            sg_apply_uniforms(UB_line_quad_vs_params, &vp_range);
            sg_apply_bindings(&bind);
            sg_draw(i * 6, 6, 1); // draw one quad at a time (6 indices each)
        }
    }

    // Compute triangle count for the frame
    {
        int total = 0;
        for (int i = 0; i < state.draw_count; ++i) {
            total += static_cast<int>(mesh_index_count_get(state.draw_queue[i].mesh.id)) / 3;
        }
        state.triangle_count = total;
    }

    simgui_render();

    // Log cull count once per 60 frames
    {
        static int log_frame = 0;
        if (state.cull_count > 0 && ++log_frame >= 60) {
            printf("[renderer] frustum cull: %d/%d objects culled\n", state.cull_count, state.draw_count);
            log_frame = 0;
        }
    }

   sg_end_pass();
    sg_commit();
    state.frame_active = false;
}

int renderer_get_draw_count() {
    return state.draw_count;
}

int renderer_get_triangle_count() {
    return state.prev_triangle_count;
}

int renderer_get_culled_count() {
    return state.prev_cull_count;
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
        dx = 0.0f; dy = -1.0f; dz = 0.0f;
    } else {
        dx /= len; dy /= len; dz /= len;
    }
    state.light_set = true;
}

void renderer_set_skybox(RendererTextureHandle cubemap) {
    state.skybox_handle = cubemap;
}

void renderer_set_culling_enabled(bool enabled) {
    state.culling_enabled = enabled;
}

// ---------------------------------------------------------------------------
// Shader API (Section 3.1.2)
// ---------------------------------------------------------------------------

static constexpr int k_shader_desc_max = 32;
static const sg_shader_desc* custom_shader_descs[k_shader_desc_max] = {};
static int next_custom_shader_id     = 4; // after magenta(0) + built-in(1-3)

RendererShaderHandle renderer_create_shader(const sg_shader_desc* desc) {
    if (next_custom_shader_id >= k_shader_desc_max) {
        printf("[renderer] WARNING: custom shader table full (%d)\n", k_shader_desc_max);
        return {};
    }
    int id = next_custom_shader_id++;
    custom_shader_descs[id] = desc;

    sg_shader shd = sg_make_shader(desc);
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: custom shader creation failed (id=%d)\n", id);
        return {};
    }

    state.shader_handles[id] = shd;

    RendererShaderHandle handle;
    handle.id = static_cast<uint32_t>(id);
    return handle;
}

RendererShaderHandle renderer_builtin_shader(BuiltinShader s) {
    RendererShaderHandle h;
    h.id = static_cast<uint32_t>((int)s) + 1; // Unlit=1, BlinnPhong=2, Lambertian=3
    return h;
}

// ---------------------------------------------------------------------------
// Per-frame API (Section 3.2.5)
// ---------------------------------------------------------------------------

void renderer_set_time(float seconds_since_start) {
    // Stored for future time-based animation (animated materials, pulsating lights, etc.).
    // Currently unused by built-in shaders — reserved API slot.
    state.current_time = seconds_since_start;
}

// ---------------------------------------------------------------------------
// Draw submission
// ---------------------------------------------------------------------------

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
                                float width, const float color[4],
                                BlendMode blend) {
    if (!state.frame_active) return;
    if (width <= 0.0f) return;
    if (state.line_quad_count >= 256) {
        printf("[renderer] line_quad queue full — command dropped\n");
        return;
    }

    LineQuadCommand* cmd = &state.line_quad_queue[state.line_quad_count];
    state.line_quad_blend[state.line_quad_count] = blend;

    if (!debug_draw_compute_billboard(cmd, p0, p1, width, color, state.camera.view)) {
        return;
    }
    ++state.line_quad_count;
}

// ---------------------------------------------------------------------------
// Convenience factories (Section 3.1.4 — updated to fill uniforms blob)
// ---------------------------------------------------------------------------

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m{};
    m.shader = renderer_builtin_shader(BuiltinShader::Unlit);
    UnlitFSParams p{};
    if (rgba) {
        p.color = glm::vec4(rgba[0], rgba[1], rgba[2], rgba[3]);
    } else {
        p.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    p.flags = glm::vec4(0.0f); // no texture by default
    material_set_uniforms(m, p);

    if (rgba && rgba[3] < 1.0f) {
        m.pipeline.blend = BlendMode::AlphaBlend;
        m.pipeline.depth_write = false;
        m.pipeline.render_queue = 2;
    }
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m{};
    m.shader = renderer_builtin_shader(BuiltinShader::Lambertian);
    // Lambertian uses BlinnPhongFSParams layout for light patching compatibility
    // The game fills base_color and spec_shin (unused by Lambertian)
    BlinnPhongFSParams p{};
    if (rgb) {
        p.base_color = glm::vec4(rgb[0], rgb[1], rgb[2], 1.0f);
    } else {
        p.base_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    p.spec_shin = glm::vec4(1.0f, 1.0f, 1.0f, 32.0f); // unused by Lambertian
    p.flags     = glm::vec4(0.0f);
    material_set_uniforms(m, p);
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                            RendererTextureHandle texture) {
    Material m{};
    m.shader = renderer_builtin_shader(BuiltinShader::BlinnPhong);
    BlinnPhongFSParams p{};
    if (rgb) {
        p.base_color = glm::vec4(rgb[0], rgb[1], rgb[2], 1.0f);
    } else {
        p.base_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    p.spec_shin = glm::vec4(1.0f, 1.0f, 1.0f, shininess);
    p.flags = glm::vec4(texture.id != 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
    material_set_uniforms(m, p);

    if (texture.id != 0) {
        m.textures[0] = texture;
        m.texture_count = 1;
    }
    return m;
}
