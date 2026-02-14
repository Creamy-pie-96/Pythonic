#pragma once

#include "scriptit_types.hpp"
#include "scriptit_methods.hpp"
#include "scriptit_builtins.hpp"

// ═══════════════════════════════════════════════════════════
// ──── Tokenizer ────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

class Tokenizer
{
public:
    std::vector<Token> tokenize(const std::string &source)
    {
        std::vector<Token> tokens;
        int line = 1;

        static const std::unordered_map<std::string, TokenType> keywords = {
            {"var", TokenType::KeywordVar}, {"fn", TokenType::KeywordFn}, {"give", TokenType::KeywordGive}, {"if", TokenType::KeywordIf}, {"elif", TokenType::KeywordElif}, {"else", TokenType::KeywordElse}, {"for", TokenType::KeywordFor}, {"in", TokenType::KeywordIn}, {"range", TokenType::KeywordRange}, {"from", TokenType::KeywordFrom}, {"to", TokenType::KeywordTo}, {"step", TokenType::KeywordStep}, {"pass", TokenType::KeywordPass}, {"while", TokenType::KeywordWhile}, {"are", TokenType::KeywordAre}, {"new", TokenType::KeywordNew}, {"let", TokenType::KeywordLet}, {"be", TokenType::KeywordBe}, {"of", TokenType::KeywordOf}, {"is", TokenType::KeywordIs}, {"points", TokenType::KeywordPoints}};

        static const std::unordered_map<char, TokenType> simpleSymbols = {
            {'+', TokenType::Operator}, {'*', TokenType::Operator}, {'/', TokenType::Operator}, {'^', TokenType::Operator}, {'%', TokenType::Operator}, {',', TokenType::Comma}, {'.', TokenType::Dot}, {':', TokenType::Colon}, {';', TokenType::Semicolon}, {'@', TokenType::At}, {'(', TokenType::LeftParen}, {')', TokenType::RightParen}, {'{', TokenType::LeftBrace}, {'}', TokenType::RightBrace}, {'[', TokenType::LeftBracket}, {']', TokenType::RightBracket}};

        for (size_t i = 0; i < source.length(); ++i)
        {
            char c = source[i];
            // Line continuation: ` before newline suppresses the newline
            if (c == '`')
            {
                // Skip whitespace until newline
                size_t j = i + 1;
                while (j < source.length() && source[j] != '\n' && std::isspace(source[j]))
                    j++;
                if (j < source.length() && source[j] == '\n')
                {
                    // Skip the backtick and the newline (continue on next line)
                    i = j; // the for-loop will increment past \n
                    line++;
                    continue;
                }
                // Stray backtick — ignore it
                continue;
            }

            if (c == '\n')
            {
                // Emit newline token (acts as implicit statement terminator)
                tokens.emplace_back(TokenType::Newline, "\\n", i, line);
                line++;
                continue;
            }
            if (std::isspace(c))
                continue;

            // Comments: --> ... <--
            if (c == '-' && i + 2 < source.length() && source[i + 1] == '-' && source[i + 2] == '>')
            {
                i += 2;
                while (i < source.length())
                {
                    if (source[i] == '\n')
                        line++;
                    if (source[i] == '<' && i + 2 < source.length() && source[i + 1] == '-' && source[i + 2] == '-')
                    {
                        i += 2;
                        break;
                    }
                    i++;
                }
                continue;
            }

            // Single-line comments: # ... (skip to end of line)
            if (c == '#')
            {
                while (i < source.length() && source[i] != '\n')
                    i++;
                // Don't consume the \n — let the normal newline handler emit it
                if (i < source.length())
                    i--; // back up so the main loop's i++ lands on \n
                continue;
            }

            // String literals: "..." or '...'
            if (c == '"' || c == '\'')
            {
                char quote = c;
                std::string str;
                int startPos = i;
                i++;
                while (i < source.length() && source[i] != quote)
                {
                    if (source[i] == '\\' && i + 1 < source.length())
                    {
                        i++;
                        if (source[i] == 'n')
                            str += '\n';
                        else if (source[i] == 't')
                            str += '\t';
                        else if (source[i] == '\\')
                            str += '\\';
                        else if (source[i] == quote)
                            str += quote;
                        else
                            str += source[i];
                    }
                    else
                    {
                        if (source[i] == '\n')
                            line++;
                        str += source[i];
                    }
                    i++;
                }
                if (i >= source.length())
                    throw std::runtime_error("Unterminated string at line " + std::to_string(line));
                tokens.emplace_back(TokenType::String, str, startPos, line);
                continue;
            }

            // Numbers
            if (std::isdigit(c) || (c == '.' && i + 1 < source.length() && std::isdigit(source[i + 1])))
            {
                std::string numStr;
                int startPos = i;
                bool hasDecimal = false;
                while (i < source.length() && (std::isdigit(source[i]) || source[i] == '.'))
                {
                    if (source[i] == '.')
                    {
                        if (hasDecimal)
                            break;
                        if (i + 1 >= source.length() || !std::isdigit(source[i + 1]))
                            break;
                        hasDecimal = true;
                    }
                    numStr += source[i++];
                }
                i--;
                tokens.emplace_back(TokenType::Number, numStr, startPos, line);
                continue;
            }

            // Identifiers / Keywords
            if (std::isalpha(c) || c == '_')
            {
                std::string value;
                int startPos = i;
                while (i < source.length() && (std::isalnum(source[i]) || source[i] == '_'))
                    value += source[i++];
                i--;

                // Multi-word type names: "long double", "long long", "unsigned int", "unsigned long", "unsigned long long"
                if (value == "long" || value == "unsigned")
                {
                    // Peek ahead past whitespace for the next word
                    size_t j = i + 1;
                    while (j < source.length() && source[j] == ' ')
                        j++;
                    if (j < source.length() && (std::isalpha(source[j]) || source[j] == '_'))
                    {
                        std::string nextWord;
                        size_t k = j;
                        while (k < source.length() && (std::isalnum(source[k]) || source[k] == '_'))
                            nextWord += source[k++];

                        if (value == "long" && nextWord == "double")
                        {
                            i = k - 1;
                            tokens.emplace_back(TokenType::Identifier, "long_double", startPos, line);
                            continue;
                        }
                        else if (value == "long" && nextWord == "long")
                        {
                            i = k - 1;
                            tokens.emplace_back(TokenType::Identifier, "long_long", startPos, line);
                            continue;
                        }
                        else if (value == "unsigned" && nextWord == "int")
                        {
                            i = k - 1;
                            tokens.emplace_back(TokenType::Identifier, "uint", startPos, line);
                            continue;
                        }
                        else if (value == "unsigned" && nextWord == "long")
                        {
                            // Check for "unsigned long long"
                            size_t j2 = k;
                            while (j2 < source.length() && source[j2] == ' ')
                                j2++;
                            if (j2 < source.length() && std::isalpha(source[j2]))
                            {
                                std::string thirdWord;
                                size_t k2 = j2;
                                while (k2 < source.length() && (std::isalnum(source[k2]) || source[k2] == '_'))
                                    thirdWord += source[k2++];
                                if (thirdWord == "long")
                                {
                                    i = k2 - 1;
                                    tokens.emplace_back(TokenType::Identifier, "ulong_long", startPos, line);
                                    continue;
                                }
                            }
                            i = k - 1;
                            tokens.emplace_back(TokenType::Identifier, "ulong", startPos, line);
                            continue;
                        }
                    }
                }

                if (keywords.count(value))
                    tokens.emplace_back(keywords.at(value), value, startPos, line);
                else if (value == "and")
                    tokens.emplace_back(TokenType::Operator, "&&", startPos, line);
                else if (value == "or")
                    tokens.emplace_back(TokenType::Operator, "||", startPos, line);
                else if (value == "not")
                    tokens.emplace_back(TokenType::Operator, "!", startPos, line);
                else
                    tokens.emplace_back(TokenType::Identifier, value, startPos, line);
                continue;
            }

            // Negative number hint (handled by parser unary)
            if (c == '-' && i + 1 < source.length() && std::isdigit(source[i + 1]))
            { /* fall through to operator */
            }

            // Multi-char operators (check BEFORE single-char to catch +=, ++, etc.)
            if (c == '+' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::PlusEquals, "+=", i, line);
                i++;
                continue;
            }
            if (c == '+' && i + 1 < source.length() && source[i + 1] == '+')
            {
                tokens.emplace_back(TokenType::PlusPlus, "++", i, line);
                i++;
                continue;
            }
            if (c == '-' && i + 1 < source.length() && source[i + 1] == '-')
            {
                // Check for --- (dash operator for undirected edges) first (3 chars)
                if (i + 2 < source.length() && source[i + 2] == '-')
                {
                    // But NOT if it's ---> (4 chars — not valid, treat first 3 as ---)
                    tokens.emplace_back(TokenType::Dash, "---", i, line);
                    i += 2;
                    continue;
                }
                // But NOT if it's --> (comment start)
                if (!(i + 2 < source.length() && source[i + 2] == '>'))
                {
                    tokens.emplace_back(TokenType::MinusMinus, "--", i, line);
                    i++;
                    continue;
                }
            }
            if (c == '-' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::MinusEquals, "-=", i, line);
                i++;
                continue;
            }
            // -> arrow (directed edge, dict key-value)
            // Must be checked AFTER --> (comment) and -- (decrement) and -=
            if (c == '-' && i + 1 < source.length() && source[i + 1] == '>')
            {
                tokens.emplace_back(TokenType::Arrow, "->", i, line);
                i++;
                continue;
            }
            if (c == '*' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::StarEquals, "*=", i, line);
                i++;
                continue;
            }
            if (c == '/' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::SlashEquals, "/=", i, line);
                i++;
                continue;
            }
            if (c == '%' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::PercentEquals, "%=", i, line);
                i++;
                continue;
            }
            if (c == '=' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::Operator, "==", i, line);
                i++;
                continue;
            }
            if (c == '!' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::Operator, "!=", i, line);
                i++;
                continue;
            }
            // <-> bidirectional arrow (must check BEFORE <= and <)
            if (c == '<' && i + 2 < source.length() && source[i + 1] == '-' && source[i + 2] == '>')
            {
                tokens.emplace_back(TokenType::BiArrow, "<->", i, line);
                i += 2;
                continue;
            }
            if (c == '<' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::Operator, "<=", i, line);
                i++;
                continue;
            }
            if (c == '>' && i + 1 < source.length() && source[i + 1] == '=')
            {
                tokens.emplace_back(TokenType::Operator, ">=", i, line);
                i++;
                continue;
            }
            if (c == '&' && i + 1 < source.length() && source[i + 1] == '&')
            {
                tokens.emplace_back(TokenType::Operator, "&&", i, line);
                i++;
                continue;
            }
            if (c == '|' && i + 1 < source.length() && source[i + 1] == '|')
            {
                tokens.emplace_back(TokenType::Operator, "||", i, line);
                i++;
                continue;
            }

            // Single char symbols
            if (simpleSymbols.count(c))
            {
                tokens.emplace_back(simpleSymbols.at(c), std::string(1, c), i, line);
                continue;
            }

            // Remaining operators
            if (c == '-')
            {
                tokens.emplace_back(TokenType::Operator, "-", i, line);
                continue;
            }
            if (c == '=')
            {
                tokens.emplace_back(TokenType::Equals, "=", i, line);
                continue;
            }
            if (c == '!')
            {
                tokens.emplace_back(TokenType::Operator, "!", i, line);
                continue;
            }
            if (c == '<')
            {
                tokens.emplace_back(TokenType::Operator, "<", i, line);
                continue;
            }
            if (c == '>')
            {
                tokens.emplace_back(TokenType::Operator, ">", i, line);
                continue;
            }

            throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at line " + std::to_string(line));
        }
        tokens.emplace_back(TokenType::Eof, "", -1, line);
        return tokens;
    }
};

