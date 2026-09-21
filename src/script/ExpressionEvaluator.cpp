#include "ExpressionEvaluator.hpp"
#include <cctype>
#include <iostream>
#include <iomanip>

// ==============================================================================
// ScriptValue Implementation
// ==============================================================================

bool ScriptValue::asBool() const {
    switch (m_type) {
        case ScriptValueType::Bool: return m_bool;
        case ScriptValueType::Number: return m_number != 0.0;
        case ScriptValueType::String:
            return !m_string.empty() && m_string != "false" && m_string != "0";
        case ScriptValueType::Null: return false;
    }
    return false;
}

double ScriptValue::asNumber() const {
    switch (m_type) {
        case ScriptValueType::Number: return m_number;
        case ScriptValueType::Bool: return m_bool ? 1.0 : 0.0;
        case ScriptValueType::String: {
            try {
                size_t idx = 0;
                double val = std::stod(m_string, &idx);
                return val;
            } catch (...) {
                return 0.0;
            }
        }
        case ScriptValueType::Null: return 0.0;
    }
    return 0.0;
}

std::string ScriptValue::asString() const {
    switch (m_type) {
        case ScriptValueType::String: return m_string;
        case ScriptValueType::Bool: return m_bool ? "true" : "false";
        case ScriptValueType::Number: {
            if (std::floor(m_number) == m_number && !std::isinf(m_number) && std::abs(m_number) < 1e15) {
                return std::to_string(static_cast<long long>(m_number));
            }
            std::ostringstream ss;
            ss << std::setprecision(10) << m_number;
            return ss.str();
        }
        case ScriptValueType::Null: return "";
    }
    return "";
}

