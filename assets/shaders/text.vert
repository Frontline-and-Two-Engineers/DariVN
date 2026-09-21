#version 330 core
layout (location = 0) in vec4 a_Vertex; // <vec2 position, vec2 texCoords>
layout (location = 1) in vec4 a_Color;

out vec2 v_TexCoords;
out vec4 v_Color;

uniform mat4 u_Projection;

void main() {
    v_TexCoords = a_Vertex.zw;
    v_Color = a_Color;
    gl_Position = u_Projection * vec4(a_Vertex.xy, 0.0, 1.0);
}
