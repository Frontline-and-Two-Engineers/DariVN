#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;
uniform vec2 u_Resolution;

uniform float u_Speed;
uniform float u_Frequency;
uniform float u_Amplitude;

void main() {
    float wave = sin(v_TexCoords.y * u_Frequency + u_Time * u_Speed) * u_Amplitude;
    vec2 uv = vec2(v_TexCoords.x + wave, v_TexCoords.y);
    vec4 tex = texture(u_Texture, uv);
    if (tex.a < 0.005) {
        discard;
    }
    FragColor = tex * u_Color;
}
