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

#ifndef DARIVN_TEXTURE2D_HPP
#define DARIVN_TEXTURE2D_HPP

#include <string>

class Texture2D {
public:
    Texture2D() = default;
    explicit Texture2D(const std::string& path, bool flipVertically = false);
    ~Texture2D();

    // Запрет копирования (RAII для OpenGL текстуры)
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Move-семантика
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // Загрузка
    bool loadFromFile(const std::string& path, bool flipVertically = false);
    bool loadFromMemory(const unsigned char* data, int width, int height, int channels);

    void bind(unsigned int slot = 0) const;
    void unbind() const;

    [[nodiscard]] unsigned int getID() const { return m_textureID; }
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    [[nodiscard]] int getChannels() const { return m_channels; }
    [[nodiscard]] bool isValid() const { return m_textureID != 0; }

private:
    void destroy();

private:
    unsigned int m_textureID = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};

#endif //DARIVN_TEXTURE2D_HPP
