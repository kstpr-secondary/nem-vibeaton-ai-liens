// Temporary no-op renderer for engine workstream build/test.
// REMOVE this file when the real renderer implementation lands in src/renderer/.

#include "renderer.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void renderer_init(const RendererConfig&) {}
void renderer_set_frame_callback(FrameCallback, void*) {}
void renderer_set_input_callback(InputCallback, void*) {}
void renderer_run() {}
void renderer_shutdown() {}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void renderer_begin_frame() {}
void renderer_end_frame() {}

// ---------------------------------------------------------------------------
// Scene setup
// ---------------------------------------------------------------------------

void renderer_set_camera(const RendererCamera&) {}
void renderer_set_directional_light(const DirectionalLight&) {}
void renderer_set_skybox(RendererTextureHandle) {}

// ---------------------------------------------------------------------------
// Draw submission
// ---------------------------------------------------------------------------

void renderer_enqueue_draw(RendererMeshHandle, const float*, const Material&) {}
void renderer_enqueue_line_quad(const float*, const float*, float, const float*) {}

// ---------------------------------------------------------------------------
// Mesh builders — return a valid sentinel handle so engine code can check it
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_make_sphere_mesh(float, int)         { return {1}; }
RendererMeshHandle renderer_make_cube_mesh(float)                { return {1}; }
RendererMeshHandle renderer_upload_mesh(const Vertex*, uint32_t, const uint32_t*, uint32_t) { return {1}; }

// ---------------------------------------------------------------------------
// Texture upload
// ---------------------------------------------------------------------------

RendererTextureHandle renderer_upload_texture_2d(const void*, int, int, int)     { return {1}; }
RendererTextureHandle renderer_upload_texture_from_file(const char*)              { return {1}; }
RendererTextureHandle renderer_upload_cubemap(const void** /*faces[6]*/, int, int, int) { return {1}; }

// ---------------------------------------------------------------------------
// Material helpers
// ---------------------------------------------------------------------------

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m{};
    m.shading_model = ShadingModel::Unlit;
    memcpy(m.base_color, rgba, 4 * sizeof(float));
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m{};
    m.shading_model = ShadingModel::Lambertian;
    memcpy(m.base_color, rgb, 3 * sizeof(float));
    m.base_color[3] = 1.f;
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess, RendererTextureHandle texture) {
    Material m{};
    m.shading_model = ShadingModel::BlinnPhong;
    memcpy(m.base_color, rgb, 3 * sizeof(float));
    m.base_color[3] = 1.f;
    m.shininess     = shininess;
    m.texture       = texture;
    return m;
}

// ---------------------------------------------------------------------------
// sokol_app surface functions — stubbed so camera.cpp can call them.
// The real renderer defines these via SOKOL_APP_IMPL.
// ---------------------------------------------------------------------------

extern "C" float sapp_widthf()  { return 1280.f; }
extern "C" float sapp_heightf() { return 720.f; }
