#version 330 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_frame;

void main() {
    vec4 color = texture(u_frame, v_uv);
    frag_color = vec4(color.rgb * vec3(0.98, 0.97, 0.95), color.a);
}
