#pragma once

#ifndef DARIVN_FRAMEBUFFER_HPP
#define DARIVN_FRAMEBUFFER_HPP

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    // Привязка внеэкранного буфера для отрисовки в него
    void bind();

    // Отвязка и возврат к системному буферу кадра экрана
    void unbind(int screenWidth, int screenHeight);

    // Очистка содержимого буфера заданным цветом
    void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    // Изменение размера буфера
    void resize(int width, int height);

    [[nodiscard]] unsigned int getColorTexture() const { return m_colorTexture; }
    [[nodiscard]] unsigned int getFBO() const { return m_fbo; }
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    [[nodiscard]] bool isValid() const { return m_fbo != 0 && m_colorTexture != 0; }

private:
    void invalidate();
    void cleanup();

private:
    unsigned int m_fbo = 0;
    unsigned int m_colorTexture = 0;
    int m_width = 0;
    int m_height = 0;
};

#endif //DARIVN_FRAMEBUFFER_HPP
