@ctype mat4 float[16]

@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};
in vec3 position;
void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs fs
out vec4 frag_color;
void main() {
    frag_color = vec4(1.0, 0.0, 1.0, 1.0);
}
@end

@program magenta vs fs
