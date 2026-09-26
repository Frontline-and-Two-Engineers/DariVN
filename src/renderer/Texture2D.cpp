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

#include "Texture2D.hpp"
#include <glad/glad.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture2D::Texture2D(const std::string& path, bool flipVertically) {
    loadFromFile(path, flipVertically);
}

Texture2D::~Texture2D() {
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_textureID(other.m_textureID),
      m_width(other.m_width),
      m_height(other.m_height),
      m_channels(other.m_channels) {
    other.m_textureID = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_channels = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        destroy();
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;

        other.m_textureID = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_channels = 0;
    }
    return *this;
}

void Texture2D::destroy() {
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    m_width = 0;
    m_height = 0;
    m_channels = 0;
}

bool Texture2D::loadFromFile(const std::string& path, bool flipVertically) {
    destroy();

    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "[Texture2D] Failed to load image '" << path << "': " 
                  << stbi_failure_reason() << std::endl;
        return false;
    }

    bool success = loadFromMemory(data, width, height, channels);
    stbi_image_free(data);

    if (success) {
        std::cout << "[Texture2D] Loaded '" << path << "' (" 
                  << width << "x" << height << ", " << channels << " channels)" << std::endl;
    }
    return success;
}

bool Texture2D::loadFromMemory(const unsigned char* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 0) {
        std::cerr << "[Texture2D] Invalid texture data for loadFromMemory" << std::endl;
        return false;
    }

    destroy();

    m_width = width;
    m_height = height;
    m_channels = channels;

    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;

    if (channels == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    } else if (channels == 1) {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // Выравнивание строк по 1 байту (предотвращает артефакты при ширине изображения, не кратной 4)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Параметры наложения и фильтрации для 2D-спрайтов
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture2D::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture2D::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}
