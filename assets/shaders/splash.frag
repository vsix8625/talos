#version 450 core
in vec2 v_pos;
in vec4 v_color;

uniform vec3 u_light_dir;
uniform float u_progress;

out vec4 frag_color;

void main()
{
    float dist = dot(v_pos, v_pos);
    if (dist > 1.0) discard;

    float fill_line = (u_progress * 2.0) - 1.0;
    
    vec3 base_color = v_color.rgb;
    if (v_pos.y > fill_line) {
        base_color *= 0.2; // Unfilled shell look
    } else {
        if (v_pos.y > fill_line - 0.05) {
            base_color += vec3(0.4, 0.2, 0.0); // White-hot glow line
        }
    }

    vec3 normal = normalize(vec3(v_pos, sqrt(1.0 - dist)));
    float diffuse = max(dot(normal, normalize(u_light_dir)), 0.0);
    float ambient = 0.15;
    float lighting = ambient + diffuse;

    float global_alpha = 1.0;
    if (u_progress > 0.8) {
        global_alpha = 1.0 - ((u_progress - 0.8) / 0.2);
    }

    frag_color = vec4(base_color * lighting, v_color.a * global_alpha);
}
