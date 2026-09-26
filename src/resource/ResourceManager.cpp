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

#include "ResourceManager.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Texture2D.hpp"

#include <iostream>

std::shared_ptr<Shader> ResourceManager::loadShader(const std::string& name, 
                                                    const std::string& vertexPath, 
                                                    const std::string& fragmentPath) {
    auto it = s_shaders.find(name);
    if (it != s_shaders.end()) {
        return it->second;
    }

    auto shader = std::make_shared<Shader>();
    if (!shader->loadFromFiles(vertexPath, fragmentPath)) {
        std::cerr << "[ResourceManager] Failed to load shader: " << name << std::endl;
        return nullptr;
    }

    s_shaders[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
    auto it = s_shaders.find(name);
    if (it == s_shaders.end()) {
        std::cerr << "[ResourceManager] Shader not found: " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<Texture2D> ResourceManager::loadTexture(const std::string& name, 
                                                        const std::string& path, 
                                                        bool flipVertically) {
    auto it = s_textures.find(name);
    if (it != s_textures.end()) {
        return it->second;
    }

    auto texture = std::make_shared<Texture2D>();
    if (!texture->loadFromFile(path, flipVertically)) {
        std::cerr << "[ResourceManager] Failed to load texture: " << name << " from " << path << std::endl;
        return nullptr;
    }

    s_textures[name] = texture;
    return texture;
}

std::shared_ptr<Texture2D> ResourceManager::loadTexture(const std::string& path, bool flipVertically) {
    return loadTexture(path, path, flipVertically);
}

std::shared_ptr<Texture2D> ResourceManager::getTexture(const std::string& name) {
    auto it = s_textures.find(name);
    if (it == s_textures.end()) {
        std::cerr << "[ResourceManager] Texture not found: " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<Texture2D> ResourceManager::getWhiteTexture() {
    if (!s_whiteTexture) {
        s_whiteTexture = std::make_shared<Texture2D>();
        const unsigned char whitePixel[] = { 255, 255, 255, 255 };
        s_whiteTexture->loadFromMemory(whitePixel, 1, 1, 4);
    }
    return s_whiteTexture;
}

bool ResourceManager::removeTexture(const std::string& name) {
    auto it = s_textures.find(name);
    if (it != s_textures.end()) {
        s_textures.erase(it);
        return true;
    }
    return false;
}

void ResourceManager::clear() {
    s_shaders.clear();
    s_textures.clear();
    s_whiteTexture.reset();
}
