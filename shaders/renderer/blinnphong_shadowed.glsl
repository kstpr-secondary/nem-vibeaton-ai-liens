@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec3 glm::vec3
@ctype vec2 glm::vec2

@vs blinnphong_shadowed_vs
layout(binding=0) uniform blinnphong_shadowed_vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;
in vec3 normal;
in vec2 texcoord0;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_world_pos;

void main() {
    v_world_pos = (model * vec4(position, 1.0)).xyz;
    v_normal    = normalize(mat3(normal_mat) * normal);
    v_uv        = texcoord0;
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs blinnphong_shadowed_fs
// All light fields packed into vec4 for std140 safety (vec3 is 12 bytes but
// std140 pads to 16; glm::vec3 stays 12 — mismatch corrupts uniforms).
layout(binding=1) uniform blinnphong_shadowed_fs_params {
    vec4 base_color;         // .rgba = material albedo + alpha
    vec4 light_dir_ws;       // .xyz = direction FROM surface TO light (world space)
    vec4 light_color_inten;  // .xyz = light color, .w = intensity
    vec4 view_pos_w;         // .xyz = camera world position
    vec4 spec_shin;          // .xyz = specular highlight color, .w = shininess exponent (16–256)
    vec4 flags;              // .x = use_texture (1.0 = sample albedo_tex, 0.0 = solid base_color)
};

layout(binding=2) uniform blinnphong_shadowed_shadow_frame_params {
    mat4 light_view_proj;
};

// Albedo texture and sampler — same layout as original blinnphong.glsl,
// shifted to 4/5 because binding 2 is used by shadow_frame_params.
// Renamed to avoid macro collision with blinnphong.glsl.h (both define VIEW_albedo_tex).
layout(binding=4) uniform texture2D shadowed_albedo_tex;
layout(binding=5) uniform sampler shadowed_smp;

// Shadow map and comparison sampler at binding 0 — renderer sets these per-draw
// during the main pass dispatch (T-6).
layout(binding=0) uniform texture2D shadow_map;
layout(binding=0) uniform sampler shadow_sampler;

in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_pos;

out vec4 frag_color;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(light_dir_ws.xyz);   // direction FROM surface TO light
    vec3 V = normalize(view_pos_w.xyz - v_world_pos);
    vec3 H = normalize(L + V);              // half vector

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float spec  = pow(NdotH, spec_shin.w) * step(0.0, NdotL);

    // Sample texture if use_texture flag is set; otherwise fall back to solid base_color.
    vec3 albedo_rgb = flags.x > 0.5
        ? texture(sampler2D(shadowed_albedo_tex, shadowed_smp), v_uv).rgb * base_color.rgb
        : base_color.rgb;

    float alpha = flags.x > 0.5
        ? texture(sampler2D(shadowed_albedo_tex, shadowed_smp), v_uv).a * base_color.a
        : base_color.a;

    // Shadow UV computation.
    vec4 lp = light_view_proj * vec4(v_world_pos, 1.0);
    vec3 shadow_uv;
#if !SOKOL_GLSL
    // Non-GL backends (Metal/D3D) need Y-flip because clip-space Y direction
    // differs from texture-space Y direction.
    shadow_uv = vec3((lp.xy / lp.w) * vec2(0.5, -0.5) + 0.5, lp.z / lp.w);
#else
    shadow_uv = vec3((lp.xy / lp.w) * 0.5 + 0.5, lp.z / lp.w);
#endif

    // Hardware-accelerated PCF comparison.
    float bias = max(0.002 + 0.005 * (1.0 - dot(N, L)), 0.001);
    shadow_uv.z -= bias;
    
    float shadow_factor = texture(sampler2DShadow(shadow_map, shadow_sampler), shadow_uv);

    vec3 diffuse   = albedo_rgb * light_color_inten.rgb * light_color_inten.w * NdotL * shadow_factor;
    vec3 spec_term = spec_shin.rgb * light_color_inten.rgb * light_color_inten.w * spec * 0.5 * shadow_factor;
    vec3 ambient   = albedo_rgb * 0.15;  // ambient NOT modulated — objects in shadow still have ambient

    frag_color = vec4(ambient + diffuse + spec_term, alpha);
}
@end

@program blinnphong_shadowed blinnphong_shadowed_vs blinnphong_shadowed_fs
