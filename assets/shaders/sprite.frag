#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;

uniform int   u_Silhouette;        // 1 = режим сплошного силуэта
uniform vec4  u_SilhouetteColor;   // цвет силуэта (обычно тёмно-синий/чёрный)
uniform float u_Flash;             // 0.0 .. 1.0 (вспышка белым цветом)
uniform float u_Sepia;             // 0.0 .. 1.0 (индивидуальная сепия персонажа)

void main() {
    vec4 tex = texture(u_Texture, v_TexCoords);
    if (tex.a < 0.005) {
        discard;
    }

    // 1. Режим силуэта (заполняет форму текстуры заданным цветом, сохраняя альфа-контур)
    if (u_Silhouette == 1) {
        FragColor = vec4(u_SilhouetteColor.rgb, tex.a * u_SilhouetteColor.a * u_Color.a);
        return;
    }

    vec4 finalColor = tex * u_Color;

    // 2. Сепия персонажа
    if (u_Sepia > 0.001) {
        vec3 sepia;
        sepia.r = dot(finalColor.rgb, vec3(0.393, 0.769, 0.189));
        sepia.g = dot(finalColor.rgb, vec3(0.349, 0.686, 0.168));
        sepia.b = dot(finalColor.rgb, vec3(0.272, 0.534, 0.131));
        finalColor.rgb = mix(finalColor.rgb, sepia, clamp(u_Sepia, 0.0, 1.0));
    }

    // 3. Вспышка белым цветом (Flash)
    if (u_Flash > 0.001) {
        finalColor.rgb = mix(finalColor.rgb, vec3(1.0), clamp(u_Flash, 0.0, 1.0));
    }

    FragColor = finalColor;
}
