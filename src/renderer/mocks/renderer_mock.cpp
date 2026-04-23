#include "renderer.h"

// No-op renderer mock. Swapped in when USE_RENDERER_MOCKS=ON.
// All void functions are no-ops; handle-returning functions return {1} (valid sentinel).

void renderer_init(const RendererConfig&) {}
void renderer_set_frame_callback(FrameCallback, void*) {}
void renderer_set_input_callback(InputCallback, void*) {}
void renderer_run() {}
void renderer_shutdown() {}

void renderer_begin_frame() {}
void renderer_end_frame() {}

void renderer_set_camera(const RendererCamera&) {}
void renderer_set_directional_light(const DirectionalLight&) {}
void renderer_set_skybox(RendererTextureHandle) {}

void renderer_enqueue_draw(RendererMeshHandle, const float[16], const Material&) {}
void renderer_enqueue_line_quad(const float[3], const float[3], float, const float[4]) {}

RendererMeshHandle renderer_make_sphere_mesh(float, int)                              { return {1}; }
RendererMeshHandle renderer_make_cube_mesh(float)                                     { return {1}; }
RendererMeshHandle renderer_upload_mesh(const Vertex*, uint32_t, const uint32_t*, uint32_t) { return {1}; }

RendererTextureHandle renderer_upload_texture_2d(const void*, int, int, int)          { return {1}; }
RendererTextureHandle renderer_upload_texture_from_file(const char*)                  { return {1}; }
RendererTextureHandle renderer_upload_cubemap(const void*[6], int, int, int)          { return {1}; }

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m;
    m.shading_model = ShadingModel::Unlit;
    if (rgba) { m.base_color[0]=rgba[0]; m.base_color[1]=rgba[1]; m.base_color[2]=rgba[2]; m.base_color[3]=rgba[3]; }
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m;
    m.shading_model = ShadingModel::Lambertian;
    if (rgb) { m.base_color[0]=rgb[0]; m.base_color[1]=rgb[1]; m.base_color[2]=rgb[2]; }
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                           RendererTextureHandle texture) {
    Material m;
    m.shading_model = ShadingModel::BlinnPhong;
    if (rgb) { m.base_color[0]=rgb[0]; m.base_color[1]=rgb[1]; m.base_color[2]=rgb[2]; }
    m.shininess = shininess;
    m.texture   = texture;
    return m;
}
