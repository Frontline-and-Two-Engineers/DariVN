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
