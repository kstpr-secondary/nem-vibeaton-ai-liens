@ctype mat4 glm::mat4
@ctype vec4 glm::vec4
@ctype vec3 glm::vec3

@vs lambertian_vs
layout(binding=0) uniform lambertian_vs_params {
    mat4 mvp;
    mat4 model;
    mat4 normal_mat;
};

in vec3 position;
in vec3 normal;

out vec3 v_normal;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    // Use normal_mat for correct normal transform (handles non-uniform scaling)
    v_normal = normalize(mat3(normal_mat) * normal);
}
@end

@fs lambertian_fs
layout(binding=1) uniform lambertian_fs_params {
    vec4 light_dir;        // .xyz = direction FROM surface TO light, .w unused
    vec4 light_color;      // .rgb = light color, .w unused
    float light_intensity; // intensity scalar
    float _pad;            // std140 padding (vec3→vec4 alignment)
    vec4 base_color;       // .rgba = material albedo + alpha
};

in vec3 v_normal;
out vec4 frag_color;

void main() {
    // Lambertian: N dot L
    // light_dir is assumed to be the direction FROM the surface TO the light
    float ndotl = max(dot(normalize(v_normal), normalize(light_dir.xyz)), 0.0);
    vec3 diffuse = base_color.rgb * light_color.rgb * light_intensity * ndotl;
    frag_color = vec4(diffuse, base_color.a);
}
@end

@program lambertian lambertian_vs lambertian_fs
