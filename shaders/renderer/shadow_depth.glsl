@ctype mat4 glm::mat4
@ctype vec4 glm::vec4

@vs shadow_depth_vs
@glsl_options fixup_clipspace
layout(binding=0) uniform shadow_depth_vs_params {
    mat4 light_mvp;
};

in vec3 position;

out float v_light_z;

void main() {
    gl_Position = light_mvp * vec4(position, 1.0);
    // In orthographic projection, clip-space z equals eye-space z.
    // Pass it through for linear depth computation in the fragment shader.
    v_light_z = gl_Position.z / gl_Position.w;
}
@end

@fs fs_shadow
out vec4 frag_color;

in float v_light_z;

// Shadow frustum near/far (must match k_shadow_near / k_shadow_far).
layout(binding=1) uniform shadow_depth_constants {
    float s_near;
    float s_far;
};

void main() {
    // Linearize from NDC [-1, 1] to [0, 1].
    float linear = (v_light_z + 1.0) / 2.0;
    
    // Write to depth buffer.
    gl_FragDepth = linear;
    
    // Also write to color attachment for debug visualization.
    frag_color = vec4(linear, linear, linear, 1.0);
}
@end

@program shadow_depth shadow_depth_vs fs_shadow
