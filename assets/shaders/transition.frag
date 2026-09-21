#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_PrevTexture;
uniform sampler2D u_NextTexture;
uniform float     u_Progress;       // 0.0 -> 1.0
uniform int       u_TransitionType; // 0 = Crossfade, 1 = Fade to Black, 2 = Dissolve

// Быстрый псевдослучайный шум для Dissolve (не требует внешних текстур шума)
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec4 prevCol = texture(u_PrevTexture, v_TexCoords);
    vec4 nextCol = texture(u_NextTexture, v_TexCoords);

    if (u_TransitionType == 0) {
        // 1. Crossfade: плавное линейное смешивание двух сцен
        FragColor = mix(prevCol, nextCol, clamp(u_Progress, 0.0, 1.0));
    } else if (u_TransitionType == 1) {
        // 2. Fade to Black: уход старой сцены в темноту и проявление новой
        if (u_Progress < 0.5) {
            float t = u_Progress * 2.0;
            FragColor = mix(prevCol, vec4(0.0, 0.0, 0.0, 1.0), clamp(t, 0.0, 1.0));
        } else {
            float t = (u_Progress - 0.5) * 2.0;
            FragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), nextCol, clamp(t, 0.0, 1.0));
        }
    } else if (u_TransitionType == 2) {
        // 3. Dissolve: стильное пиксельно-шумовое растворение по маске
        float noise = hash(floor(v_TexCoords * vec2(320.0, 180.0)));
        float edge = smoothstep(u_Progress - 0.08, u_Progress + 0.08, noise);
        FragColor = mix(nextCol, prevCol, edge);
    } else {
        FragColor = mix(prevCol, nextCol, u_Progress);
    }
}
