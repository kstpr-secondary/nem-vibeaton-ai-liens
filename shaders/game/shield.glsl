@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec3 glm::vec3

@vs shield_vs
layout(binding=0) uniform shield_vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;
in vec3 normal;

out vec3 v_normal;
out vec3 v_world_pos;

void main() {
    v_world_pos = (model * vec4(position, 1.0)).xyz;
    v_normal    = normalize(mat3(normal_mat) * normal);
    gl_Position = mvp * vec4(position, 1.0);
}
@end

@fs shield_fs
layout(binding=1) uniform shield_fs_params {
    vec4 shield_color;    // .rgb = tint (e.g. 0.3,0.5,1.0 blue), .a = base opacity scale
    vec4 view_pos_w;      // .xyz = camera world position
    vec4 fresnel_params;  // .x = exponent (3.0), .y = rim intensity (1.2)
};

in vec3 v_normal;
in vec3 v_world_pos;

out vec4 frag_color;

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(view_pos_w.xyz - v_world_pos);
    float rim = pow(1.0 - max(dot(N, V), 0.0), fresnel_params.x);
    float alpha = rim * fresnel_params.y * shield_color.a;
    frag_color = vec4(shield_color.rgb * (0.3 + rim), alpha);
}
@end

@program shield shield_vs shield_fs
