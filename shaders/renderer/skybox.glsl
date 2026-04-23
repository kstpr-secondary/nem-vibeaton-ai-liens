@ctype mat4 glm::mat4

@vs skybox_vs
layout(binding=0) uniform vs_params {
    mat4 view_proj;  // projection * mat4(mat3(view)) — translation stripped from view
};

in vec3 position;  // unit-cube vertex; position IS the cubemap lookup direction

out vec3 v_texcoord;

void main() {
    v_texcoord  = position;
    vec4 pos    = view_proj * vec4(position, 1.0);
    gl_Position = pos.xyww;  // z = w → NDC z = 1.0 (far plane) after perspective divide
}
@end

@fs skybox_fs
layout(binding=0) uniform textureCube skybox_tex;
layout(binding=0) uniform sampler smp;

in vec3 v_texcoord;

out vec4 frag_color;

void main() {
    frag_color = texture(samplerCube(skybox_tex, smp), normalize(v_texcoord));
}
@end

@program skybox skybox_vs skybox_fs
