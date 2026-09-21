#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_ScreenTexture;
uniform vec2 u_ScreenResolution;

uniform float u_BlurStrength;       // 0.0 = нет размытия, 1.0+ = мягкое киноразмытие
uniform float u_VignetteIntensity;  // 0.0 = нет виньетки, 0.7-1.0 = драматичное затемнение углов
uniform float u_SepiaTone;          // 0.0 = цвет, 1.0 = теплый эффект воспоминаний/сепии
uniform vec4  u_ColorTint;          // цветовой множитель (холодная ночь, закат и т.д.)

vec4 sampleBlur(vec2 uv, float strength) {
    if (strength <= 0.001) {
        return texture(u_ScreenTexture, uv);
    }

    vec2 texelSize = (1.0 / u_ScreenResolution) * strength;
    vec4 color = vec4(0.0);

    // 9-точечная выборка с гауссовыми весами
    vec2 offsets[9] = vec2[](
        vec2( 0.0,  0.0),
        vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0),
        vec2(-1.0,  0.0),                    vec2( 1.0,  0.0),
        vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0)
    );

    float weights[9] = float[](
        0.25,
        0.0625, 0.125, 0.0625,
        0.125,         0.125,
        0.0625, 0.125, 0.0625
    );

    for (int i = 0; i < 9; i++) {
        color += texture(u_ScreenTexture, uv + offsets[i] * texelSize * 2.8) * weights[i];
    }
    return color;
}

void main() {
    vec4 baseColor = sampleBlur(v_TexCoords, u_BlurStrength);

    // 1. Цветовой оттенок (Color Tint)
    baseColor.rgb *= u_ColorTint.rgb;

    // 2. Сепия / Воспоминания (Sepia filter)
    if (u_SepiaTone > 0.001) {
        vec3 sepia;
        sepia.r = dot(baseColor.rgb, vec3(0.393, 0.769, 0.189));
        sepia.g = dot(baseColor.rgb, vec3(0.349, 0.686, 0.168));
        sepia.b = dot(baseColor.rgb, vec3(0.272, 0.534, 0.131));
        baseColor.rgb = mix(baseColor.rgb, sepia, clamp(u_SepiaTone, 0.0, 1.0));
    }

    // 3. Виньетирование (Vignette)
    if (u_VignetteIntensity > 0.001) {
        vec2 uv = (v_TexCoords - 0.5) * 2.0;
        float dist = length(uv);
        float vignette = smoothstep(1.5 - u_VignetteIntensity * 0.7, 0.4, dist);
        baseColor.rgb *= mix(1.0, vignette, clamp(u_VignetteIntensity, 0.0, 1.0));
    }

    FragColor = baseColor;
}
