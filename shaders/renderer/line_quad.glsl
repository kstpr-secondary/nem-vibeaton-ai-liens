@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs line_quad_vs
layout(binding=0) uniform vs_params {
    mat4 vp;
};

in vec3 position;
in vec4 color;

out vec4 v_color;

void main() {
    v_color     = color;
    gl_Position = vp * vec4(position, 1.0);
}
@end

@fs line_quad_fs
in vec4 v_color;
out vec4 frag_color;

void main() {
    frag_color = v_color;
}
@end

@program line_quad line_quad_vs line_quad_fs
