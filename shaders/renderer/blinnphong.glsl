@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec2 glm::vec2

@vs blinnphong_vs
layout(binding=0) uniform blinnphong_vs_params {
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

@fs blinnphong_fs
// All light fields packed into vec4 for std140 safety (vec3 is 12 bytes but
// std140 pads to 16; glm::vec3 stays 12 — mismatch corrupts uniforms).
layout(binding=1) uniform blinnphong_fs_params {
    vec4 base_color;         // .rgba = material albedo + alpha
    vec4 light_dir_ws;       // .xyz = direction FROM surface TO light (world space)
    vec4 light_color_inten;  // .xyz = light color, .w = intensity
    vec4 view_pos_w;         // .xyz = camera world position
    vec4 spec_shin;          // .xyz = specular highlight color, .w = shininess exponent (16–256)
    vec4 flags;              // .x = use_texture (1.0 = sample albedo_tex, 0.0 = solid base_color)
};

layout(binding=2) uniform texture2D albedo_tex;
layout(binding=3) uniform sampler smp;

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

    // Sample texture if use_texture flag is set; otherwise fall back to solid color
    vec4 albedo = flags.x > 0.5
        ? texture(sampler2D(albedo_tex, smp), v_uv)
        : base_color;

    vec3 diffuse   = albedo.rgb * light_color_inten.rgb * light_color_inten.w * NdotL;
    vec3 spec_term = spec_shin.rgb * light_color_inten.rgb * light_color_inten.w * spec * 0.5;
    vec3 ambient   = albedo.rgb * 0.15;

    frag_color = vec4(ambient + diffuse + spec_term, albedo.a);
}
@end

@program blinnphong blinnphong_vs blinnphong_fs
