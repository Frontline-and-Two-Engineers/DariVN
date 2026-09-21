#include "TextRenderer.hpp"
#include "Shader.hpp"
#include "resource/ResourceManager.hpp"

#include <glad/glad.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

static uint32_t nextUtf8Char(const char*& ptr, const char* end) {
    if (ptr >= end) return 0;
    auto c = static_cast<unsigned char>(*ptr++);
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0) {
        if (ptr >= end) return 0;
        auto c2 = static_cast<unsigned char>(*ptr++);
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    }
    if ((c & 0xF0) == 0xE0) {
        if (ptr + 1 >= end) return 0;
        auto c2 = static_cast<unsigned char>(*ptr++);
        auto c3 = static_cast<unsigned char>(*ptr++);
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    if ((c & 0xF8) == 0xF0) {
        if (ptr + 2 >= end) return 0;
        auto c2 = static_cast<unsigned char>(*ptr++);
        auto c3 = static_cast<unsigned char>(*ptr++);
        auto c4 = static_cast<unsigned char>(*ptr++);
        return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
    }
    return c;
}

TextRenderer::TextRenderer() {
    initRenderData();
}

TextRenderer::~TextRenderer() {
    if (m_fontTexture != 0) {
        glDeleteTextures(1, &m_fontTexture);
        m_fontTexture = 0;
    }
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
}

void TextRenderer::initRenderData() {
    std::string vertPath = "assets/shaders/text.vert";
    std::string fragPath = "assets/shaders/text.frag";
    if (!std::filesystem::exists(vertPath)) {
        if (std::filesystem::exists("../" + vertPath)) {
            vertPath = "../" + vertPath;
            fragPath = "../" + fragPath;
        } else if (std::filesystem::exists("../../" + vertPath)) {
            vertPath = "../../" + vertPath;
            fragPath = "../../" + fragPath;
        }
    }
    m_shader = ResourceManager::loadShader("text", vertPath, fragPath);

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // 8 float на вершину: x, y, u, v, r, g, b, a
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 6 * 1024, nullptr, GL_DYNAMIC_DRAW);

    // Location 0: a_Vertex (vec2 pos, vec2 texCoords)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));

    // Location 1: a_Color (vec4 color)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(4 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool TextRenderer::loadFont(const std::string& fontPath, float fontSize) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[TextRenderer] Failed to open font file: " << fontPath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> fontBuffer(size);
    if (!file.read(reinterpret_cast<char*>(fontBuffer.data()), size)) {
        std::cerr << "[TextRenderer] Failed to read font file: " << fontPath << std::endl;
        return false;
    }

    m_fontSize = fontSize;
    m_lineHeight = fontSize * 1.25f;

    const int atlasWidth = 1024;
    const int atlasHeight = 1024;
    std::vector<unsigned char> atlasBitmap(atlasWidth * atlasHeight, 0);

    stbtt_pack_context spc;
    if (!stbtt_PackBegin(&spc, atlasBitmap.data(), atlasWidth, atlasHeight, 0, 1, nullptr)) {
        std::cerr << "[TextRenderer] stbtt_PackBegin failed" << std::endl;
        return false;
    }

    stbtt_PackSetOversampling(&spc, 2, 2);

    stbtt_pack_range ranges[2];
    // 1. Диапазон латиницы (32..126)
    ranges[0].font_size = fontSize;
    ranges[0].first_unicode_codepoint_in_range = 32;
    ranges[0].array_of_unicode_codepoints = nullptr;
    ranges[0].num_chars = 95;
    std::vector<stbtt_packedchar> latinPacked(95);
    ranges[0].chardata_for_range = latinPacked.data();

    // 2. Диапазон кириллицы (0x0400..0x04FF)
    ranges[1].font_size = fontSize;
    ranges[1].first_unicode_codepoint_in_range = 0x0400;
    ranges[1].array_of_unicode_codepoints = nullptr;
    ranges[1].num_chars = 256;
    std::vector<stbtt_packedchar> cyrillicPacked(256);
    ranges[1].chardata_for_range = cyrillicPacked.data();

    if (!stbtt_PackFontRanges(&spc, fontBuffer.data(), 0, ranges, 2)) {
        std::cerr << "[TextRenderer] stbtt_PackFontRanges failed to fit glyphs" << std::endl;
        stbtt_PackEnd(&spc);
        return false;
    }

    stbtt_PackEnd(&spc);

    // Заполняем кэш метрик глифов
    m_latinGlyphs.clear();
    m_latinGlyphs.reserve(95);
    for (int i = 0; i < 95; ++i) {
        const auto& pc = latinPacked[i];
        CharacterGlyph g{};
        g.x0 = pc.xoff;
        g.y0 = pc.yoff;
        g.x1 = pc.xoff2;
        g.y1 = pc.yoff2;
        g.s0 = static_cast<float>(pc.x0) / static_cast<float>(atlasWidth);
        g.t0 = static_cast<float>(pc.y0) / static_cast<float>(atlasHeight);
        g.s1 = static_cast<float>(pc.x1) / static_cast<float>(atlasWidth);
        g.t1 = static_cast<float>(pc.y1) / static_cast<float>(atlasHeight);
        g.xAdvance = pc.xadvance;
        m_latinGlyphs.push_back(g);
    }

    m_cyrillicGlyphs.clear();
    m_cyrillicGlyphs.reserve(256);
    for (int i = 0; i < 256; ++i) {
        const auto& pc = cyrillicPacked[i];
        CharacterGlyph g{};
        g.x0 = pc.xoff;
        g.y0 = pc.yoff;
        g.x1 = pc.xoff2;
        g.y1 = pc.yoff2;
        g.s0 = static_cast<float>(pc.x0) / static_cast<float>(atlasWidth);
        g.t0 = static_cast<float>(pc.y0) / static_cast<float>(atlasHeight);
        g.s1 = static_cast<float>(pc.x1) / static_cast<float>(atlasWidth);
        g.t1 = static_cast<float>(pc.y1) / static_cast<float>(atlasHeight);
        g.xAdvance = pc.xadvance;
        m_cyrillicGlyphs.push_back(g);
    }

    // Создаем OpenGL текстуру шрифта (1 канал яркости GL_R8)
    if (m_fontTexture != 0) {
        glDeleteTextures(1, &m_fontTexture);
    }

    glGenTextures(1, &m_fontTexture);
    glBindTexture(GL_TEXTURE_2D, m_fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[TextRenderer] Loaded font '" << fontPath << "' (" << fontSize << "px, Latin + Cyrillic)" << std::endl;
    return true;
}

