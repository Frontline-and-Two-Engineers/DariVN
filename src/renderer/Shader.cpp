#include "Shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    loadFromFiles(vertexPath, fragmentPath);
}

Shader::~Shader() {
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : m_programID(other.m_programID),
      m_uniformLocationCache(std::move(other.m_uniformLocationCache)) {
    other.m_programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        destroy();
        m_programID = other.m_programID;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        other.m_programID = 0;
    }
    return *this;
}

void Shader::destroy() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
    m_uniformLocationCache.clear();
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::ifstream vShaderFile(vertexPath);
    std::ifstream fShaderFile(fragmentPath);

    if (!vShaderFile.is_open()) {
        std::cerr << "[Shader] Error: Failed to open vertex shader file: " << vertexPath << std::endl;
        return false;
    }

    if (!fShaderFile.is_open()) {
        std::cerr << "[Shader] Error: Failed to open fragment shader file: " << fragmentPath << std::endl;
        return false;
    }

    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    return loadFromMemory(vShaderStream.str(), fShaderStream.str());
}

bool Shader::loadFromMemory(const std::string& vertexSrc, const std::string& fragmentSrc) {
    destroy();

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "[Shader] Program Link Error:\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_programID = program;
    return true;
}

unsigned int Shader::compileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::string_view typeName = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "[Shader] Compilation Error (" << typeName << "):\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::bind() const {
    glUseProgram(m_programID);
}

void Shader::unbind() const {
    glUseProgram(0);
}

int Shader::getUniformLocation(std::string_view name) {
    std::string key(name);
    auto it = m_uniformLocationCache.find(key);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    int location = glGetUniformLocation(m_programID, key.c_str());
    if (location == -1) {
        std::cerr << "[Shader] Warning: uniform '" << key << "' not found or unused in shader program " << m_programID << std::endl;
    }
    m_uniformLocationCache[key] = location;
    return location;
}

void Shader::setInt(std::string_view name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(std::string_view name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(std::string_view name, const glm::vec2& value) {
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setVec3(std::string_view name, const glm::vec3& value) {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec4(std::string_view name, const glm::vec4& value) {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::setMat4(std::string_view name, const glm::mat4& matrix) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}