// ═══════════════════════════════════════════════════════════
// ──── Evaluator (Expression::evaluate) ─────────────────────
// ═══════════════════════════════════════════════════════════

var Expression::evaluate(Scope &scope)
{
    // Short-circuit evaluation for logical operators
    if (!logicalOp.empty() && lhs && rhs)
    {
        var leftVal = lhs->evaluate(scope);
        if (logicalOp == "&&")
        {
            if (!static_cast<bool>(leftVal))
                return var(false);
            return var(static_cast<bool>(rhs->evaluate(scope)));
        }
        else
        {
            if (static_cast<bool>(leftVal))
                return var(true);
            return var(static_cast<bool>(rhs->evaluate(scope)));
        }
    }

    std::stack<var> stk;
    std::stack<std::string> nameStk; // tracks source variable name for pass-by-ref

    auto pushVal = [&](const var &v, const std::string &name = "")
    {
        stk.push(v);
        nameStk.push(name);
    };
    auto popVal = [&]() -> var
    {
        var v = stk.top();
        stk.pop();
        if (!nameStk.empty())
            nameStk.pop();
        return v;
    };
    auto topName = [&]() -> std::string
    {
        return nameStk.empty() ? "" : nameStk.top();
    };

    for (const auto &token : rpn)
    {
        if (token.type == TokenType::Number)
        {
            if (token.value.find('.') != std::string::npos)
                pushVal(var(std::stod(token.value)));
            else
            {
                try
                {
                    pushVal(var(std::stoi(token.value)));
                }
                catch (...)
                {
                    pushVal(var((long long)std::stoll(token.value)));
                }
            }
        }
        else if (token.type == TokenType::String)
        {
            pushVal(var(token.value));
        }
        else if (token.type == TokenType::Identifier)
        {
            if (token.value == "True")
                pushVal(var(true));
            else if (token.value == "False")
                pushVal(var(false));
            else if (token.value == "None")
                pushVal(var(NoneType{}));
            else
                pushVal(scope.get(token.value), token.value);
        }
        else if (token.type == TokenType::Operator)
        {
            if (token.value == "~")
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for unary '~' at line " + std::to_string(token.line));
                var a = popVal();
                if (a.is_int())
                    pushVal(var(-a.as_int_unchecked()));
                else
                    pushVal(var(-var_to_double(a)));
            }
            else if (token.value == "!")
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for unary '!' at line " + std::to_string(token.line));
                var a = popVal();
                pushVal(var(!static_cast<bool>(a)));
            }
            else
            {
                if (stk.size() < 2)
                    throw std::runtime_error("Stack underflow for binary operator '" + token.value + "' at line " + std::to_string(token.line));
                var b = popVal();
                var a = popVal();

                using BinOp = std::function<var(const var &, const var &)>;
                static const std::unordered_map<std::string, BinOp> binaryOps = {
                    {"+", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() || b.is_string())
                         {
                             std::string sa = a.is_string() ? a.as_string_unchecked() : a.str();
                             std::string sb = b.is_string() ? b.as_string_unchecked() : b.str();
                             return var(sa + sb);
                         }
                         if (a.is_list() && b.is_list())
                             return a + b;
                         return pythonic::math::add(a, b, Overflow::Promote);
                     }},
                    {"-", [](const var &a, const var &b) -> var
                     { return pythonic::math::sub(a, b, Overflow::Promote); }},
                    {"*", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && (b.is_int() || b.is_long() || b.is_long_long()))
                             return a * b;
                         if (b.is_string() && (a.is_int() || a.is_long() || a.is_long_long()))
                             return b * a;
                         if (a.is_list() && (b.is_int() || b.is_long() || b.is_long_long()))
                             return a * b;
                         return pythonic::math::mul(a, b, Overflow::Promote);
                     }},
                    {"/", [](const var &a, const var &b) -> var
                     {
                         double bd = var_to_double(b);
                         if (std::abs(bd) < 1e-15)
                             throw std::runtime_error("Division by zero");
                         return pythonic::math::div(a, b, Overflow::Promote);
                     }},
                    {"%", [](const var &a, const var &b) -> var
                     {
                         double bd = var_to_double(b);
                         if (std::abs(bd) < 1e-15)
                             throw std::runtime_error("Modulo by zero");
                         return pythonic::math::mod(a, b, Overflow::Promote);
                     }},
                    {"^", [](const var &a, const var &b) -> var
                     { return pythonic::math::pow(a, b, Overflow::Promote); }},
                    {"==", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() == b.as_string_unchecked());
                         if (a.is_none() || b.is_none())
                             return var(a.is_none() && b.is_none());
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var(a == b);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) < 1e-9);
                     }},
                    {"!=", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() != b.as_string_unchecked());
                         if (a.is_none() || b.is_none())
                             return var(!(a.is_none() && b.is_none()));
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var(a != b);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) > 1e-9);
                     }},
                    {"<", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) < var_to_double(b)); }},
                    {">", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) > var_to_double(b)); }},
                    {"<=", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) <= var_to_double(b)); }},
                    {">=", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) >= var_to_double(b)); }},
                    {"is", [](const var &a, const var &b) -> var
                     {
                         // Value-based equality (like ==)
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() == b.as_string_unchecked());
                         if (a.is_none() || b.is_none())
                             return var(a.is_none() && b.is_none());
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var(a == b);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) < 1e-9);
                     }},
                    {"is not", [](const var &a, const var &b) -> var
                     {
                         // Value-based inequality (like !=)
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() != b.as_string_unchecked());
                         if (a.is_none() || b.is_none())
                             return var(!(a.is_none() && b.is_none()));
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var(a != b);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) > 1e-9);
                     }},
                    {"points", [](const var &a, const var &b) -> var
                     {
                         // Identity/reference check — in a value-semantics language,
                         // containers are always copies, so this checks if two vars
                         // hold the exact same type + value (stricter than ==, no tolerance).
                         if (a.type() != b.type())
                             return var(false);
                         if (a.is_none() && b.is_none())
                             return var(true);
                         if (a.is_bool() && b.is_bool())
                             return var(a.as_bool_unchecked() == b.as_bool_unchecked());
                         if (a.is_int() && b.is_int())
                             return var(a.as_int_unchecked() == b.as_int_unchecked());
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() == b.as_string_unchecked());
                         // For containers: value compare (since all vars are value types)
                         return var(a == b);
                     }},
                    {"not points", [](const var &a, const var &b) -> var
                     {
                         if (a.type() != b.type())
                             return var(true);
                         if (a.is_none() && b.is_none())
                             return var(false);
                         if (a.is_bool() && b.is_bool())
                             return var(a.as_bool_unchecked() != b.as_bool_unchecked());
                         if (a.is_int() && b.is_int())
                             return var(a.as_int_unchecked() != b.as_int_unchecked());
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() != b.as_string_unchecked());
                         return var(a != b);
                     }},
                    {"&&", [](const var &a, const var &b) -> var
                     { return var(static_cast<bool>(a) && static_cast<bool>(b)); }},
                    {"||", [](const var &a, const var &b) -> var
                     { return var(static_cast<bool>(a) || static_cast<bool>(b)); }},
                    {"->", [](const var &a, const var &b) -> var
                     {
                         // Edge spec: directed edge from a to b
                         Dict d;
                         d["__from__"] = a;
                         d["__to__"] = b;
                         d["__dir__"] = var("directed");
                         return var(std::move(d));
                     }},
                    {"<->", [](const var &a, const var &b) -> var
                     {
                         // Edge spec: bidirectional edge between a and b
                         Dict d;
                         d["__from__"] = a;
                         d["__to__"] = b;
                         d["__dir__"] = var("bidirectional");
                         return var(std::move(d));
                     }},
                    {"---", [](const var &a, const var &b) -> var
                     {
                         // Edge spec: undirected edge between a and b
                         Dict d;
                         d["__from__"] = a;
                         d["__to__"] = b;
                         d["__dir__"] = var("undirected");
                         return var(std::move(d));
                     }},
                };

                auto opIt = binaryOps.find(token.value);
                if (opIt != binaryOps.end())
                {
                    try
                    {
                        pushVal(opIt->second(a, b));
                    }
                    catch (const std::runtime_error &e)
                    {
                        throw std::runtime_error(std::string(e.what()) + " at line " + std::to_string(token.line));
                    }
                }
                else
                    throw std::runtime_error("Unknown binary operator: " + token.value + " at line " + std::to_string(token.line));
            }
        }
        // List literal
        else if (token.type == TokenType::LeftBracket && token.value == "LIST")
        {
            int count = token.position;
            std::vector<var> temp;
            for (int i = 0; i < count; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for list literal at line " + std::to_string(token.line));
                temp.push_back(stk.top());
                nameStk.pop();
                stk.pop();
            }
            std::reverse(temp.begin(), temp.end());
            List items(temp.begin(), temp.end());
            pushVal(var(std::move(items)));
        }
        // Set literal
        else if (token.type == TokenType::LeftBrace && token.value == "SET")
        {
            int count = token.position;
            Set items;
            for (int i = 0; i < count; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for set literal at line " + std::to_string(token.line));
                items.insert(stk.top());
                nameStk.pop();
                stk.pop();
            }
            pushVal(var(std::move(items)));
        }
        // Dict literal {key -> value, ...}
        else if (token.type == TokenType::LeftBrace && token.value == "DICT")
        {
            int count = token.position; // number of key-value pairs
            std::vector<std::pair<var, var>> pairs;
            for (int i = 0; i < count; ++i)
            {
                if (stk.size() < 2)
                    throw std::runtime_error("Stack underflow for dict literal at line " + std::to_string(token.line));
                var val = popVal();
                var key = popVal();
                pairs.push_back({key, val});
            }
            std::reverse(pairs.begin(), pairs.end());
            Dict d;
            for (auto &p : pairs)
            {
                std::string key = p.first.is_string() ? p.first.as_string_unchecked() : p.first.str();
                d[key] = p.second;
            }
            pushVal(var(std::move(d)));
        }
        // ── Method call via dtype dispatch ──
        else if (token.type == TokenType::At)
        {
            std::string method = token.value;
            int argc = token.position;

            std::vector<var> args;
            for (int i = 0; i < argc; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for method args at line " + std::to_string(token.line));
                args.push_back(popVal());
            }
            std::reverse(args.begin(), args.end());

            if (stk.empty())
                throw std::runtime_error("Stack underflow for method call (no object) at line " + std::to_string(token.line));
            std::string selfName = topName();
            var self = popVal();

            var result = dispatch_method(self, method, args);

            // Write back mutations to the original scope variable
            // (methods like clear(), append(), etc. mutate `self` in-place)
            if (!selfName.empty())
            {
                try
                {
                    scope.set(selfName, self);
                }
                catch (...)
                {
                }
            }

            pushVal(result);
        }
        // ── Function calls ──
        else if (token.type == TokenType::KeywordFn)
        {
            std::string fname = token.value;
            int argc = token.position;
            int callLine = token.line;

            if (is_math_function(fname))
            {
                try
                {
                    stk.push(dispatch_math(fname, stk));
                    // Sync nameStk: math functions may pop args; push empty name for result
                    while (nameStk.size() > stk.size())
                        nameStk.pop();
                    while (nameStk.size() < stk.size())
                        nameStk.push("");
                }
                catch (const std::runtime_error &e)
                {
                    std::string msg = e.what();
                    if (msg.find("at line") == std::string::npos)
                        throw std::runtime_error(msg + " at line " + std::to_string(callLine));
                    throw;
                }
                continue;
            }

            const auto &builtins = get_builtins();
            auto builtinIt = builtins.find(fname);
            if (builtinIt != builtins.end())
            {
                try
                {
                    builtinIt->second(stk, argc);
                    // Sync nameStk
                    while (nameStk.size() > stk.size())
                        nameStk.pop();
                    while (nameStk.size() < stk.size())
                        nameStk.push("");
                }
                catch (const std::runtime_error &e)
                {
                    std::string msg = e.what();
                    if (msg.find("at line") == std::string::npos)
                        throw std::runtime_error(msg + " at line " + std::to_string(callLine));
                    throw;
                }
                continue;
            }

            // User-defined function call
            try
            {
                FunctionDef def = scope.getFunction(fname, argc);
                if (!def.body)
                    throw std::runtime_error("Function '" + fname + "' was forward-declared but never defined at line " + std::to_string(token.line));
                if ((int)stk.size() < argc)
                    throw std::runtime_error("Stack underflow for function args at line " + std::to_string(token.line));
                Scope funcScope(&scope, true);
                std::vector<var> args;
                std::vector<std::string> argNames; // caller variable names for ref params
                for (int i = 0; i < argc; ++i)
                {
                    argNames.push_back(topName());
                    args.push_back(popVal());
                }
                std::reverse(args.begin(), args.end());
                std::reverse(argNames.begin(), argNames.end());
                for (size_t i = 0; i < def.params.size(); ++i)
                    funcScope.define(def.params[i], args[i]);
                try
                {
                    def.body->execute(funcScope);
                }
                catch (ReturnException &ret)
                {
                    // Write back ref params to caller's scope
                    for (size_t i = 0; i < def.params.size(); ++i)
                    {
                        if (i < def.isRefParam.size() && def.isRefParam[i] && !argNames[i].empty())
                            scope.set(argNames[i], funcScope.get(def.params[i]));
                    }
                    pushVal(ret.value);
                    continue;
                }
                // Write back ref params to caller's scope (even if no give)
                for (size_t i = 0; i < def.params.size(); ++i)
                {
                    if (i < def.isRefParam.size() && def.isRefParam[i] && !argNames[i].empty())
                        scope.set(argNames[i], funcScope.get(def.params[i]));
                }
                // Function returned without give() → return None
                pushVal(var(NoneType{}));
            }
            catch (const std::runtime_error &e)
            {
                if (std::string(e.what()).find("Unknown function") != std::string::npos)
                    throw std::runtime_error("Unknown function call: " + fname + " at line " + std::to_string(token.line));
                throw;
            }
        }
    }

    if (stk.empty())
        return var(0);
    return stk.top();
}