static bool isStringNumeric(std::string_view s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start >= s.size()) return false;
    bool hasDot = false;
    for (size_t i = start; i < s.size(); ++i) {
        if (s[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

bool ScriptValue::operator==(const ScriptValue& other) const {
    if (m_type == ScriptValueType::Null && other.m_type == ScriptValueType::Null) return true;
    if (m_type == ScriptValueType::Null || other.m_type == ScriptValueType::Null) return false;

    if (m_type == ScriptValueType::Bool || other.m_type == ScriptValueType::Bool) {
        return asBool() == other.asBool();
    }

    if (m_type == ScriptValueType::Number && other.m_type == ScriptValueType::Number) {
        return std::abs(m_number - other.m_number) < 1e-9;
    }

    if (m_type == ScriptValueType::Number && other.m_type == ScriptValueType::String) {
        if (isStringNumeric(other.m_string)) {
            return std::abs(m_number - other.asNumber()) < 1e-9;
        }
        return asString() == other.m_string;
    }

    if (m_type == ScriptValueType::String && other.m_type == ScriptValueType::Number) {
        if (isStringNumeric(m_string)) {
            return std::abs(asNumber() - other.m_number) < 1e-9;
        }
        return m_string == other.asString();
    }

    return m_string == other.m_string;
}

bool ScriptValue::operator<(const ScriptValue& other) const {
    if ((m_type == ScriptValueType::Number || (m_type == ScriptValueType::String && isStringNumeric(m_string))) &&
        (other.m_type == ScriptValueType::Number || (other.m_type == ScriptValueType::String && isStringNumeric(other.m_string)))) {
        return asNumber() < other.asNumber();
    }
    return asString() < other.asString();
}

bool ScriptValue::operator<=(const ScriptValue& other) const {
    return (*this < other) || (*this == other);
}

bool ScriptValue::operator>(const ScriptValue& other) const {
    return !(*this <= other);
}

bool ScriptValue::operator>=(const ScriptValue& other) const {
    return !(*this < other);
}

ScriptValue ScriptValue::operator+(const ScriptValue& other) const {
    if (m_type == ScriptValueType::String || other.m_type == ScriptValueType::String) {
        return ScriptValue(asString() + other.asString());
    }
    return ScriptValue(asNumber() + other.asNumber());
}

ScriptValue ScriptValue::operator-(const ScriptValue& other) const {
    return ScriptValue(asNumber() - other.asNumber());
}

ScriptValue ScriptValue::operator*(const ScriptValue& other) const {
    return ScriptValue(asNumber() * other.asNumber());
}

ScriptValue ScriptValue::operator/(const ScriptValue& other) const {
    double denom = other.asNumber();
    if (std::abs(denom) < 1e-9) {
        return ScriptValue(0.0);
    }
    return ScriptValue(asNumber() / denom);
}

ScriptValue ScriptValue::operator!() const {
    return ScriptValue(!asBool());
}

// ==============================================================================
// ExpressionEvaluator Parser Implementation
// ==============================================================================

namespace {

enum class TokenType {
    End,
    Number,
    String,
    Variable,
    True,
    False,
    Null,
    And,
    Or,
    Not,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Plus,
    Minus,
    Multiply,
    Divide,
    LeftParen,
    RightParen
};

struct Token {
    TokenType type = TokenType::End;
    std::string strValue;
    double numValue = 0.0;
};

class Lexer {
public:
    explicit Lexer(std::string_view src) : m_src(src) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (m_pos < m_src.size()) {
            char c = m_src[m_pos];

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                m_pos++;
                continue;
            }

            // Числа
            if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && m_pos + 1 < m_src.size() && std::isdigit(static_cast<unsigned char>(m_src[m_pos + 1])))) {
                size_t start = m_pos;
                bool hasDot = false;
                while (m_pos < m_src.size()) {
                    char ch = m_src[m_pos];
                    if (ch == '.') {
                        if (hasDot) break;
                        hasDot = true;
                        m_pos++;
                    } else if (std::isdigit(static_cast<unsigned char>(ch))) {
                        m_pos++;
                    } else {
                        break;
                    }
                }
                std::string numStr(m_src.substr(start, m_pos - start));
                Token tok;
                tok.type = TokenType::Number;
                tok.numValue = std::stod(numStr);
                tok.strValue = std::move(numStr);
                tokens.push_back(tok);
                continue;
            }

            // Строки в кавычках ("..." или '...')
            if (c == '"' || c == '\'') {
                char quote = c;
                m_pos++;
                size_t start = m_pos;
                std::string s;
                while (m_pos < m_src.size()) {
                    char ch = m_src[m_pos];
                    if (ch == '\\' && m_pos + 1 < m_src.size()) {
                        m_pos++;
                        char esc = m_src[m_pos];
                        if (esc == 'n') s += '\n';
                        else if (esc == 't') s += '\t';
                        else s += esc;
                        m_pos++;
                    } else if (ch == quote) {
                        m_pos++;
                        break;
                    } else {
                        s += ch;
                        m_pos++;
                    }
                }
                Token tok;
                tok.type = TokenType::String;
                tok.strValue = std::move(s);
                tokens.push_back(tok);
                continue;
            }

            // Операторы из двух символов
            if (m_pos + 1 < m_src.size()) {
                std::string_view two = m_src.substr(m_pos, 2);
                if (two == "==") { tokens.push_back({TokenType::EqualEqual, "=="}); m_pos += 2; continue; }
                if (two == "!=") { tokens.push_back({TokenType::NotEqual, "!="}); m_pos += 2; continue; }
                if (two == "<=") { tokens.push_back({TokenType::LessEqual, "<="}); m_pos += 2; continue; }
                if (two == ">=") { tokens.push_back({TokenType::GreaterEqual, ">="}); m_pos += 2; continue; }
                if (two == "&&") { tokens.push_back({TokenType::And, "&&"}); m_pos += 2; continue; }
                if (two == "||") { tokens.push_back({TokenType::Or, "||"}); m_pos += 2; continue; }
            }

            // Одиночные операторы
            if (c == '+') { tokens.push_back({TokenType::Plus, "+"}); m_pos++; continue; }
            if (c == '-') { tokens.push_back({TokenType::Minus, "-"}); m_pos++; continue; }
            if (c == '*') { tokens.push_back({TokenType::Multiply, "*"}); m_pos++; continue; }
            if (c == '/') { tokens.push_back({TokenType::Divide, "/"}); m_pos++; continue; }
            if (c == '(') { tokens.push_back({TokenType::LeftParen, "("}); m_pos++; continue; }
            if (c == ')') { tokens.push_back({TokenType::RightParen, ")"}); m_pos++; continue; }
            if (c == '<') { tokens.push_back({TokenType::Less, "<"}); m_pos++; continue; }
            if (c == '>') { tokens.push_back({TokenType::Greater, ">"}); m_pos++; continue; }
            if (c == '!') { tokens.push_back({TokenType::Not, "!"}); m_pos++; continue; }
            if (c == '=') { tokens.push_back({TokenType::EqualEqual, "=="}); m_pos++; continue; } // forgiving in condition: $a = 5

            // Идентификаторы и переменные ($var или identifier)
            if (c == '$' || std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                size_t start = m_pos;
                m_pos++;
                while (m_pos < m_src.size()) {
                    char ch = m_src[m_pos];
                    if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.') {
                        m_pos++;
                    } else {
                        break;
                    }
                }
                std::string ident(m_src.substr(start, m_pos - start));

                if (ident == "true") { tokens.push_back({TokenType::True, "true"}); continue; }
                if (ident == "false") { tokens.push_back({TokenType::False, "false"}); continue; }
                if (ident == "null" || ident == "nil" || ident == "none") { tokens.push_back({TokenType::Null, "null"}); continue; }
                if (ident == "and") { tokens.push_back({TokenType::And, "and"}); continue; }
                if (ident == "or") { tokens.push_back({TokenType::Or, "or"}); continue; }
                if (ident == "not") { tokens.push_back({TokenType::Not, "not"}); continue; }

                Token tok;
                tok.type = TokenType::Variable;
                tok.strValue = std::move(ident);
                tokens.push_back(tok);
                continue;
            }

            // Неизвестный символ - пропускаем
            m_pos++;
        }

        tokens.push_back({TokenType::End, ""});
        return tokens;
    }

private:
    std::string_view m_src;
    size_t m_pos = 0;
};

