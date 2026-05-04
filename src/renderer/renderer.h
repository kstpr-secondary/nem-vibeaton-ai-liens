// FROZEN — v1.2
// Source of truth: docs/interfaces/renderer-interface-spec.md
// Do NOT edit without human supervisor approval and version bump.

#pragma once
#include <cstdint>
#include <cstring>

// GLM for FS param structs (UnlitFSParams, BlinnPhongFSParams)
#include <glm/glm.hpp>

// sokol_gfx.h for sg_shader_desc pointer (renderer_create_shader)
// Note: renderer.cpp includes full sokol_gfx.h before this header, so downstream code
// that includes both will get sg_shader_desc transitively. For standalone use, include
// sokol_gfx.h before renderer.h.
struct sg_shader_desc;

// ---------------------------------------------------------------------------
// Handle types — opaque GPU resource references
// ---------------------------------------------------------------------------

struct RendererMeshHandle    { uint32_t id = 0; };
struct RendererTextureHandle { uint32_t id = 0; };
struct RendererShaderHandle  { uint32_t id = 0; };

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
// Pipeline state — travels with the material instance
// ---------------------------------------------------------------------------

enum class BlendMode  : uint8_t { Opaque = 0, Cutout, AlphaBlend, Additive };
enum class CullMode   : uint8_t { Back   = 0, Front, Off };

struct PipelineState {
    BlendMode blend        = BlendMode::Opaque;
    CullMode  cull         = CullMode::Back;
    bool      depth_write  = true;
    uint8_t   render_queue = 0;
    // render_queue: 0=opaque, 1=cutout, 2=transparent, 3=additive
    // Renderer draws passes in this order. Custom shaders choose their queue.
};

// ---------------------------------------------------------------------------
// Material — inline value type, passed per draw call
// ---------------------------------------------------------------------------

static constexpr int k_material_uniform_bytes = 256;
static constexpr int k_material_texture_slots  = 4;

struct Material {
    RendererShaderHandle  shader;
    PipelineState         pipeline;
    uint8_t               uniforms[k_material_uniform_bytes] = {};
    uint8_t               uniforms_size  = 0;
    RendererTextureHandle textures[k_material_texture_slots] = {};
    uint8_t               texture_count  = 0;
};

// Typed helpers — both declared inline in renderer.h
template<typename T>
void material_set_uniforms(Material& m, const T& params) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    memcpy(m.uniforms, &params, sizeof(T));
    m.uniforms_size = static_cast<uint8_t>(sizeof(T));
}

template<typename T>
T* material_uniforms_as(Material& m) {
    static_assert(sizeof(T) <= k_material_uniform_bytes, "Params struct too large");
    return reinterpret_cast<T*>(m.uniforms);
}

// ---------------------------------------------------------------------------
// Published FS param structs for built-in shaders
// (Game code may reinterpret_cast into Material::uniforms using these)
// ---------------------------------------------------------------------------

struct UnlitFSParams {
    glm::vec4 color;             // rgba
    glm::vec4 flags;             // reserved — not consumed by unlit shader (future animated textures)
};

struct BlinnPhongFSParams {
    glm::vec4 base_color;
    glm::vec4 light_dir_ws;
    glm::vec4 light_color_inten;
    glm::vec4 view_pos_w;
    glm::vec4 spec_shin;         // .rgb = specular color, .w = shininess
    glm::vec4 flags;             // .x = use_texture
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
    float view[16];        // column-major 4x4 world→camera
    float projection[16];  // column-major 4x4 camera→clip
};

// ---------------------------------------------------------------------------
// Input callback
// ---------------------------------------------------------------------------

using InputCallback = void(*)(const void* sapp_event, void* user_data);

// ---------------------------------------------------------------------------
// Frame callback — consumer injects per-frame logic
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
// Shader API
// ---------------------------------------------------------------------------

// Register a shader from a sokol-shdc generated descriptor.
// Call once at init after renderer_init(). Returns an opaque handle.
RendererShaderHandle renderer_create_shader(const sg_shader_desc* desc);

// Retrieve handles to the built-in shaders (no GPU work — just returns a stored handle).
enum class BuiltinShader : uint8_t { Unlit = 0, BlinnPhong, Lambertian };
RendererShaderHandle renderer_builtin_shader(BuiltinShader s);

// ---------------------------------------------------------------------------
// Per-frame API
// ---------------------------------------------------------------------------

void renderer_begin_frame();
void renderer_end_frame();

// Query submitted draw count, triangle count, and frustum cull count for ImGui HUD display.
int renderer_get_draw_count();
int renderer_get_triangle_count();
int renderer_get_culled_count();

// Propagate elapsed time to animated materials.
// Called once per tick from game_tick, before enqueue calls.
void renderer_set_time(float seconds_since_start);

// ---------------------------------------------------------------------------
// Scene setup (between begin_frame / end_frame)
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera& camera);
void renderer_set_directional_light(const DirectionalLight& light);
void renderer_set_skybox(RendererTextureHandle cubemap);
void renderer_set_culling_enabled(bool enabled);

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
    const float color[4],
    BlendMode   blend = BlendMode::Opaque   // Additive for laser/VFX
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
    uint32_t        index_count,
    float           radius = 0.0f      // 0.0f = auto-compute from vertex positions
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
// Material helpers — convenience factories
// (Internally fill the uniforms blob using material_set_uniforms<T>(…)
//  and set m.shader = renderer_builtin_shader(...))
// ---------------------------------------------------------------------------

Material renderer_make_unlit_material(const float rgba[4]);
Material renderer_make_lambertian_material(const float rgb[3]);
Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                            RendererTextureHandle texture = {});