// ═══════════════════════════════════════════════════════════
// ──── Statement Implementations ────────────────────────────
// ═══════════════════════════════════════════════════════════

void BlockStmt::execute(Scope &scope)
{
    Scope blockScope(&scope, false);
    // Two-pass execution: first register all function definitions,
    // then execute everything (enables forward references)
    for (auto &stmt : statements)
    {
        auto funcDef = std::dynamic_pointer_cast<FunctionDefStmt>(stmt);
        if (funcDef && funcDef->body)
        {
            FunctionDef def;
            def.name = funcDef->name;
            def.params = funcDef->params;
            def.body = funcDef->body;
            blockScope.defineFunction(funcDef->name, def);
        }
    }
    for (auto &stmt : statements)
        stmt->execute(blockScope);
}

void IfStmt::execute(Scope &scope)
{
    for (auto &branch : branches)
    {
        if (static_cast<bool>(branch.condition->evaluate(scope)))
        {
            branch.block->execute(scope);
            return;
        }
    }
    if (elseBlock)
        elseBlock->execute(scope);
}

void ForStmt::execute(Scope &scope)
{
    double start = var_to_double(startExpr->evaluate(scope));
    double end = var_to_double(endExpr->evaluate(scope));
    double step;
    if (stepExpr)
    {
        step = var_to_double(stepExpr->evaluate(scope));
        if (std::abs(step) < 1e-15)
            throw std::runtime_error("Step cannot be zero in range");
    }
    else
    {
        step = (end >= start) ? 1.0 : -1.0;
    }
    Scope loopScope(&scope);
    loopScope.define(iteratorName, var(start));
    double current = start;
    if (step > 0)
    {
        while (current <= end + 1e-9)
        {
            loopScope.set(iteratorName, var(current));
            body->execute(loopScope);
            current += step;
        }
    }
    else
    {
        while (current >= end - 1e-9)
        {
            loopScope.set(iteratorName, var(current));
            body->execute(loopScope);
            current += step;
        }
    }
}