class Parser {
public:
    Parser(std::vector<Token> tokens, ExpressionEvaluator::VariableLookup lookup)
        : m_tokens(std::move(tokens)), m_lookup(std::move(lookup)) {}

    ScriptValue parse() {
        if (m_tokens.empty() || m_tokens.front().type == TokenType::End) {
            return ScriptValue();
        }
        return parseOr();
    }

private:
    const Token& peek() const {
        return m_tokens[m_current];
    }

    const Token& previous() const {
        return m_tokens[m_current - 1];
    }

    bool isAtEnd() const {
        return peek().type == TokenType::End;
    }

    bool check(TokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    Token advance() {
        if (!isAtEnd()) m_current++;
        return previous();
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    ScriptValue parseOr() {
        ScriptValue expr = parseAnd();
        while (match(TokenType::Or)) {
            ScriptValue right = parseAnd();
            expr = ScriptValue(expr.asBool() || right.asBool());
        }
        return expr;
    }

    ScriptValue parseAnd() {
        ScriptValue expr = parseEquality();
        while (match(TokenType::And)) {
            ScriptValue right = parseEquality();
            expr = ScriptValue(expr.asBool() && right.asBool());
        }
        return expr;
    }

    ScriptValue parseEquality() {
        ScriptValue expr = parseRelational();
        while (true) {
            if (match(TokenType::EqualEqual)) {
                ScriptValue right = parseRelational();
                expr = ScriptValue(expr == right);
            } else if (match(TokenType::NotEqual)) {
                ScriptValue right = parseRelational();
                expr = ScriptValue(expr != right);
            } else {
                break;
            }
        }
        return expr;
    }

    ScriptValue parseRelational() {
        ScriptValue expr = parseAdditive();
        while (true) {
            if (match(TokenType::Less)) {
                ScriptValue right = parseAdditive();
                expr = ScriptValue(expr < right);
            } else if (match(TokenType::LessEqual)) {
                ScriptValue right = parseAdditive();
                expr = ScriptValue(expr <= right);
            } else if (match(TokenType::Greater)) {
                ScriptValue right = parseAdditive();
                expr = ScriptValue(expr > right);
            } else if (match(TokenType::GreaterEqual)) {
                ScriptValue right = parseAdditive();
                expr = ScriptValue(expr >= right);
            } else {
                break;
            }
        }
        return expr;
    }

    ScriptValue parseAdditive() {
        ScriptValue expr = parseMultiplicative();
        while (true) {
            if (match(TokenType::Plus)) {
                ScriptValue right = parseMultiplicative();
                expr = expr + right;
            } else if (match(TokenType::Minus)) {
                ScriptValue right = parseMultiplicative();
                expr = expr - right;
            } else {
                break;
            }
        }
        return expr;
    }

    ScriptValue parseMultiplicative() {
        ScriptValue expr = parseUnary();
        while (true) {
            if (match(TokenType::Multiply)) {
                ScriptValue right = parseUnary();
                expr = expr * right;
            } else if (match(TokenType::Divide)) {
                ScriptValue right = parseUnary();
                expr = expr / right;
            } else {
                break;
            }
        }
        return expr;
    }

    ScriptValue parseUnary() {
        if (match(TokenType::Not)) {
            ScriptValue right = parseUnary();
            return !right;
        }
        if (match(TokenType::Minus)) {
            ScriptValue right = parseUnary();
            return ScriptValue(-right.asNumber());
        }
        if (match(TokenType::Plus)) {
            return parseUnary();
        }
        return parsePrimary();
    }

    ScriptValue parsePrimary() {
        if (match(TokenType::Number)) {
            return ScriptValue(previous().numValue);
        }
        if (match(TokenType::String)) {
            return ScriptValue(previous().strValue);
        }
        if (match(TokenType::True)) {
            return ScriptValue(true);
        }
        if (match(TokenType::False)) {
            return ScriptValue(false);
        }
        if (match(TokenType::Null)) {
            return ScriptValue();
        }
        if (match(TokenType::Variable)) {
            std::string name = previous().strValue;
            if (name.starts_with('$')) {
                name = name.substr(1);
            }
            if (m_lookup) {
                return m_lookup(name);
            }
            return ScriptValue();
        }
        if (match(TokenType::LeftParen)) {
            ScriptValue expr = parseOr();
            if (!match(TokenType::RightParen)) {
                std::cerr << "[ExpressionEvaluator] Missing closing parenthesis ')'" << std::endl;
            }
            return expr;
        }

        // Если неожиданный токен, пропускаем его и возвращаем Null
        if (!isAtEnd()) advance();
        return ScriptValue();
    }

private:
    std::vector<Token> m_tokens;
    ExpressionEvaluator::VariableLookup m_lookup;
    size_t m_current = 0;
};

} // anonymous namespace

ScriptValue ExpressionEvaluator::evaluate(std::string_view expr, const VariableLookup& lookup) {
    Lexer lexer(expr);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens), lookup);
    return parser.parse();
}

bool ExpressionEvaluator::evaluateCondition(std::string_view condition, const VariableLookup& lookup) {
    if (condition.empty()) return true;
    ScriptValue val = evaluate(condition, lookup);
    return val.asBool();
}
