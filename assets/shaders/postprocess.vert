#version 330 core
layout (location = 0) in vec2 a_Position; // [-1, 1]
layout (location = 1) in vec2 a_TexCoords; // [0, 1]

out vec2 v_TexCoords;

void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
