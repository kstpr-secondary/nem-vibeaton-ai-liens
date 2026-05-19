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
    // Read linear depth [0,1] from R32F color attachment.
    float d = texture(sampler2D(shadow_tex, debug_smp), v_uv).r;
    
    // Invert so closer-to-light surfaces are brighter (dark shadow on bright ground).
    float inverted = 1.0 - d;
    
    // Aggressive contrast stretch: small depth differences become visible.
    float contrast = pow(max(inverted, 0.0), 0.4);
    
    // Gamma for perceptual uniformity.
    contrast = pow(max(contrast, 0.0), 1.0 / 2.2);
    
    frag_color = vec4(contrast, contrast, contrast, 1.0);
}
@end

@program shadow_debug shadow_debug_vs fs_shadow_debug
