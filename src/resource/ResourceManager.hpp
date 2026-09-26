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
