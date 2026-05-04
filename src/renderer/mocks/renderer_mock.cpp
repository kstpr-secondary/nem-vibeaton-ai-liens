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
void renderer_set_culling_enabled(bool) {}

void renderer_enqueue_draw(RendererMeshHandle, const float[16], const Material&) {}
void renderer_enqueue_line_quad(const float[3], const float[3], float, const float[4], BlendMode) {}

RendererMeshHandle renderer_make_sphere_mesh(float, int)                              { return {1}; }
RendererMeshHandle renderer_make_cube_mesh(float)                                     { return {1}; }
RendererMeshHandle renderer_upload_mesh(const Vertex*, uint32_t, const uint32_t*, uint32_t, float) { return {1}; }

RendererTextureHandle renderer_upload_texture_2d(const void*, int, int, int)          { return {1}; }
RendererTextureHandle renderer_upload_texture_from_file(const char*)                  { return {1}; }
RendererTextureHandle renderer_upload_cubemap(const void*[6], int, int, int)          { return {1}; }

RendererShaderHandle renderer_create_shader(const sg_shader_desc*)                    { return {1}; }
RendererShaderHandle renderer_builtin_shader(BuiltinShader)                           { return {1}; }

void renderer_set_time(float) {}

Material renderer_make_unlit_material(const float rgba[4]) {
    Material m;
    UnlitFSParams p{};
    if (rgba) { p.color = glm::vec4(rgba[0], rgba[1], rgba[2], rgba[3]); }
    else      { p.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); }
    p.flags = glm::vec4(0.0f);
    material_set_uniforms(m, p);
    m.shader = renderer_builtin_shader(BuiltinShader::Unlit);
    return m;
}

Material renderer_make_lambertian_material(const float rgb[3]) {
    Material m;
    BlinnPhongFSParams p{};
    if (rgb) { p.base_color = glm::vec4(rgb[0], rgb[1], rgb[2], 1.0f); }
    else     { p.base_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); }
   p.light_dir_ws       = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);   // light from above (surface→light)
    p.light_color_inten  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    p.spec_shin          = glm::vec4(1.0f, 1.0f, 1.0f, 32.0f);
    p.flags              = glm::vec4(0.0f);
    material_set_uniforms(m, p);
    m.shader = renderer_builtin_shader(BuiltinShader::Lambertian);
    return m;
}

Material renderer_make_blinnphong_material(const float rgb[3], float shininess,
                                             RendererTextureHandle texture) {
    Material m;
    BlinnPhongFSParams p{};
    if (rgb) { p.base_color = glm::vec4(rgb[0], rgb[1], rgb[2], 1.0f); }
    else     { p.base_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); }
    p.light_dir_ws       = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);   // light from above (surface→light)
    p.light_color_inten  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    p.view_pos_w         = glm::vec4(0.0f);
    p.spec_shin          = glm::vec4(1.0f, 1.0f, 1.0f, shininess);
    p.flags              = glm::vec4(texture.id != 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
    material_set_uniforms(m, p);
    m.shader = renderer_builtin_shader(BuiltinShader::BlinnPhong);
    if (texture.id != 0) {
        m.textures[0] = texture;
        m.texture_count = 1;
    }
    return m;
}