void ForInStmt::execute(Scope &scope)
{
    var iterable = iterableExpr->evaluate(scope);
    if (!iterable.is_list() && !iterable.is_string() && !iterable.is_set())
        throw std::runtime_error("for-in requires a list, string, or set; got " + iterable.type());
    Scope loopScope(&scope);
    loopScope.define(iteratorName, var(0));
    for (const auto &item : iterable)
    {
        loopScope.set(iteratorName, item);
        body->execute(loopScope);
    }
}

void WhileStmt::execute(Scope &scope)
{
    while (static_cast<bool>(condition->evaluate(scope)))
    {
        Scope loopScope(&scope, false);
        body->execute(loopScope);
    }
}

void FunctionDefStmt::execute(Scope &scope)
{
    if (!body)
    {
        // Forward declaration only — record that this function exists
        // Don't error if already defined; just update the declaration
        std::string key = Scope::funcKey(name, (int)params.size());
        if (!scope.functions.count(key))
        {
            scope.declareFunction(name, params);
        }
        return;
    }
    // Full definition — always allowed (supports redefinition, like Python)
    FunctionDef def;
    def.name = name;
    def.params = params;
    def.isRefParam = isRefParam;
    def.body = body;
    scope.defineFunction(name, def);
}

void ReturnStmt::execute(Scope &scope)
{
    throw ReturnException(expr->evaluate(scope));
}

void AssignStmt::execute(Scope &scope)
{
    var val = expr->evaluate(scope);
    if (isDeclaration)
        scope.define(name, val);
    else
        scope.set(name, val);
}

void ExprStmt::execute(Scope &scope)
{
    var val = expr->evaluate(scope);
    if (!val.is_none())
        std::cout << format_output(val) << std::endl;
}

void LetContextStmt::execute(Scope &scope)
{
    // Evaluate the expression (e.g. open("file.txt", "w"))
    var resource = expr->evaluate(scope);

    // Create a child scope and bind the resource
    Scope childScope(&scope);
    childScope.define(name, resource);

    try
    {
        body->execute(childScope);
    }
    catch (...)
    {
        // Auto-close file if it's a file handle
        int fid;
        if (is_file_dict(resource, fid))
        {
            try
            {
                FileRegistry::instance().close(fid);
            }
            catch (...)
            {
            }
        }
        throw; // re-throw
    }

    // Auto-close the resource after the block
    int fid;
    if (is_file_dict(resource, fid))
    {
        FileRegistry::instance().close(fid);
    }
}

// ═══════════════════════════════════════════════════════════
// ──── Parser ───────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

class Parser
{
    const std::vector<Token> &tokens;
    size_t pos = 0;
    int braceDepth = 0; // Track brace nesting for context-sensitive -> parsing

public:
    Parser(const std::vector<Token> &t) : tokens(t) {}

    std::shared_ptr<BlockStmt> parseProgram()
    {
        auto block = std::make_shared<BlockStmt>();
        while (!isAtEnd())
        {
            // Skip newline tokens between statements
            while (check(TokenType::Newline))
                advance();
            if (isAtEnd())
                break;
            block->statements.push_back(parseStatement());
        }
        return block;
    }

