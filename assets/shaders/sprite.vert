#version 330 core
layout (location = 0) in vec4 a_Vertex; // <vec2 position, vec2 texCoords>

out vec2 v_TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_Model;

void main() {
    v_TexCoords = a_Vertex.zw;
    gl_Position = u_Projection * u_Model * vec4(a_Vertex.xy, 0.0, 1.0);
}
