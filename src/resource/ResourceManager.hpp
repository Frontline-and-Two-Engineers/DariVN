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

#ifndef DARIVN_RESOURCEMANAGER_HPP
#define DARIVN_RESOURCEMANAGER_HPP

#include <string>
#include <memory>
#include <unordered_map>

class Shader;
class Texture2D;

class ResourceManager {
public:
    ResourceManager() = delete; // Только статический класс

    // Шейдеры
    static std::shared_ptr<Shader> loadShader(const std::string& name, 
                                              const std::string& vertexPath, 
                                              const std::string& fragmentPath);
    static std::shared_ptr<Shader> getShader(const std::string& name);

    // Текстуры
    static std::shared_ptr<Texture2D> loadTexture(const std::string& name, 
                                                  const std::string& path, 
                                                  bool flipVertically = false);
    static std::shared_ptr<Texture2D> loadTexture(const std::string& path, 
                                                  bool flipVertically = false);
    static std::shared_ptr<Texture2D> getTexture(const std::string& name);

    // Вспомогательная белая текстура 1x1 для рисования цветных плашек и UI без картинок
    static std::shared_ptr<Texture2D> getWhiteTexture();

    // Удаление отдельной текстуры из кэша (для сброса превью сохранений)
    static bool removeTexture(const std::string& name);

    // Очистка всех кэшированных ресурсов
    static void clear();

private:
    static inline std::unordered_map<std::string, std::shared_ptr<Shader>> s_shaders;
    static inline std::unordered_map<std::string, std::shared_ptr<Texture2D>> s_textures;
    static inline std::shared_ptr<Texture2D> s_whiteTexture = nullptr;
};

#endif //DARIVN_RESOURCEMANAGER_HPP