const CharacterGlyph* TextRenderer::getGlyph(uint32_t codepoint) const {
    if (codepoint == 0x2019 || codepoint == 0x02BC || codepoint == 0x2018 || codepoint == '`') {
        codepoint = '\'';
    }
    if (codepoint >= 32 && codepoint <= 126) {
        size_t idx = codepoint - 32;
        if (idx < m_latinGlyphs.size()) return &m_latinGlyphs[idx];
    } else if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
        size_t idx = codepoint - 0x0400;
        if (idx < m_cyrillicGlyphs.size()) return &m_cyrillicGlyphs[idx];
    }
    return nullptr;
}

void TextRenderer::setProjection(const glm::mat4& projection) {
    if (m_shader) {
        m_shader->bind();
        m_shader->setMat4("u_Projection", projection);
    }
}

// -----------------------------------------------------------------------------
// Вспомогательные структуры и функции для разбора rich-text форматирования
// -----------------------------------------------------------------------------
namespace {

struct FormattingState {
    std::vector<glm::vec4> colorStack;
    std::vector<float> scaleStack;
    std::vector<float> alphaStack;
    int boldDepth = 0;
    int italicDepth = 0;

    void reset() {
        colorStack.clear();
        scaleStack.clear();
        alphaStack.clear();
        boldDepth = 0;
        italicDepth = 0;
    }

