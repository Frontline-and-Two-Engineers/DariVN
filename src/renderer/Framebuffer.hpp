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
