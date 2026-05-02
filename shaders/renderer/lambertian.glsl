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
    vec3 light_dir;
    vec3 light_color;
    float light_intensity;
    vec4 base_color;
};

in vec3 v_normal;
out vec4 frag_color;

void main() {
    // Lambertian: N dot L
    // light_dir is assumed to be the direction FROM the surface TO the light
    // or adjusted accordingly. Task says "normalize(light_dir)".
    float ndotl = max(dot(normalize(v_normal), normalize(light_dir)), 0.0);
    vec3 diffuse = base_color.rgb * light_color * light_intensity * ndotl;
    frag_color = vec4(diffuse, base_color.a);
}
@end

@program lambertian lambertian_vs lambertian_fs
