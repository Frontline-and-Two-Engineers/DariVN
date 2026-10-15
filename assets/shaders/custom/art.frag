#version 330 core

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;
uniform vec2 u_Resolution;

// Параметры мягкой живописной стилизации
uniform float u_EdgeThreshold  = 0.16;  // Порог контура (выше = меньше грязи на воде и небе)
uniform float u_EdgeStrength   = 0.25;  // Сила линий (0.2-0.3 дает легкую тушь вместо черной проволоки)
uniform float u_PosterizeSteps = 0.0;   // 0.0 - отключено (убирает полосы на небе), 16-24 для мягких плашек
uniform float u_Saturation     = 1.15;  // Умеренная насыщенность (без ядовитого перенасыщения)
uniform float u_Contrast       = 1.02;  // Мягкий контраст
uniform float u_BrushScale     = 1.25;  // Масштаб мазков кисти (чуть шире = мягче фон)

const int RADIUS = 4;

// Яркость пикселя
float getLuminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// 1. Фильтр Кувахары (эффект акварельных / масляных мазков)
vec3 kuwahara(sampler2D tex, vec2 uv, vec2 stepSize, int radius) {
    float n = float((radius + 1) * (radius + 1));

    vec3 m[4] = vec3[4](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
    vec3 s[4] = vec3[4](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));

    // Сектор 1: верх-лево
    for (int j = -radius; j <= 0; ++j) {
        for (int i = -radius; i <= 0; ++i) {
            vec3 c = texture(tex, uv + vec2(i, j) * stepSize).rgb;
            m[0] += c;
            s[0] += c * c;
        }
    }

    // Сектор 2: верх-право
    for (int j = -radius; j <= 0; ++j) {
        for (int i = 0; i <= radius; ++i) {
            vec3 c = texture(tex, uv + vec2(i, j) * stepSize).rgb;
            m[1] += c;
            s[1] += c * c;
        }
    }

    // Сектор 3: низ-лево
    for (int j = 0; j <= radius; ++j) {
        for (int i = -radius; i <= 0; ++i) {
            vec3 c = texture(tex, uv + vec2(i, j) * stepSize).rgb;
            m[2] += c;
            s[2] += c * c;
        }
    }

    // Сектор 4: низ-право
    for (int j = 0; j <= radius; ++j) {
        for (int i = 0; i <= radius; ++i) {
            vec3 c = texture(tex, uv + vec2(i, j) * stepSize).rgb;
            m[3] += c;
            s[3] += c * c;
        }
    }

    float min_sigma2 = 1e+2;
    vec3 finalColor = m[0] / n;

    for (int k = 0; k < 4; ++k) {
        m[k] /= n;
        s[k] = abs(s[k] / n - m[k] * m[k]);
        float sigma2 = s[k].r + s[k].g + s[k].b;

        if (sigma2 < min_sigma2) {
            min_sigma2 = sigma2;
            finalColor = m[k];
        }
    }

    return finalColor;
}

// Получение сглаженной яркости в точке (игнорирует мелкий шум сенсора и мелкую рябь)
float getSmoothLuma(sampler2D tex, vec2 uv, vec2 step) {
    vec3 c1 = texture(tex, uv).rgb;
    vec3 c2 = texture(tex, uv + vec2(step.x, 0.0)).rgb;
    vec3 c3 = texture(tex, uv - vec2(step.x, 0.0)).rgb;
    vec3 c4 = texture(tex, uv + vec2(0.0, step.y)).rgb;
    vec3 c5 = texture(tex, uv - vec2(0.0, step.y)).rgb;
    return getLuminance((c1 * 2.0 + c2 + c3 + c4 + c5) / 6.0);
}

// 2. Детектор крупных контуров (игнорирует мелкую рябь и шум)
float detectEdges(sampler2D tex, vec2 uv, vec2 texelSize) {
    if (u_EdgeStrength <= 0.001) return 0.0;

    vec2 d = texelSize * 2.0; // более широкий шаг для отсечения шума

    float tl = getSmoothLuma(tex, uv + vec2(-d.x, -d.y), texelSize);
    float t  = getSmoothLuma(tex, uv + vec2(  0.0, -d.y), texelSize);
    float tr = getSmoothLuma(tex, uv + vec2( d.x, -d.y), texelSize);
    float l  = getSmoothLuma(tex, uv + vec2(-d.x,  0.0), texelSize);
    float r  = getSmoothLuma(tex, uv + vec2( d.x,  0.0), texelSize);
    float bl = getSmoothLuma(tex, uv + vec2(-d.x,  d.y), texelSize);
    float b  = getSmoothLuma(tex, uv + vec2(  0.0,  d.y), texelSize);
    float br = getSmoothLuma(tex, uv + vec2( d.x,  d.y), texelSize);

    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;

    float edge = sqrt(gx * gx + gy * gy);
    return smoothstep(u_EdgeThreshold, u_EdgeThreshold + 0.25, edge);
}

void main() {
    vec2 res = max(u_Resolution, vec2(1.0));
    vec2 texelSize = 1.0 / res;

    float baseAlpha = texture(u_Texture, v_TexCoords).a;
    if (baseAlpha < 0.005) {
        discard;
    }

    // 1. Живописное сглаживание Кувахары с масштабируемым мазком
    vec2 brushStep = texelSize * max(u_BrushScale, 0.5);
    vec3 painted = kuwahara(u_Texture, v_TexCoords, brushStep, RADIUS);

    // 2. Постеризация (если включена > 0; по умолчанию выключена для гладкого неба)
    if (u_PosterizeSteps > 0.5) {
        painted = floor(painted * u_PosterizeSteps + 0.5) / u_PosterizeSteps;
    }

    // 3. Выделение только крупных силуэтов (перила, скалы, горизонт)
    float edge = detectEdges(u_Texture, v_TexCoords, texelSize);

    // 4. Мягкая растушевка контуров (без черной проволоки)
    vec3 artColor = mix(painted, painted * 0.35, edge * u_EdgeStrength);

    // 5. Мягкая цветокоррекция (пастельные тона)
    float lum = getLuminance(artColor);
    vec3 graded = mix(vec3(lum), artColor, u_Saturation);
    graded = (graded - 0.5) * u_Contrast + 0.5;

    FragColor = vec4(clamp(graded, 0.0, 1.0), baseAlpha) * u_Color;
}
