@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs shadow_debug_vs
in vec2 position;
in vec2 texcoord0;
out vec2 v_uv;

void main() {
    v_uv = texcoord0;
    gl_Position = vec4(position, 0.0, 1.0);
}
@end

@fs fs_shadow_debug
// Shadow map texture and sampler at binding 0 (renderer sets these per-draw).
layout(binding=0) uniform texture2D shadow_tex;
layout(binding=0) uniform sampler debug_smp;

in vec2 v_uv;
out vec4 frag_color;

void main() {
    // Raw depth read (no comparison sampling — debug_smp has compare=NEVER).
    float d = texture(sampler2D(shadow_tex, debug_smp), v_uv).r;
    
    // Invert depth so background (1.0) is black (0.0) and objects (closer) are lighter.
    float display_d = 1.0 - d;
    
    // Boost contrast slightly if needed, but keep it simple first.
    frag_color = vec4(display_d, display_d, display_d, 1.0);
}
@end

@program shadow_debug shadow_debug_vs fs_shadow_debug