    std::shared_ptr<Statement> parseStatement()
    {
        if (match(TokenType::KeywordIf))
            return parseIf();
        if (match(TokenType::KeywordFor))
            return parseFor();
        if (match(TokenType::KeywordWhile))
            return parseWhile();
        if (match(TokenType::KeywordFn))
            return parseFunction();
        if (match(TokenType::KeywordGive))
            return parseReturn();
        if (match(TokenType::KeywordPass))
            return parsePass();

        // let x be expr.  OR  let x be expr : block ;
        if (match(TokenType::KeywordLet))
        {
            Token name = consume(TokenType::Identifier, "Expected identifier after let");
            consume(TokenType::KeywordBe, "Expected 'be' after let <name>");
            auto expr = parseExpression();

            // Check for context-manager form: let f be open(...) : ... ;
            if (match(TokenType::Colon))
            {
                auto ctx = std::make_shared<LetContextStmt>();
                ctx->name = name.value;
                ctx->expr = expr;
                ctx->body = parseBlock({TokenType::Semicolon});
                if (!isAtEnd())
                    match(TokenType::Semicolon); // consume the terminator
                return ctx;
            }

            consumeDotOrForgive();
            auto assign = std::make_shared<AssignStmt>();
            assign->name = name.value;
            assign->expr = expr;
            assign->isDeclaration = true;
            return assign;
        }

        // var declarations — unified handling
        if (match(TokenType::KeywordVar))
        {
            auto multi = std::make_shared<MultiVarStmt>();

            auto parseOneVar = [&]() -> std::shared_ptr<AssignStmt>
            {
                Token varName = consume(TokenType::Identifier, "Expected identifier after var");
                auto assign = std::make_shared<AssignStmt>();
                assign->name = varName.value;
                assign->isDeclaration = true;
                if (match(TokenType::Equals))
                    assign->expr = parseExpression();
                else
                {
                    auto noneExpr = std::make_shared<Expression>();
                    noneExpr->rpn.push_back(Token(TokenType::Identifier, "None", -1, varName.line));
                    assign->expr = noneExpr;
                }
                return assign;
            };

            multi->assignments.push_back(parseOneVar());

            while (true)
            {
                if (match(TokenType::Comma))
                    multi->assignments.push_back(parseOneVar());
                else if (check(TokenType::Identifier) && !is_builtin_function(peek().value) &&
                         peek().value != "True" && peek().value != "False" && peek().value != "None")
                {
                    TokenType nextType = peekNext().type;
                    // Check if next is Dot — only treat as multi-var if it's a terminator dot (not method call)
                    bool isDotTerminator = false;
                    if (nextType == TokenType::Dot)
                    {
                        // If the dot is adjacent to the identifier, it's a method call (x.upper())
                        // If there's a gap, it's a terminator (x .)
                        int identEnd = peek().position + (int)peek().value.size();
                        size_t nextIdx = pos + 1;
                        // peekNext might have skipped newlines — find the actual dot position
                        while (nextIdx < tokens.size() && tokens[nextIdx].type == TokenType::Newline)
                            nextIdx++;
                        if (nextIdx < tokens.size() && tokens[nextIdx].type == TokenType::Dot)
                        {
                            int dotPos = tokens[nextIdx].position;
                            isDotTerminator = (dotPos != identEnd); // gap = terminator, no gap = method call
                        }
                    }
                    if (nextType == TokenType::Equals ||
                        isDotTerminator ||
                        nextType == TokenType::Comma ||
                        nextType == TokenType::Identifier ||
                        nextType == TokenType::Eof)
                        multi->assignments.push_back(parseOneVar());
                    else
                        break;
                }
                else
                    break;
            }
            consumeDotOrForgive();

            if (multi->assignments.size() == 1)
                return multi->assignments[0];
            return multi;
        }

        // Compound assignment:  x += expr.  x -= expr.  x *= expr.  x /= expr.  x %= expr.
        if (check(TokenType::Identifier))
        {
            TokenType nextT = peekNext().type;
            if (nextT == TokenType::PlusEquals || nextT == TokenType::MinusEquals ||
                nextT == TokenType::StarEquals || nextT == TokenType::SlashEquals ||
                nextT == TokenType::PercentEquals)
            {
                Token name = advance();
                Token op = advance(); // consume the compound op
                auto rhs = parseExpression();

                // Determine the arithmetic operator string
                std::string arithOp;
                if (op.type == TokenType::PlusEquals)
                    arithOp = "+";
                else if (op.type == TokenType::MinusEquals)
                    arithOp = "-";
                else if (op.type == TokenType::StarEquals)
                    arithOp = "*";
                else if (op.type == TokenType::SlashEquals)
                    arithOp = "/";
                else
                    arithOp = "%";

                // Build expression: name op rhs  →  [name] [rhs_rpn...] [op]
                auto combined = std::make_shared<Expression>();
                combined->rpn.push_back(Token(TokenType::Identifier, name.value, name.position, name.line));
                for (auto &t : rhs->rpn)
                    combined->rpn.push_back(t);
                combined->rpn.push_back(Token(TokenType::Operator, arithOp, op.position, op.line));

                consumeDotOrForgive();
                auto assign = std::make_shared<AssignStmt>();
                assign->name = name.value;
                assign->expr = combined;
                return assign;
            }
        }

        // Pre-increment/decrement:  ++i.  --i.
        if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus))
        {
            Token op = advance(); // ++ or --
            Token name = consume(TokenType::Identifier, "Expected identifier after " + op.value);
            std::string arithOp = (op.type == TokenType::PlusPlus) ? "+" : "-";

            auto combined = std::make_shared<Expression>();
            combined->rpn.push_back(Token(TokenType::Identifier, name.value, name.position, name.line));
            combined->rpn.push_back(Token(TokenType::Number, "1", -1, name.line));
            combined->rpn.push_back(Token(TokenType::Operator, arithOp, -1, name.line));

            consumeDotOrForgive();
            auto assign = std::make_shared<AssignStmt>();
            assign->name = name.value;
            assign->expr = combined;
            return assign;
        }

        // Identifier = expr. (assignment)
        if (check(TokenType::Identifier) && peekNext().type == TokenType::Equals)
        {
            Token name = advance();
            advance(); // =
            auto expr = parseExpression();
            consumeDotOrForgive();
            auto assign = std::make_shared<AssignStmt>();
            assign->name = name.value;
            assign->expr = expr;
            return assign;
        }

        // Post-increment/decrement:  i++.  i--.  (as a statement)
        if (check(TokenType::Identifier) &&
            (peekNext().type == TokenType::PlusPlus || peekNext().type == TokenType::MinusMinus))
        {
            Token name = advance();
            Token op = advance(); // ++ or --
            std::string arithOp = (op.type == TokenType::PlusPlus) ? "+" : "-";

            auto combined = std::make_shared<Expression>();
            combined->rpn.push_back(Token(TokenType::Identifier, name.value, name.position, name.line));
            combined->rpn.push_back(Token(TokenType::Number, "1", -1, name.line));
            combined->rpn.push_back(Token(TokenType::Operator, arithOp, -1, name.line));

            consumeDotOrForgive();
            auto assign = std::make_shared<AssignStmt>();
            assign->name = name.value;
            assign->expr = combined;
            return assign;
        }

        // Expression statement
        auto expr = parseExpression();
        consumeDotOrForgive();
        auto stmt = std::make_shared<ExprStmt>();
        stmt->expr = expr;
        return stmt;
    }

    std::shared_ptr<IfStmt> parseIf()
    {
        auto stmt = std::make_shared<IfStmt>();
        auto cond = parseExpression();
        consume(TokenType::Colon, "Expected : after if condition");
        auto block = parseBlock({TokenType::KeywordElif, TokenType::KeywordElse, TokenType::Semicolon});
        stmt->branches.push_back({cond, block});
        while (match(TokenType::KeywordElif))
        {
            auto elifCond = parseExpression();
            consume(TokenType::Colon, "Expected : after elif");
            auto elifBlock = parseBlock({TokenType::KeywordElif, TokenType::KeywordElse, TokenType::Semicolon});
            stmt->branches.push_back({elifCond, elifBlock});
        }
        if (match(TokenType::KeywordElse))
        {
            consume(TokenType::Colon, "Expected : after else");
            stmt->elseBlock = parseBlock({TokenType::Semicolon});
        }
        consume(TokenType::Semicolon, "Expected ; at end of if-structure");
        return stmt;
    }

    std::shared_ptr<Statement> parseFor()
    {
        Token iter = consume(TokenType::Identifier, "Expected iterator name");
        consume(TokenType::KeywordIn, "Expected in");

        if (check(TokenType::KeywordRange))
        {
            consume(TokenType::KeywordRange, "Expected range");
            consume(TokenType::LeftParen, "Expected (");

            // Three forms:
            //   range(N)               → 0 to N, step 1
            //   range(from A to B)     → A to B, auto step
            //   range(from A to B step S) → A to B, step S
            std::shared_ptr<Expression> start, end;
            std::shared_ptr<Expression> stepExpr = nullptr;

            if (check(TokenType::KeywordFrom))
            {
                consume(TokenType::KeywordFrom, "Expected from");
                start = parseExpression();
                consume(TokenType::KeywordTo, "Expected to");
                end = parseExpression();
                if (match(TokenType::KeywordStep))
                {
                    stepExpr = parseExpression();
                }
            }
            else
            {
                // Simple range(N) — from 0 to N
                end = parseExpression();
                // Create a literal 0 for start
                start = std::make_shared<Expression>();
                start->rpn.push_back(Token(TokenType::Number, "0", -1, peek().line));
            }

            consume(TokenType::RightParen, "Expected )");
            consume(TokenType::Colon, "Expected :");
            auto body = parseBlock({TokenType::Semicolon});
            consume(TokenType::Semicolon, "Expected ; after loop");
            auto stmt = std::make_shared<ForStmt>();
            stmt->iteratorName = iter.value;
            stmt->startExpr = start;
            stmt->endExpr = end;
            stmt->stepExpr = stepExpr;
            stmt->body = body;
            return stmt;
        }

        auto iterableExpr = parseExpression();
        consume(TokenType::Colon, "Expected :");
        auto body = parseBlock({TokenType::Semicolon});
        consume(TokenType::Semicolon, "Expected ; after loop");
        auto stmt = std::make_shared<ForInStmt>();
        stmt->iteratorName = iter.value;
        stmt->iterableExpr = iterableExpr;
        stmt->body = body;
        return stmt;
    }

    std::shared_ptr<PassStmt> parsePass()
    {
        consumeDotOrForgive();
        return std::make_shared<PassStmt>();
    }

    std::shared_ptr<WhileStmt> parseWhile()
    {
        auto stmt = std::make_shared<WhileStmt>();
        stmt->condition = parseExpression();
        consume(TokenType::Colon, "Expected : after while condition");
        stmt->body = parseBlock({TokenType::Semicolon});
        consume(TokenType::Semicolon, "Expected ; after while body");
        return stmt;
    }

    std::shared_ptr<FunctionDefStmt> parseFunction()
    {
        auto stmt = std::make_shared<FunctionDefStmt>();
        stmt->name = consume(TokenType::Identifier, "Expected function name").value;
        consume(TokenType::LeftParen, "Expected ( after function name");
        if (!check(TokenType::RightParen))
        {
            std::unordered_set<std::string> seenParams;
            do
            {
                bool isRef = match(TokenType::At); // @ prefix for pass-by-reference
                std::string pname = consume(TokenType::Identifier, "Expected param name").value;
                if (seenParams.count(pname))
                    throw std::runtime_error("Duplicate parameter name '" + pname + "' in function '" + stmt->name + "'");
                seenParams.insert(pname);
                stmt->params.push_back(pname);
                stmt->isRefParam.push_back(isRef);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "Expected ) after params");

        // Forward declaration: fn name(params).  (no body)
        if (check(TokenType::Dot) || check(TokenType::Newline) || isAtEnd())
        {
            consumeDotOrForgive();
            stmt->body = nullptr; // marker for forward declaration
            return stmt;
        }

        consume(TokenType::Colon, "Expected : start of function body");
        stmt->body = parseBlock({TokenType::Semicolon});
        consume(TokenType::Semicolon, "Expected ; after function body");
        if (stmt->body->statements.empty())
            throw std::runtime_error("Empty function body not allowed, use 'pass'.");
        return stmt;
    }

    std::shared_ptr<ReturnStmt> parseReturn()
    {
        // give expr.       → return expr (no parens)
        // give(expr).      → return expr (optional parens for grouping)
        // give().           → return None
        auto stmt = std::make_shared<ReturnStmt>();
        auto expr = parseExpression();
        // If the expression is empty (nothing parsed), return None
        if (expr->rpn.empty() && expr->logicalOp.empty())
        {
            auto noneExpr = std::make_shared<Expression>();
            noneExpr->rpn.push_back(Token(TokenType::Identifier, "None", -1, -1));
            stmt->expr = noneExpr;
        }
        else
        {
            stmt->expr = expr;
        }
        consumeDotOrForgive();
        return stmt;
    }

    std::shared_ptr<BlockStmt> parseBlock(const std::vector<TokenType> &terminators)
    {
        auto block = std::make_shared<BlockStmt>();
        while (!isAtEnd())
        {
            // Skip newlines between statements in blocks
            while (check(TokenType::Newline))
                advance();
            if (isAtEnd())
                break;
            for (auto t : terminators)
                if (check(t))
                    return block;
            block->statements.push_back(parseStatement());
        }
        return block;
    }

    // ── Expression Parsing ──

    std::shared_ptr<Expression> parseExpression()
    {
        auto expr = parseLogicalOr();

        // Handle "func(args) of target" → target.func(args)
        if (check(TokenType::KeywordOf))
        {
            advance();                      // consume 'of'
            auto target = parseLogicalOr(); // parse the target expression

            // Rewrite RPN: expr's rpn has [args...] [funcname(At/KeywordFn/Identifier)]
            // We need: [target_rpn] [args...] [funcname(At, method call)]
            auto newExpr = std::make_shared<Expression>();

            if (!expr->rpn.empty() && expr->rpn.back().type == TokenType::At)
            {
                // Already a method call — prepend target
                for (auto &tok : target->rpn)
                    newExpr->rpn.push_back(tok);
                for (auto &tok : expr->rpn)
                    newExpr->rpn.push_back(tok);
                return newExpr;
            }
            else if (!expr->rpn.empty())
            {
                // Function call: last token is KeywordFn or Identifier with position=argCount
                // Convert to method call: push target first, then args, then convert to At
                for (auto &tok : target->rpn)
                    newExpr->rpn.push_back(tok);
                for (size_t i = 0; i < expr->rpn.size(); ++i)
                {
                    Token tok = expr->rpn[i];
                    if (i == expr->rpn.size() - 1 &&
                        (tok.type == TokenType::Identifier || tok.type == TokenType::KeywordFn))
                    {
                        // Convert function call to method call
                        tok.type = TokenType::At;
                    }
                    newExpr->rpn.push_back(tok);
                }
                return newExpr;
            }
        }

        return expr;
    }

    std::shared_ptr<Expression> parseLogicalOr()
    {
        auto left = parseLogicalAnd();
        while (!isAtEnd() && peek().type == TokenType::Operator && peek().value == "||")
        {
            advance();
            auto right = parseLogicalAnd();
            auto node = std::make_shared<Expression>();
            node->logicalOp = "||";
            node->lhs = left;
            node->rhs = right;
            left = node;
        }
        return left;
    }

    std::shared_ptr<Expression> parseLogicalAnd()
    {
        auto left = parsePrimaryExpr();
        while (!isAtEnd() && peek().type == TokenType::Operator && peek().value == "&&")
        {
            advance();
            auto right = parsePrimaryExpr();
            auto node = std::make_shared<Expression>();
            node->logicalOp = "&&";
            node->lhs = left;
            node->rhs = right;
            left = node;
        }
        return left;
    }

    // Shunting-Yard
    std::shared_ptr<Expression> parsePrimaryExpr()
    {
        auto expr = std::make_shared<Expression>();
        std::queue<Token> out;
        std::stack<Token> opStack;
        TokenType lastTokenType = TokenType::Eof;

        while (!isAtEnd())
        {
            Token t = peek();

            // ── Dot: method call or expression terminator ──
            if (t.type == TokenType::Dot)
            {
                if (pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::Identifier &&
                    pos + 2 < tokens.size() && tokens[pos + 2].type == TokenType::LeftParen)
                {
                    int dotEndPos = t.position + 1;
                    int identStartPos = tokens[pos + 1].position;
                    if (dotEndPos == identStartPos)
                    {
                        advance(); // consume '.'
                        Token methodName = advance();
                        consume(TokenType::LeftParen, "(");
                        int argCount = 0;
                        if (!check(TokenType::RightParen))
                        {
                            do
                            {
                                auto argExpr = parseExpression();
                                if (argExpr->logicalOp.empty())
                                    for (auto &at : argExpr->rpn)
                                        out.push(at);
                                else
                                    flattenExprToQueue(argExpr, out);
                                argCount++;
                            } while (match(TokenType::Comma));
                        }
                        consume(TokenType::RightParen, ")");
                        out.push(Token(TokenType::At, methodName.value, argCount, methodName.line));
                        lastTokenType = TokenType::Identifier;
                        continue;
                    }
                }
                break;
            }
            if (t.type == TokenType::Colon || t.type == TokenType::Semicolon ||
                t.type == TokenType::KeywordIn || t.type == TokenType::KeywordTo ||
                t.type == TokenType::KeywordStep ||
                t.type == TokenType::KeywordElif || t.type == TokenType::KeywordElse ||
                t.type == TokenType::KeywordBe || t.type == TokenType::Equals ||
                t.type == TokenType::Newline || t.type == TokenType::KeywordOf ||
                t.type == TokenType::PlusEquals || t.type == TokenType::MinusEquals ||
                t.type == TokenType::StarEquals || t.type == TokenType::SlashEquals ||
                t.type == TokenType::PercentEquals || t.type == TokenType::PlusPlus ||
                t.type == TokenType::MinusMinus)
                break;

            // Handle 'is', 'is not', 'points', 'not points' as binary operators
            if (t.type == TokenType::KeywordIs)
            {
                advance(); // consume 'is'
                std::string opStr = "is";
                // Check for compound 'is not'
                if (!isAtEnd() && peek().type == TokenType::Operator && peek().value == "!")
                {
                    advance(); // consume 'not'
                    opStr = "is not";
                }
                Token opToken(TokenType::Operator, opStr, t.position, t.line);
                int currPrec = get_operator_precedence(opStr);
                while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                       get_operator_precedence(opStack.top().value) >= currPrec)
                {
                    out.push(opStack.top());
                    opStack.pop();
                }
                opStack.push(opToken);
                lastTokenType = TokenType::Operator;
                continue;
            }
            if (t.type == TokenType::KeywordPoints)
            {
                advance(); // consume 'points'
                Token opToken(TokenType::Operator, "points", t.position, t.line);
                int currPrec = get_operator_precedence("points");
                while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                       get_operator_precedence(opStack.top().value) >= currPrec)
                {
                    out.push(opStack.top());
                    opStack.pop();
                }
                opStack.push(opToken);
                lastTokenType = TokenType::Operator;
                continue;
            }

            if (t.type == TokenType::Operator && (t.value == "&&" || t.value == "||"))
            {
                bool hasParen = false;
                auto temp = opStack;
                while (!temp.empty())
                {
                    if (temp.top().type == TokenType::LeftParen)
                    {
                        hasParen = true;
                        break;
                    }
                    temp.pop();
                }
                if (!hasParen)
                    break;
            }
            if (t.type == TokenType::Comma || t.type == TokenType::RightParen)
            {
                bool hasParen = false;
                auto temp = opStack;
                while (!temp.empty())
                {
                    if (temp.top().type == TokenType::LeftParen)
                    {
                        hasParen = true;
                        break;
                    }
                    temp.pop();
                }
                if (!hasParen)
                    break;
            }
            if (t.type == TokenType::RightBracket || t.type == TokenType::RightBrace)
                break;

            // Stop at "identifier =" boundary — signals next assignment in multi-var
            // e.g. "var a = 10 y = 3." → expression for a stops before "y ="
            if (t.type == TokenType::Identifier && pos + 1 < tokens.size() &&
                tokens[pos + 1].type == TokenType::Equals)
            {
                bool hasParen = false;
                auto temp = opStack;
                while (!temp.empty())
                {
                    if (temp.top().type == TokenType::LeftParen)
                    {
                        hasParen = true;
                        break;
                    }
                    temp.pop();
                }
                if (!hasParen)
                    break;
            }

            Token token = advance();

            // Implicit multiplication
            // Triggers when two "value" tokens are adjacent without an operator:
            //   3x → 3*x, x y → x*y, (a)(b) → (a)*(b), x(2+3) → x*(2+3)
            if ((token.type == TokenType::Number || token.type == TokenType::Identifier || token.type == TokenType::LeftParen) &&
                (lastTokenType == TokenType::Number || lastTokenType == TokenType::RightParen ||
                 lastTokenType == TokenType::Identifier || lastTokenType == TokenType::RightBracket))
            {
                Token mulOp(TokenType::Operator, "*", token.position, token.line);
                int currPrec = get_operator_precedence("*");
                while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                       get_operator_precedence(opStack.top().value) >= currPrec)
                {
                    out.push(opStack.top());
                    opStack.pop();
                }
                opStack.push(mulOp);
            }

            if (token.type == TokenType::Number)
                out.push(token);
            else if (token.type == TokenType::String)
                out.push(token);
            else if (token.type == TokenType::Identifier)
            {
                if (check(TokenType::LeftParen))
                {
                    consume(TokenType::LeftParen, "(");
                    int argCount = 0;
                    if (!check(TokenType::RightParen))
                    {
                        do
                        {
                            auto argExpr = parseExpression();
                            if (argExpr->logicalOp.empty())
                                for (auto &at : argExpr->rpn)
                                    out.push(at);
                            else
                                flattenExprToQueue(argExpr, out);
                            argCount++;
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RightParen, ")");
                    out.push(Token(TokenType::KeywordFn, token.value, argCount, token.line));
                    token.type = TokenType::Identifier;
                }
                else
                    out.push(token);
            }
            else if (token.type == TokenType::LeftBracket)
            {
                int count = 0;
                if (!check(TokenType::RightBracket))
                {
                    do
                    {
                        auto elemExpr = parseExpression();
                        if (elemExpr->logicalOp.empty())
                            for (auto &at : elemExpr->rpn)
                                out.push(at);
                        else
                            flattenExprToQueue(elemExpr, out);
                        count++;
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RightBracket, "Expected ] to close list");
                out.push(Token(TokenType::LeftBracket, "LIST", count, token.line));
                lastTokenType = TokenType::RightBracket;
                continue;
            }
            else if (token.type == TokenType::LeftBrace)
            {
                braceDepth++;
                int count = 0;
                bool isDict = false;
                if (!check(TokenType::RightBrace))
                {
                    // Parse first element to detect dict vs set
                    auto firstExpr = parseExpression();
                    if (check(TokenType::Arrow))
                    {
                        // Dict literal: {key -> value, ...}
                        isDict = true;
                        advance(); // consume ->
                        // Push key
                        if (firstExpr->logicalOp.empty())
                            for (auto &at : firstExpr->rpn)
                                out.push(at);
                        else
                            flattenExprToQueue(firstExpr, out);
                        // Parse value
                        auto valExpr = parseExpression();
                        if (valExpr->logicalOp.empty())
                            for (auto &at : valExpr->rpn)
                                out.push(at);
                        else
                            flattenExprToQueue(valExpr, out);
                        count++;
                        while (match(TokenType::Comma))
                        {
                            auto keyExpr = parseExpression();
                            consume(TokenType::Arrow, "Expected '->' in dict literal");
                            auto vExpr = parseExpression();
                            if (keyExpr->logicalOp.empty())
                                for (auto &at : keyExpr->rpn)
                                    out.push(at);
                            else
                                flattenExprToQueue(keyExpr, out);
                            if (vExpr->logicalOp.empty())
                                for (auto &at : vExpr->rpn)
                                    out.push(at);
                            else
                                flattenExprToQueue(vExpr, out);
                            count++;
                        }
                    }
                    else
                    {
                        // Set literal
                        if (firstExpr->logicalOp.empty())
                            for (auto &at : firstExpr->rpn)
                                out.push(at);
                        else
                            flattenExprToQueue(firstExpr, out);
                        count++;
                        while (match(TokenType::Comma))
                        {
                            auto elemExpr = parseExpression();
                            if (elemExpr->logicalOp.empty())
                                for (auto &at : elemExpr->rpn)
                                    out.push(at);
                            else
                                flattenExprToQueue(elemExpr, out);
                            count++;
                        }
                    }
                }
                consume(TokenType::RightBrace, "Expected } to close " + std::string(isDict ? "dict" : "set"));
                braceDepth--;
                out.push(Token(TokenType::LeftBrace, isDict ? "DICT" : "SET", count, token.line));
                lastTokenType = TokenType::RightBrace;
                continue;
            }
            else if (token.type == TokenType::Operator)
            {
                // Check for 'not points' compound operator (tokenized as ! followed by KeywordPoints)
                if (token.value == "!" && !isAtEnd() && peek().type == TokenType::KeywordPoints)
                {
                    advance(); // consume 'points'
                    Token opToken(TokenType::Operator, "not points", token.position, token.line);
                    int currPrec = get_operator_precedence("not points");
                    while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                           get_operator_precedence(opStack.top().value) >= currPrec)
                    {
                        out.push(opStack.top());
                        opStack.pop();
                    }
                    opStack.push(opToken);
                    lastTokenType = TokenType::Operator;
                    continue;
                }

                bool isUnary = false;
                if (token.value == "-" || token.value == "!")
                {
                    if (lastTokenType == TokenType::Eof || lastTokenType == TokenType::LeftParen ||
                        lastTokenType == TokenType::Comma || lastTokenType == TokenType::Operator ||
                        lastTokenType == TokenType::Equals || lastTokenType == TokenType::Colon ||
                        lastTokenType == TokenType::KeywordIf || lastTokenType == TokenType::KeywordElif ||
                        lastTokenType == TokenType::KeywordGive)
                        isUnary = true;
                }
                if (isUnary)
                {
                    std::string op = (token.value == "-") ? "~" : token.value;
                    opStack.push(Token(TokenType::Operator, op, token.position, token.line));
                }
                else
                {
                    int currPrec = get_operator_precedence(token.value);
                    while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                           get_operator_precedence(opStack.top().value) >= currPrec)
                    {
                        out.push(opStack.top());
                        opStack.pop();
                    }
                    opStack.push(token);
                }
            }
            // Arrow operators: ->, <->, --- (treated as binary operators in shunting-yard)
            // Inside braces {}, Arrow acts as dict key-value separator, not expression operator
            else if (token.type == TokenType::Arrow || token.type == TokenType::BiArrow || token.type == TokenType::Dash)
            {
                if (braceDepth > 0 && token.type == TokenType::Arrow)
                {
                    // Inside braces: -> is a dict separator, not an operator
                    // Put it back and let the brace handler deal with it
                    pos--;
                    break;
                }
                // Convert to Operator token for uniform handling
                Token opToken(TokenType::Operator, token.value, token.position, token.line);
                int currPrec = get_operator_precedence(token.value);
                while (!opStack.empty() && opStack.top().type == TokenType::Operator &&
                       get_operator_precedence(opStack.top().value) >= currPrec)
                {
                    out.push(opStack.top());
                    opStack.pop();
                }
                opStack.push(opToken);
                lastTokenType = TokenType::Operator;
                continue;
            }
            else if (token.type == TokenType::LeftParen)
                opStack.push(token);
            else if (token.type == TokenType::RightParen)
            {
                while (!opStack.empty() && opStack.top().type != TokenType::LeftParen)
                {
                    out.push(opStack.top());
                    opStack.pop();
                }
                if (!opStack.empty())
                    opStack.pop();
                else
                {
                    pos--;
                    break;
                }
            }
            else
            {
                pos--;
                break;
            }

            lastTokenType = token.type;
        }

        while (!opStack.empty())
        {
            if (opStack.top().type == TokenType::LeftParen)
                throw std::runtime_error("Mismatched parens at end");
            out.push(opStack.top());
            opStack.pop();
        }
        while (!out.empty())
        {
            expr->rpn.push_back(out.front());
            out.pop();
        }
        return expr;
    }

    void flattenExprToQueue(std::shared_ptr<Expression> expr, std::queue<Token> &out)
    {
        if (expr->logicalOp.empty())
            for (auto &t : expr->rpn)
                out.push(t);
        else
        {
            flattenExprToQueue(expr->lhs, out);
            flattenExprToQueue(expr->rhs, out);
            out.push(Token(TokenType::Operator, expr->logicalOp, -1, -1));
        }
    }

