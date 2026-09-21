#version 330 core
in vec2 v_TexCoords;
in vec4 v_Color;
out vec4 FragColor;

uniform sampler2D u_FontAtlas;
uniform vec4 u_TextColor;

void main() {
    float alpha = texture(u_FontAtlas, v_TexCoords).r;
    FragColor = vec4(v_Color.rgb * u_TextColor.rgb, v_Color.a * u_TextColor.a * alpha);
}