    [[nodiscard]] glm::vec4 currentColor(const glm::vec4& defaultColor) const {
        glm::vec4 c = !colorStack.empty() ? colorStack.back() : defaultColor;
        if (!alphaStack.empty()) {
            c.a *= alphaStack.back();
        }
        return c;
    }

    [[nodiscard]] float currentScale(float baseScale) const {
        if (!scaleStack.empty()) {
            return baseScale * scaleStack.back();
        }
        return baseScale;
    }

    [[nodiscard]] bool isBold() const { return boldDepth > 0; }
    [[nodiscard]] bool isItalic() const { return italicDepth > 0; }
};

static bool parseHex(std::string_view s, glm::vec4& outColor) {
    if (s.empty() || s[0] != '#') return false;
    std::string_view hex = s.substr(1);
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    if (hex.size() == 3) {
        int r = hexVal(hex[0]), g = hexVal(hex[1]), b = hexVal(hex[2]);
        if (r < 0 || g < 0 || b < 0) return false;
        outColor = glm::vec4((r * 17) / 255.0f, (g * 17) / 255.0f, (b * 17) / 255.0f, 1.0f);
        return true;
    }
    if (hex.size() == 4) {
        int r = hexVal(hex[0]), g = hexVal(hex[1]), b = hexVal(hex[2]), a = hexVal(hex[3]);
        if (r < 0 || g < 0 || b < 0 || a < 0) return false;
        outColor = glm::vec4((r * 17) / 255.0f, (g * 17) / 255.0f, (b * 17) / 255.0f, (a * 17) / 255.0f);
        return true;
    }
    if (hex.size() == 6) {
        int r1 = hexVal(hex[0]), r2 = hexVal(hex[1]);
        int g1 = hexVal(hex[2]), g2 = hexVal(hex[3]);
        int b1 = hexVal(hex[4]), b2 = hexVal(hex[5]);
        if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0) return false;
        outColor = glm::vec4((r1 * 16 + r2) / 255.0f, (g1 * 16 + g2) / 255.0f, (b1 * 16 + b2) / 255.0f, 1.0f);
        return true;
    }
    if (hex.size() == 8) {
        int r1 = hexVal(hex[0]), r2 = hexVal(hex[1]);
        int g1 = hexVal(hex[2]), g2 = hexVal(hex[3]);
        int b1 = hexVal(hex[4]), b2 = hexVal(hex[5]);
        int a1 = hexVal(hex[6]), a2 = hexVal(hex[7]);
        if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0 || a1 < 0 || a2 < 0) return false;
        outColor = glm::vec4((r1 * 16 + r2) / 255.0f, (g1 * 16 + g2) / 255.0f, (b1 * 16 + b2) / 255.0f, (a1 * 16 + a2) / 255.0f);
        return true;
    }
    return false;
}

static bool parseColor(std::string_view s, glm::vec4& outColor) {
    if (s.empty()) return false;
    if (s[0] == '#') return parseHex(s, outColor);
    std::string lower(s);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower == "red") { outColor = glm::vec4(0.94f, 0.27f, 0.27f, 1.0f); return true; }
    if (lower == "green") { outColor = glm::vec4(0.13f, 0.77f, 0.37f, 1.0f); return true; }
    if (lower == "blue") { outColor = glm::vec4(0.23f, 0.51f, 0.96f, 1.0f); return true; }
    if (lower == "yellow") { outColor = glm::vec4(0.92f, 0.70f, 0.03f, 1.0f); return true; }
    if (lower == "orange") { outColor = glm::vec4(0.98f, 0.45f, 0.09f, 1.0f); return true; }
    if (lower == "gold") { outColor = glm::vec4(0.96f, 0.62f, 0.04f, 1.0f); return true; }
    if (lower == "cyan") { outColor = glm::vec4(0.02f, 0.71f, 0.83f, 1.0f); return true; }
    if (lower == "magenta" || lower == "pink") { outColor = glm::vec4(0.93f, 0.28f, 0.60f, 1.0f); return true; }
    if (lower == "purple" || lower == "violet") { outColor = glm::vec4(0.66f, 0.33f, 0.97f, 1.0f); return true; }
    if (lower == "white") { outColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); return true; }
    if (lower == "black") { outColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); return true; }
    if (lower == "gray" || lower == "grey") { outColor = glm::vec4(0.61f, 0.64f, 0.69f, 1.0f); return true; }
    if (lower == "silver") { outColor = glm::vec4(0.82f, 0.84f, 0.86f, 1.0f); return true; }
    return false;
}