private:
    int lastConsumedLine = 1;
    Token peek() { return pos < tokens.size() ? tokens[pos] : tokens.back(); }
    Token peekNext()
    {
        // Skip over Newline tokens to find the next meaningful token
        size_t nextPos = pos + 1;
        while (nextPos < tokens.size() && tokens[nextPos].type == TokenType::Newline)
            nextPos++;
        return nextPos < tokens.size() ? tokens[nextPos] : tokens.back();
    }
    bool isAtEnd() { return peek().type == TokenType::Eof; }
    Token advance()
    {
        if (!isAtEnd())
            pos++;
        lastConsumedLine = tokens[pos - 1].line;
        return tokens[pos - 1];
    }
    bool check(TokenType t) { return peek().type == t; }
    bool match(TokenType t)
    {
        // Skip newlines before matching (except when matching Newline itself)
        if (t != TokenType::Newline)
        {
            while (check(TokenType::Newline))
                advance();
        }
        if (check(t))
        {
            advance();
            return true;
        }
        return false;
    }
    Token consume(TokenType t, std::string err)
    {
        // Skip newlines before consuming expected token
        while (check(TokenType::Newline))
            advance();
        if (check(t))
            return advance();
        throw std::runtime_error(err + " at line " + std::to_string(peek().line));
    }
    Token consumeDotOrForgive()
    {
        // Skip any newline tokens first, consuming the last one as the terminator
        while (check(TokenType::Newline))
            advance();
        if (check(TokenType::Dot))
            return advance();
        if (isAtEnd() || check(TokenType::Semicolon) ||
            check(TokenType::KeywordElif) || check(TokenType::KeywordElse))
            return Token(TokenType::Dot, ".", -1, peek().line);
        if (peek().line > lastConsumedLine)
            return Token(TokenType::Dot, ".", -1, lastConsumedLine);
        throw std::runtime_error("Expected '.' at line " + std::to_string(lastConsumedLine));
    }
};
