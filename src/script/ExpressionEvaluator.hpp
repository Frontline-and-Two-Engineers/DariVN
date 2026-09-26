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

#ifndef DARIVN_EXPRESSIONEVALUATOR_HPP
#define DARIVN_EXPRESSIONEVALUATOR_HPP

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>

enum class ScriptValueType {
    Null,
    Bool,
    Number,
    String
};

class ScriptValue {
public:
    ScriptValue() : m_type(ScriptValueType::Null), m_bool(false), m_number(0.0) {}
    ScriptValue(bool b) : m_type(ScriptValueType::Bool), m_bool(b), m_number(b ? 1.0 : 0.0) {}
    ScriptValue(int i) : m_type(ScriptValueType::Number), m_bool(i != 0), m_number(static_cast<double>(i)) {}
    ScriptValue(long long l) : m_type(ScriptValueType::Number), m_bool(l != 0), m_number(static_cast<double>(l)) {}
    ScriptValue(float f) : m_type(ScriptValueType::Number), m_bool(f != 0.0f), m_number(static_cast<double>(f)) {}
    ScriptValue(double d) : m_type(ScriptValueType::Number), m_bool(d != 0.0), m_number(d) {}
    ScriptValue(std::string s) : m_type(ScriptValueType::String), m_bool(!s.empty() && s != "false" && s != "0"), m_number(0.0), m_string(std::move(s)) {}
    ScriptValue(std::string_view sv) : m_type(ScriptValueType::String), m_bool(!sv.empty() && sv != "false" && sv != "0"), m_number(0.0), m_string(sv) {}
    ScriptValue(const char* s) : m_type(ScriptValueType::String), m_bool(s && s[0] != '\0' && std::string_view(s) != "false" && std::string_view(s) != "0"), m_number(0.0), m_string(s ? s : "") {}

    [[nodiscard]] ScriptValueType getType() const { return m_type; }
    [[nodiscard]] bool isNull() const { return m_type == ScriptValueType::Null; }
    [[nodiscard]] bool isBool() const { return m_type == ScriptValueType::Bool; }
    [[nodiscard]] bool isNumber() const { return m_type == ScriptValueType::Number; }
    [[nodiscard]] bool isString() const { return m_type == ScriptValueType::String; }

    [[nodiscard]] bool asBool() const;
    [[nodiscard]] double asNumber() const;
    [[nodiscard]] std::string asString() const;

    bool operator==(const ScriptValue& other) const;
    bool operator!=(const ScriptValue& other) const { return !(*this == other); }
    bool operator<(const ScriptValue& other) const;
    bool operator<=(const ScriptValue& other) const;
    bool operator>(const ScriptValue& other) const;
    bool operator>=(const ScriptValue& other) const;

    ScriptValue operator+(const ScriptValue& other) const;
    ScriptValue operator-(const ScriptValue& other) const;
    ScriptValue operator*(const ScriptValue& other) const;
    ScriptValue operator/(const ScriptValue& other) const;
    ScriptValue operator!() const;

private:
    ScriptValueType m_type = ScriptValueType::Null;
    bool m_bool = false;
    double m_number = 0.0;
    std::string m_string;
};

class ExpressionEvaluator {
public:
    using VariableLookup = std::function<ScriptValue(const std::string& name)>;

    ExpressionEvaluator() = delete;

    // Вычисление произвольного выражения (арифметического, строкового или логического)
    static ScriptValue evaluate(std::string_view expr, const VariableLookup& lookup = nullptr);

    // Удобный метод для проверки булева условия
    static bool evaluateCondition(std::string_view condition, const VariableLookup& lookup = nullptr);
};

#endif //DARIVN_EXPRESSIONEVALUATOR_HPP