static bool applyFormattingTag(std::string_view tag, FormattingState& state) {
    if (tag.empty()) return false;

    // Закрывающие теги
    if (tag == "/b") {
        if (state.boldDepth > 0) state.boldDepth--;
        return true;
    }
    if (tag == "/i") {
        if (state.italicDepth > 0) state.italicDepth--;
        return true;
    }
    if (tag == "/color" || tag == "/#" || tag == "/c") {
        if (!state.colorStack.empty()) state.colorStack.pop_back();
        return true;
    }
    if (tag == "/scale" || tag == "/size" || tag == "/s") {
        if (!state.scaleStack.empty()) state.scaleStack.pop_back();
        return true;
    }
    if (tag == "/alpha" || tag == "/a") {
        if (!state.alphaStack.empty()) state.alphaStack.pop_back();
        return true;
    }
    if (tag == "reset" || tag == "r") {
        state.reset();
        return true;
    }

    // Открывающие простые теги
    if (tag == "b") {
        state.boldDepth++;
        return true;
    }
    if (tag == "i") {
        state.italicDepth++;
        return true;
    }

    // Теги с параметрами
    if (tag.starts_with("color=") || tag.starts_with("color:")) {
        std::string_view val = tag.substr(6);
        glm::vec4 c;
        if (parseColor(val, c)) {
            state.colorStack.push_back(c);
            return true;
        }
    } else if (tag.starts_with('#')) {
        glm::vec4 c;
        if (parseHex(tag, c)) {
            state.colorStack.push_back(c);
            return true;
        }
    } else if (tag.starts_with("scale=") || tag.starts_with("size=")) {
        std::string_view val = (tag[1] == 'c') ? tag.substr(6) : tag.substr(5);
        try {
            float s = std::stof(std::string(val));
            state.scaleStack.push_back(std::max(0.1f, s));
            return true;
        } catch (...) {}
    } else if (tag.starts_with("alpha=")) {
        std::string_view val = tag.substr(6);
        try {
            float a = std::stof(std::string(val));
            state.alphaStack.push_back(std::clamp(a, 0.0f, 1.0f));
            return true;
        } catch (...) {}
    } else if (tag.starts_with("w=") || tag == "w") {
        // Тег паузы печатной машинки - для рендерера валиден, поглощается
        return true;
    }

    return false;
}

} // anonymous namespace

std::string TextRenderer::stripTags(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{') {
            if (i + 1 < text.size() && text[i + 1] == '{') {
                result += '{';
                i += 2;
                continue;
            }
            size_t close = text.find('}', i + 1);
            if (close != std::string_view::npos) {
                i = close + 1;
                continue;
            }
        } else if (text[i] == '}') {
            if (i + 1 < text.size() && text[i + 1] == '}') {
                result += '}';
                i += 2;
                continue;
            }
        }
        result += text[i++];
    }
    return result;
}

