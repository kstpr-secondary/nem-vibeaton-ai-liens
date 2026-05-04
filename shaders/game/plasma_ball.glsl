@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec3 glm::vec3

// ──────────────────────────────────────────────────────────────────────────────
// Vertex shader
// ──────────────────────────────────────────────────────────────────────────────

@vs plasma_ball_vs
layout(binding=0) uniform plasma_ball_vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;
in vec3 normal;

out vec3 v_normal;
out vec3 v_world_pos;
out vec3 v_obj_pos;

void main() {
    v_obj_pos     = position;
    v_world_pos   = (model * vec4(position, 1.0)).xyz;
    v_normal      = normalize(mat3(normal_mat) * normal);
    gl_Position   = mvp * vec4(position, 1.0);
}
@end

// ──────────────────────────────────────────────────────────────────────────────
// Fragment shader
// ──────────────────────────────────────────────────────────────────────────────

@fs plasma_ball_fs
layout(binding=1) uniform plasma_ball_fs_params {
    vec4 core_color;    // .rgb = warm white (1.0,1.0,0.9)
    vec4 rim_color;     // .rgb = blue-white (0.4,0.6,1.0)
    vec4 bolt_color;    // .rgb = per-instance tint (from game)
    vec4 params;        // .x=time, .y=fresnel_exp(3.0), .z=core_threshold(0.65), .w=opacity(1.0)
    vec4 view_pos_w;    // .xyz = camera world position
};

in vec3 v_normal;
in vec3 v_world_pos;
in vec3 v_obj_pos;

out vec4 frag_color;

// ── Value noise (Appendix A from visual-improvements.md) ────────────────────

float hash31(vec3 p) {
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

float vnoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash31(i),                hash31(i + vec3(1, 0, 0)), f.x),
            mix(hash31(i + vec3(0, 1, 0)), hash31(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash31(i + vec3(0, 0, 1)),  hash31(i + vec3(1, 0, 1)), f.x),
            mix(hash31(i + vec3(0, 1, 1)),  hash31(i + vec3(1, 1, 1)), f.x), f.y),
        f.z);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(view_pos_w.xyz - v_world_pos);

    float NdotV = max(dot(N, V), 0.0);
    float rim   = pow(1.0 - NdotV, params.y);
    float core  = smoothstep(params.z - 0.15, params.z + 0.05, NdotV);

    vec3 p = normalize(v_obj_pos);
    float t = params.x;
    float n1 = vnoise(p * 4.0 + vec3(t * 0.6, 0.0, t * 0.4));
    float n2 = vnoise(p * 9.0 + vec3(-t * 0.9, t * 0.5, 0.0));
    float bolts = pow(max(n1 * n2, 0.0), 2.5) * 4.0;

    vec3 col = core_color.rgb * core * 3.0
             + bolt_color.rgb * bolts
             + rim_color.rgb * rim;
    float alpha = (core * 0.9 + rim * 0.7 + bolts * 0.5) * params.w;
    frag_color = vec4(col, alpha);
}
@end

@program plasma_ball plasma_ball_vs plasma_ball_fs
