#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;
uniform vec2 u_Resolution;

uniform float u_Speed;
uniform float u_Intensity;
uniform float u_ColorOffset;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = v_TexCoords;

    // Смещение полос по горизонтали при скачках шума
    float sliceY = floor(uv.y * 28.0);
    float noise = rand(vec2(sliceY, floor(u_Time * u_Speed * 8.0)));
    if (noise > 0.75) {
        uv.x += (rand(vec2(u_Time * 5.0, sliceY)) - 0.5) * u_Intensity;
    }

    // Хроматическая аберрация (смещение цветовых каналов R и B)
    float r = texture(u_Texture, uv + vec2(u_ColorOffset, 0.0)).r;
    float g = texture(u_Texture, uv).g;
    float b = texture(u_Texture, uv - vec2(u_ColorOffset, 0.0)).b;
    float a = texture(u_Texture, uv).a;

    if (a < 0.005) {
        discard;
    }

    FragColor = vec4(r, g, b, a) * u_Color;
}