void TextRenderer::renderText(std::string_view text, float x, float y, float scale, glm::vec4 color) {
    if (text.empty() || m_fontTexture == 0 || !m_shader) return;

    m_shader->bind();
    // Глобальный тинт устанавливаем в 1.0f, а точные цвета передаем через атрибуты вершин a_Color
    m_shader->setVec4("u_TextColor", glm::vec4(1.0f));
    m_shader->setInt("u_FontAtlas", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fontTexture);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float curX = x;
    float curY = y + m_fontSize * scale; // baseline
    float startX = x;

    std::vector<float> vertices;
    vertices.reserve(text.size() * 48);

    FormattingState state;

    const char* ptr = text.data();
    const char* end = ptr + text.size();

    while (ptr < end) {
        if (*ptr == '{') {
            if (ptr + 1 < end && *(ptr + 1) == '{') {
                // Экранированный {{ -> рендерим обычный '{'
                ptr += 2;
                uint32_t cp = '{';
                const auto* glyph = getGlyph(cp);
                if (glyph) {
                    float effScale = state.currentScale(scale);
                    glm::vec4 effColor = state.currentColor(color);
                    float xpos = curX + glyph->x0 * effScale;
                    float ypos = curY + glyph->y0 * effScale;
                    float w = (glyph->x1 - glyph->x0) * effScale;
                    float h = (glyph->y1 - glyph->y0) * effScale;
                    float s0 = glyph->s0, t0 = glyph->t0, s1 = glyph->s1, t1 = glyph->t1;
                    float slant = state.isItalic() ? (0.20f * h) : 0.0f;
                    float r = effColor.r, g = effColor.g, b = effColor.b, a = effColor.a;

                    float quad[] = {
                        xpos,             ypos + h, s0, t1, r, g, b, a,
                        xpos + w + slant, ypos,     s1, t0, r, g, b, a,
                        xpos + slant,     ypos,     s0, t0, r, g, b, a,
                        xpos,             ypos + h, s0, t1, r, g, b, a,
                        xpos + w,         ypos + h, s1, t1, r, g, b, a,
                        xpos + w + slant, ypos,     s1, t0, r, g, b, a
                    };
                    vertices.insert(vertices.end(), quad, quad + 48);
                    curX += glyph->xAdvance * effScale;
                }
                continue;
            }

            const char* close = ptr + 1;
            while (close < end && *close != '}' && *close != '\n') close++;
            if (close < end && *close == '}') {
                std::string_view tag(ptr + 1, close - (ptr + 1));
                if (applyFormattingTag(tag, state)) {
                    ptr = close + 1;
                    continue;
                }
            }
        } else if (*ptr == '}') {
            if (ptr + 1 < end && *(ptr + 1) == '}') {
                ptr += 2;
                uint32_t cp = '}';
                const auto* glyph = getGlyph(cp);
                if (glyph) {
                    float effScale = state.currentScale(scale);
                    glm::vec4 effColor = state.currentColor(color);
                    float xpos = curX + glyph->x0 * effScale;
                    float ypos = curY + glyph->y0 * effScale;
                    float w = (glyph->x1 - glyph->x0) * effScale;
                    float h = (glyph->y1 - glyph->y0) * effScale;
                    float s0 = glyph->s0, t0 = glyph->t0, s1 = glyph->s1, t1 = glyph->t1;
                    float slant = state.isItalic() ? (0.20f * h) : 0.0f;
                    float r = effColor.r, g = effColor.g, b = effColor.b, a = effColor.a;

                    float quad[] = {
                        xpos,             ypos + h, s0, t1, r, g, b, a,
                        xpos + w + slant, ypos,     s1, t0, r, g, b, a,
                        xpos + slant,     ypos,     s0, t0, r, g, b, a,
                        xpos,             ypos + h, s0, t1, r, g, b, a,
                        xpos + w,         ypos + h, s1, t1, r, g, b, a,
                        xpos + w + slant, ypos,     s1, t0, r, g, b, a
                    };
                    vertices.insert(vertices.end(), quad, quad + 48);
                    curX += glyph->xAdvance * effScale;
                }
                continue;
            }
        }

        uint32_t cp = nextUtf8Char(ptr, end);
        if (cp == '\n') {
            curX = startX;
            curY += m_lineHeight * state.currentScale(scale);
            continue;
        }

        const auto* glyph = getGlyph(cp);
        if (!glyph) {
            glyph = getGlyph(' ');
            if (glyph) curX += glyph->xAdvance * state.currentScale(scale);
            continue;
        }

        float effScale = state.currentScale(scale);
        glm::vec4 effColor = state.currentColor(color);

        float xpos = curX + glyph->x0 * effScale;
        float ypos = curY + glyph->y0 * effScale;
        float w = (glyph->x1 - glyph->x0) * effScale;
        float h = (glyph->y1 - glyph->y0) * effScale;

        float s0 = glyph->s0;
        float t0 = glyph->t0;
        float s1 = glyph->s1;
        float t1 = glyph->t1;

        float slant = state.isItalic() ? (0.20f * h) : 0.0f;
        float r = effColor.r, g = effColor.g, b = effColor.b, a = effColor.a;

        float quadVertices[] = {
            xpos,             ypos + h, s0, t1, r, g, b, a,
            xpos + w + slant, ypos,     s1, t0, r, g, b, a,
            xpos + slant,     ypos,     s0, t0, r, g, b, a,

            xpos,             ypos + h, s0, t1, r, g, b, a,
            xpos + w,         ypos + h, s1, t1, r, g, b, a,
            xpos + w + slant, ypos,     s1, t0, r, g, b, a
        };
        vertices.insert(vertices.end(), quadVertices, quadVertices + 48);

        // Faux-bold: второй проход отрисовки со смещением
        if (state.isBold()) {
            float bx = 1.2f * effScale;
            float boldVertices[] = {
                xpos + bx,             ypos + h, s0, t1, r, g, b, a,
                xpos + bx + w + slant, ypos,     s1, t0, r, g, b, a,
                xpos + bx + slant,     ypos,     s0, t0, r, g, b, a,

                xpos + bx,             ypos + h, s0, t1, r, g, b, a,
                xpos + bx + w,         ypos + h, s1, t1, r, g, b, a,
                xpos + bx + w + slant, ypos,     s1, t0, r, g, b, a
            };
            vertices.insert(vertices.end(), boldVertices, boldVertices + 48);
        }

        curX += glyph->xAdvance * effScale;
    }

    if (!vertices.empty()) {
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 8));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

