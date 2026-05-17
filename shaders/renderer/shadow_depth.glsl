@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs shadow_depth_vs
@glsl_options fixup_clipspace
layout(binding=0) uniform shadow_depth_vs_params {
    mat4 light_mvp;
};

in vec3 position;

void main() {
    gl_Position = light_mvp * vec4(position, 1.0);
}
@end

@fs fs_shadow
out vec4 frag_color;
void main() {
    frag_color = vec4(gl_FragDepth, gl_FragDepth, gl_FragDepth, 1.0);
}
@end

@program shadow_depth shadow_depth_vs fs_shadow
