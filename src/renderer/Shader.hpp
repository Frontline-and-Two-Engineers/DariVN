/*
 * DariVN - Visual Novel Engine
 *
 * Copyright (C) 2026 Arsenii Soloviov <arsenii.soloviov.02@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef DARIVN_SHADER_HPP
#define DARIVN_SHADER_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <glm/glm.hpp>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Запрещаем копирование, чтобы избежать двойного вызова glDeleteProgram
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Разрешаем перемещение (Move semantics)
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Загрузка и компиляция
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromMemory(const std::string& vertexSrc, const std::string& fragmentSrc);

    void bind() const;
    void unbind() const;

    [[nodiscard]] unsigned int getID() const { return m_programID; }
    [[nodiscard]] bool isValid() const { return m_programID != 0; }

    // Установка Uniform-переменных
    void setInt(std::string_view name, int value);
    void setFloat(std::string_view name, float value);
    void setVec2(std::string_view name, const glm::vec2& value);
    void setVec3(std::string_view name, const glm::vec3& value);
    void setVec4(std::string_view name, const glm::vec4& value);
    void setMat4(std::string_view name, const glm::mat4& matrix);

private:
    int getUniformLocation(std::string_view name);
    static unsigned int compileShader(unsigned int type, const std::string& source);
    void destroy();

private:
    unsigned int m_programID = 0;
    std::unordered_map<std::string, int> m_uniformLocationCache;
};

#endif //DARIVN_SHADER_HPP