glm::vec2 TextRenderer::measureText(std::string_view text, float scale) const {
    if (text.empty() || m_fontTexture == 0) return {0.0f, 0.0f};

    float maxLineWidth = 0.0f;
    float curLineWidth = 0.0f;
    int lineCount = 1;

    FormattingState state;

    const char* ptr = text.data();
    const char* end = ptr + text.size();

    while (ptr < end) {
        if (*ptr == '{') {
            if (ptr + 1 < end && *(ptr + 1) == '{') {
                ptr += 2;
                uint32_t cp = '{';
                const auto* glyph = getGlyph(cp);
                if (glyph) curLineWidth += glyph->xAdvance * state.currentScale(scale);
                continue;
            }

            const char* close = ptr + 1;
            while (close < end && *close != '}' && *close != '\n') close++;
            if (close < end && *close == '}') {
                std::string_view tag(ptr + 1, close - (ptr + 1));
                if (applyFormattingTag(tag, state)) {
                    ptr = close + 1;
                    continue;
                }
            }
        } else if (*ptr == '}') {
            if (ptr + 1 < end && *(ptr + 1) == '}') {
                ptr += 2;
                uint32_t cp = '}';
                const auto* glyph = getGlyph(cp);
                if (glyph) curLineWidth += glyph->xAdvance * state.currentScale(scale);
                continue;
            }
        }

        uint32_t cp = nextUtf8Char(ptr, end);
        if (cp == '\n') {
            if (curLineWidth > maxLineWidth) maxLineWidth = curLineWidth;
            curLineWidth = 0.0f;
            lineCount++;
            continue;
        }

        const auto* glyph = getGlyph(cp);
        if (glyph) {
            float effScale = state.currentScale(scale);
            curLineWidth += glyph->xAdvance * effScale;
        }
    }

    if (curLineWidth > maxLineWidth) maxLineWidth = curLineWidth;
    float totalHeight = static_cast<float>(lineCount) * (m_lineHeight * scale);

    return {maxLineWidth, totalHeight};
}

