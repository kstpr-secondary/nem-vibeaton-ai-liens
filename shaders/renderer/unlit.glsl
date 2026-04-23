@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs unlit_vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};

in vec3 position;
in vec3 normal;
in vec2 uv;
in vec3 tangent;

out vec2 frag_uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    frag_uv = uv;
}
@end

@fs unlit_fs
layout(binding=1) uniform fs_params {
    vec4 base_color;
};

in vec2 frag_uv;
out vec4 frag_color;

void main() {
    frag_color = base_color;
}
@end

@program unlit unlit_vs unlit_fs
