@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs unlit_vs
layout(binding=0) uniform unlit_vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs unlit_fs
layout(binding=1) uniform unlit_fs_params {
    vec4 color;
    vec4 flags;
};

out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program unlit unlit_vs unlit_fs