std::string TextRenderer::wrapText(std::string_view text, float maxWidth, float scale) const {
    if (text.empty() || maxWidth <= 0.0f) return std::string(text);

    std::string result;
    result.reserve(text.size() + 16);

    float spaceWidth = 0.0f;
    const auto* spaceGlyph = getGlyph(' ');
    if (spaceGlyph) spaceWidth = spaceGlyph->xAdvance * scale;
    else spaceWidth = m_fontSize * 0.3f * scale;

    float currentLineWidth = 0.0f;
    std::string currentLine;

    const char* ptr = text.data();
    const char* end = ptr + text.size();

    while (ptr < end) {
        // 1. Явный перенос строки \n
        if (*ptr == '\n') {
            if (!result.empty()) result += '\n';
            result += currentLine;
            currentLine.clear();
            currentLineWidth = 0.0f;
            ptr++;
            continue;
        }

        // 2. Пробелы между словами
        if (*ptr == ' ' || *ptr == '\t') {
            if (currentLine.empty()) {
                ptr++;
                continue;
            }
            while (ptr < end && (*ptr == ' ' || *ptr == '\t')) {
                ptr++;
            }
            continue;
        }

        // 3. Считываем слово (включая любые прикрепленные теги {...})
        const char* wordStart = ptr;
        while (ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') {
            if (*ptr == '{') {
                if (ptr + 1 < end && *(ptr + 1) == '{') {
                    ptr += 2;
                    continue;
                }
                const char* close = ptr + 1;
                while (close < end && *close != '}' && *close != '\n') close++;
                if (close < end && *close == '}') {
                    ptr = close + 1;
                    continue;
                }
            }
            nextUtf8Char(ptr, end);
        }
        std::string_view word(wordStart, ptr - wordStart);
        float wordWidth = measureText(word, scale).x;

        // 4. Проверяем, помещается ли слово в текущую строку
        float spaceNeeded = currentLine.empty() ? 0.0f : spaceWidth;
        if (currentLineWidth + spaceNeeded + wordWidth > maxWidth && !currentLine.empty()) {
            if (!result.empty()) result += '\n';
            result += currentLine;
            currentLine.clear();
            currentLineWidth = 0.0f;
            spaceNeeded = 0.0f;
        }

        // Если само слово длиннее всей строки (аномально длинное без пробелов)
        if (wordWidth > maxWidth) {
            const char* wPtr = word.data();
            const char* wEnd = wPtr + word.size();
            while (wPtr < wEnd) {
                // Если встречаем тег, поглощаем его целиком без разбиения!
                if (*wPtr == '{') {
                    if (wPtr + 1 < wEnd && *(wPtr + 1) == '{') {
                        currentLine.append("{{");
                        wPtr += 2;
                        continue;
                    }
                    const char* close = wPtr + 1;
                    while (close < wEnd && *close != '}' && *close != '\n') close++;
                    if (close < wEnd && *close == '}') {
                        currentLine.append(wPtr, close + 1 - wPtr);
                        wPtr = close + 1;
                        continue;
                    }
                }

                const char* charStart = wPtr;
                nextUtf8Char(wPtr, wEnd);
                std::string_view charView(charStart, wPtr - charStart);
                float charW = measureText(charView, scale).x;

                if (currentLineWidth + charW > maxWidth && !currentLine.empty()) {
                    if (!result.empty()) result += '\n';
                    result += currentLine;
                    currentLine.clear();
                    currentLineWidth = 0.0f;
                }
                currentLine += charView;
                currentLineWidth += charW;
            }
        } else {
            // Обычное слово
            if (!currentLine.empty()) {
                currentLine += ' ';
                currentLineWidth += spaceWidth;
            }
            currentLine += word;
            currentLineWidth += wordWidth;
        }
    }

    if (!currentLine.empty()) {
        if (!result.empty()) result += '\n';
        result += currentLine;
    }

    return result;
}

void TextRenderer::renderWrappedText(std::string_view text, float x, float y, float maxWidth, float scale, glm::vec4 color) {
    std::string wrapped = wrapText(text, maxWidth, scale);
    renderText(wrapped, x, y, scale, color);
}

