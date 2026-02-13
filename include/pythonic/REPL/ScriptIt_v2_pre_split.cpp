// ScriptIt v2 — A scripting language powered by pythonic::vars::var
// Extension: .sit | Run: scriptit <file.sit> | REPL: scriptit
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <cmath>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <functional>
#include <memory>
#include <limits>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "../pythonicVars.hpp"
#include "../pythonicMath.hpp"

using pythonic::overflow::Overflow;
using pythonic::vars::Dict;
using pythonic::vars::List;
using pythonic::vars::NoneType;
using pythonic::vars::Set;
using pythonic::vars::var;

// --- Enums & Structures ---

enum class TokenType
{
    Number,
    String,
    Identifier,
    Operator,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    KeywordVar,
    KeywordFn,
    KeywordGive,
    KeywordIf,
    KeywordElif,
    KeywordElse,
    KeywordFor,
    KeywordIn,
    KeywordRange,
    KeywordFrom,
    KeywordTo,
    KeywordPass,
    KeywordWhile,
    KeywordAre,
    KeywordNew,
    KeywordLet,
    KeywordBe,
    Equals,
    Comma,
    Dot,
    Colon,
    Semicolon,
    At,
    CommentStart,
    CommentEnd,
    Eof
};

struct Token
{
    TokenType type;
    std::string value;
    int position;
    int line;
    Token(TokenType t, std::string v, int pos = -1, int ln = -1)
        : type(t), value(v), position(pos), line(ln) {}
};

// --- Helper Functions ---

bool is_math_function(const std::string &str)
{
    static const std::unordered_set<std::string> funcs = {
        "sin", "cos", "tan", "cot", "sec", "csc",
        "asin", "acos", "atan", "acot", "asec", "acsc",
        "log", "log2", "log10", "sqrt", "abs", "min", "max", "ceil", "floor", "round"};
    return funcs.count(str);
}

bool is_builtin_function(const std::string &str)
{
    static const std::unordered_set<std::string> funcs = {
        "print", "pprint", "read", "write", "readLine",
        "len", "type", "str", "int", "float",
        "append", "pop", "input", "list", "set", "range_list",
        "bool", "repr", "isinstance", "sum", "sorted", "reversed",
        "all", "any", "dict", "enumerate", "zip", "map", "abs"};
    return funcs.count(str) || is_math_function(str);
}

int get_operator_precedence(const std::string &op)
{
    static const std::unordered_map<std::string, int> precedence = {
        {"||", 1}, {"&&", 2}, {"==", 3}, {"!=", 3}, {"<", 4}, {"<=", 4}, {">", 4}, {">=", 4}, {"+", 5}, {"-", 5}, {"*", 6}, {"/", 6}, {"%", 6}, {"^", 7}, {"~", 8}, {"!", 8}};
    auto it = precedence.find(op);
    return it != precedence.end() ? it->second : 0;
}

double var_to_double(const var &v)
{
    if (v.is_int())
        return (double)v.as_int_unchecked();
    if (v.is_double())
        return v.as_double_unchecked();
    if (v.is_float())
        return (double)v.as_float_unchecked();
    if (v.is_long())
        return (double)v.as_long_unchecked();
    if (v.is_long_long())
        return (double)v.as_long_long_unchecked();
    if (v.is_long_double())
        return (double)v.as_long_double_unchecked();
    if (v.is_bool())
        return v.as_bool_unchecked() ? 1.0 : 0.0;
    if (v.is_uint())
        return (double)v.as_uint_unchecked();
    if (v.is_ulong())
        return (double)v.as_ulong_unchecked();
    if (v.is_ulong_long())
        return (double)v.as_ulong_long_unchecked();
    throw std::runtime_error("Cannot convert " + v.type() + " to number");
}

// Format var for output — backward compatible with old double-based output
std::string format_output(const var &v)
{
    if (v.is_none())
        return "None";
    if (v.is_string())
        return v.as_string_unchecked();
    if (v.is_bool())
        return v.as_bool_unchecked() ? "True" : "False";
    if (v.is_double() || v.is_float() || v.is_long_double())
    {
        double d = var_to_double(v);
        std::ostringstream ss;
        ss << d;
        return ss.str();
    }
    return v.str();
}

// --- Tokenizer ---

class Tokenizer
{
public:
    std::vector<Token> tokenize(const std::string &source)
    {
        std::vector<Token> tokens;
        int line = 1;

        static const std::unordered_map<std::string, TokenType> keywords = {
            {"var", TokenType::KeywordVar}, {"fn", TokenType::KeywordFn}, {"give", TokenType::KeywordGive}, {"if", TokenType::KeywordIf}, {"elif", TokenType::KeywordElif}, {"else", TokenType::KeywordElse}, {"for", TokenType::KeywordFor}, {"in", TokenType::KeywordIn}, {"range", TokenType::KeywordRange}, {"from", TokenType::KeywordFrom}, {"to", TokenType::KeywordTo}, {"pass", TokenType::KeywordPass}, {"while", TokenType::KeywordWhile}, {"are", TokenType::KeywordAre}, {"new", TokenType::KeywordNew}, {"let", TokenType::KeywordLet}, {"be", TokenType::KeywordBe}};

        static const std::unordered_map<char, TokenType> simpleSymbols = {
            {'+', TokenType::Operator}, {'*', TokenType::Operator}, {'/', TokenType::Operator}, {'^', TokenType::Operator}, {'%', TokenType::Operator}, {',', TokenType::Comma}, {'.', TokenType::Dot}, {':', TokenType::Colon}, {';', TokenType::Semicolon}, {'@', TokenType::At}, {'(', TokenType::LeftParen}, {')', TokenType::RightParen}, {'{', TokenType::LeftBrace}, {'}', TokenType::RightBrace}, {'[', TokenType::LeftBracket}, {']', TokenType::RightBracket}};

        for (size_t i = 0; i < source.length(); ++i)
        {
            char c = source[i];
            if (c == '\n')
            {
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
                {
                    value += source[i++];
                }
                i--;
                if (keywords.count(value))
                {
                    tokens.emplace_back(keywords.at(value), value, startPos, line);
                }
                else if (value == "and")
                {
                    tokens.emplace_back(TokenType::Operator, "&&", startPos, line);
                }
                else if (value == "or")
                {
                    tokens.emplace_back(TokenType::Operator, "||", startPos, line);
                }
                else if (value == "not")
                {
                    tokens.emplace_back(TokenType::Operator, "!", startPos, line);
                }
                else
                {
                    // True, False, None handled as identifiers → evaluator converts
                    tokens.emplace_back(TokenType::Identifier, value, startPos, line);
                }
                continue;
            }

            // Negative number hint (handled by parser unary)
            if (c == '-' && i + 1 < source.length() && std::isdigit(source[i + 1]))
            { /* fall through to operator */
            }

            // Multi-char operators
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

// --- AST Nodes ---

struct Scope;

struct ASTNode
{
    virtual ~ASTNode() = default;
};

struct Statement : ASTNode
{
    virtual void execute(Scope &scope) = 0;
};

struct Expression : ASTNode
{
    std::vector<Token> rpn;
    std::string logicalOp;
    std::shared_ptr<Expression> lhs;
    std::shared_ptr<Expression> rhs;
    virtual var evaluate(Scope &scope);
};

struct BlockStmt : Statement
{
    std::vector<std::shared_ptr<Statement>> statements;
    void execute(Scope &scope) override;
};

struct IfStmt : Statement
{
    struct Branch
    {
        std::shared_ptr<Expression> condition;
        std::shared_ptr<BlockStmt> block;
    };
    std::vector<Branch> branches;
    std::shared_ptr<BlockStmt> elseBlock;
    void execute(Scope &scope) override;
};

struct ForStmt : Statement
{
    std::string iteratorName;
    std::shared_ptr<Expression> startExpr;
    std::shared_ptr<Expression> endExpr;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override;
};

struct ForInStmt : Statement
{
    std::string iteratorName;
    std::shared_ptr<Expression> iterableExpr;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override;
};

struct FunctionDefStmt : Statement
{
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override;
};

struct ReturnStmt : Statement
{
    std::shared_ptr<Expression> expr;
    void execute(Scope &scope) override;
};

struct AssignStmt : Statement
{
    std::string name;
    std::shared_ptr<Expression> expr;
    bool isDeclaration = false;
    void execute(Scope &scope) override;
};

struct ExprStmt : Statement
{
    std::shared_ptr<Expression> expr;
    void execute(Scope &scope) override;
};

struct WhileStmt : Statement
{
    std::shared_ptr<Expression> condition;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override;
};

struct PassStmt : Statement
{
    void execute(Scope &scope) override { /* No-op */ }
};

struct MultiVarStmt : Statement
{
    std::vector<std::shared_ptr<AssignStmt>> assignments;
    void execute(Scope &scope) override
    {
        for (auto &a : assignments)
            a->execute(scope);
    }
};

// --- Environment / Scope ---

struct ReturnException : public std::exception
{
    var value;
    ReturnException(var v) : value(std::move(v)) {}
};

struct FunctionDef
{
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
};

struct Scope
{
    std::map<std::string, var> values;
    std::map<std::string, FunctionDef> functions;
    Scope *parent;
    bool barrier;

    Scope(Scope *p = nullptr, bool b = false) : parent(p), barrier(b) {}

    void define(const std::string &name, const var &val) { values[name] = val; }
    void defineFunction(const std::string &name, const FunctionDef &def) { functions[name] = def; }

    void set(const std::string &name, const var &val)
    {
        if (values.count(name))
        {
            values[name] = val;
            return;
        }
        if (parent && !barrier)
        {
            try
            {
                parent->set(name, val);
                return;
            }
            catch (const std::runtime_error &)
            {
            }
        }
        throw std::runtime_error("Undefined variable '" + name + "' in current scope (cannot mutate outer scope).");
    }

    var get(const std::string &name)
    {
        if (values.count(name))
            return values[name];
        if (parent)
            return parent->get(name);
        // Undefined variables auto-create as None
        return var(NoneType{});
    }

    FunctionDef getFunction(const std::string &name)
    {
        if (functions.count(name))
            return functions[name];
        if (parent)
            return parent->getFunction(name);
        throw std::runtime_error("Unknown function: " + name);
    }

    void clear()
    {
        values.clear();
        functions.clear();
    }
};

// --- Evaluator Implementation ---

var Expression::evaluate(Scope &scope)
{
    // Short-circuit evaluation for logical operators
    if (!logicalOp.empty() && lhs && rhs)
    {
        var leftVal = lhs->evaluate(scope);
        if (logicalOp == "&&")
        {
            if (!static_cast<bool>(leftVal))
                return var(0);
            var rightVal = rhs->evaluate(scope);
            return var(static_cast<bool>(rightVal) ? 1 : 0);
        }
        else
        {
            if (static_cast<bool>(leftVal))
                return var(1);
            var rightVal = rhs->evaluate(scope);
            return var(static_cast<bool>(rightVal) ? 1 : 0);
        }
    }

    std::stack<var> stk;

    for (const auto &token : rpn)
    {
        if (token.type == TokenType::Number)
        {
            if (token.value.find('.') != std::string::npos)
            {
                stk.push(var(std::stod(token.value)));
            }
            else
            {
                try
                {
                    stk.push(var(std::stoi(token.value)));
                }
                catch (...)
                {
                    stk.push(var((long long)std::stoll(token.value)));
                }
            }
        }
        else if (token.type == TokenType::String)
        {
            stk.push(var(token.value));
        }
        else if (token.type == TokenType::Identifier)
        {
            if (token.value == "True")
                stk.push(var(1));
            else if (token.value == "False")
                stk.push(var(0));
            else if (token.value == "None")
                stk.push(var(NoneType{}));
            else
                stk.push(scope.get(token.value));
        }
        else if (token.type == TokenType::Operator)
        {
            if (token.value == "~")
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for unary '~'");
                var a = stk.top();
                stk.pop();
                if (a.is_int())
                    stk.push(var(-a.as_int_unchecked()));
                else
                    stk.push(var(-var_to_double(a)));
            }
            else if (token.value == "!")
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for unary '!'");
                var a = stk.top();
                stk.pop();
                stk.push(var(static_cast<bool>(a) ? 0 : 1));
            }
            else
            {
                if (stk.size() < 2)
                    throw std::runtime_error("Stack underflow for binary operator '" + token.value + "'");
                var b = stk.top();
                stk.pop();
                var a = stk.top();
                stk.pop();

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
                         return pythonic::math::mul(a, b, Overflow::Promote);
                     }},
                    {"/", [](const var &a, const var &b) -> var
                     {
                         double bd = var_to_double(b);
                         if (std::abs(bd) < 1e-15)
                             throw std::runtime_error("Div by 0");
                         return pythonic::math::div(a, b, Overflow::Promote);
                     }},
                    {"%", [](const var &a, const var &b) -> var
                     {
                         double bd = var_to_double(b);
                         if (std::abs(bd) < 1e-15)
                             throw std::runtime_error("Mod by 0");
                         return pythonic::math::mod(a, b, Overflow::Promote);
                     }},
                    {"^", [](const var &a, const var &b) -> var
                     {
                         return pythonic::math::pow(a, b, Overflow::Promote);
                     }},
                    {"==", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() == b.as_string_unchecked() ? 1 : 0);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) < 1e-9 ? 1 : 0);
                     }},
                    {"!=", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() != b.as_string_unchecked() ? 1 : 0);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) > 1e-9 ? 1 : 0);
                     }},
                    {"<", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) < var_to_double(b) ? 1 : 0); }},
                    {">", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) > var_to_double(b) ? 1 : 0); }},
                    {"<=", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) <= var_to_double(b) ? 1 : 0); }},
                    {">=", [](const var &a, const var &b) -> var
                     { return var(var_to_double(a) >= var_to_double(b) ? 1 : 0); }},
                    {"&&", [](const var &a, const var &b) -> var
                     { return var((static_cast<bool>(a) && static_cast<bool>(b)) ? 1 : 0); }},
                    {"||", [](const var &a, const var &b) -> var
                     { return var((static_cast<bool>(a) || static_cast<bool>(b)) ? 1 : 0); }}};

                auto opIt = binaryOps.find(token.value);
                if (opIt != binaryOps.end())
                    stk.push(opIt->second(a, b));
                else
                    throw std::runtime_error("Unknown binary operator: " + token.value);
            }
        }
        // List literal: [expr, expr, ...] → pushed as LIST marker with count
        else if (token.type == TokenType::LeftBracket && token.value == "LIST")
        {
            int count = token.position;
            std::vector<var> temp;
            for (int i = 0; i < count; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for list literal");
                temp.push_back(stk.top());
                stk.pop();
            }
            std::reverse(temp.begin(), temp.end());
            List items(temp.begin(), temp.end());
            stk.push(var(std::move(items)));
        }
        // Set literal: {expr, expr, ...} → pushed as SET marker with count
        else if (token.type == TokenType::LeftBrace && token.value == "SET")
        {
            int count = token.position;
            Set items;
            for (int i = 0; i < count; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for set literal");
                items.insert(stk.top());
                stk.pop();
            }
            stk.push(var(std::move(items)));
        }
        // Method call: obj.method(args) — At token with method name, argc in position
        else if (token.type == TokenType::At)
        {
            std::string method = token.value;
            int argc = token.position;

            // Pop arguments first (in reverse order)
            std::vector<var> args;
            for (int i = 0; i < argc; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for method args");
                args.push_back(stk.top());
                stk.pop();
            }
            std::reverse(args.begin(), args.end());

            // Pop self (the object)
            if (stk.empty())
                throw std::runtime_error("Stack underflow for method call (no object)");
            var self = stk.top();
            stk.pop();

            // --- Dispatch method calls ---
            // String methods
            using Method0 = std::function<var(var &)>;
            using Method1 = std::function<var(var &, const var &)>;
            using Method2 = std::function<var(var &, const var &, const var &)>;

            static const std::unordered_map<std::string, Method0> methods0 = {
                {"upper", [](var &s) -> var { return s.upper(); }},
                {"lower", [](var &s) -> var { return s.lower(); }},
                {"strip", [](var &s) -> var { return s.strip(); }},
                {"lstrip", [](var &s) -> var { return s.lstrip(); }},
                {"rstrip", [](var &s) -> var { return s.rstrip(); }},
                {"capitalize", [](var &s) -> var { return s.capitalize(); }},
                {"title", [](var &s) -> var { return s.title(); }},
                {"clear", [](var &s) -> var
                 { s.clear(); return var(NoneType{}); }},
                {"empty", [](var &s) -> var
                 { return var(s.empty() ? 1 : 0); }},
                {"front", [](var &s) -> var { return s.front(); }},
                {"back", [](var &s) -> var { return s.back(); }},
                {"keys", [](var &s) -> var { return s.keys(); }},
                {"values", [](var &s) -> var { return s.values(); }},
                {"items", [](var &s) -> var { return s.items(); }},
                {"pop", [](var &s) -> var { return s.pop(); }},
                {"sort", [](var &s) -> var { s.sort(); return s; }},
                {"reverse", [](var &s) -> var { s.reverse(); return s; }},
                {"str", [](var &s) -> var { return var(s.str()); }},
                {"pretty_str", [](var &s) -> var { return var(s.pretty_str()); }},
                {"type", [](var &s) -> var { return var(s.type()); }},
                {"len", [](var &s) -> var { return s.len(); }},
                {"isNone", [](var &s) -> var { return var(s.isNone() ? 1 : 0); }},
                {"isNumeric", [](var &s) -> var { return var(s.isNumeric() ? 1 : 0); }},
                {"isIntegral", [](var &s) -> var { return var(s.isIntegral() ? 1 : 0); }},
                {"is_list", [](var &s) -> var { return var(s.is_list() ? 1 : 0); }},
                {"is_dict", [](var &s) -> var { return var(s.is_dict() ? 1 : 0); }},
                {"is_set", [](var &s) -> var { return var(s.is_set() ? 1 : 0); }},
                {"is_string", [](var &s) -> var { return var(s.is_string() ? 1 : 0); }},
                {"is_int", [](var &s) -> var { return var(s.is_int() ? 1 : 0); }},
                {"is_double", [](var &s) -> var { return var(s.is_double() ? 1 : 0); }},
                {"is_float", [](var &s) -> var { return var(s.is_float() ? 1 : 0); }},
                {"is_bool", [](var &s) -> var { return var(s.is_bool() ? 1 : 0); }},
                {"is_none", [](var &s) -> var { return var(s.is_none() ? 1 : 0); }},
                {"is_any_integral", [](var &s) -> var { return var(s.is_any_integral() ? 1 : 0); }},
                {"is_any_floating", [](var &s) -> var { return var(s.is_any_floating() ? 1 : 0); }},
                {"is_any_numeric", [](var &s) -> var { return var(s.is_any_numeric() ? 1 : 0); }},
                {"toInt", [](var &s) -> var { return var(s.toInt()); }},
                {"toDouble", [](var &s) -> var { return var(s.toDouble()); }},
                {"toLongDouble", [](var &s) -> var { return var(s.toLongDouble()); }},
                {"hash", [](var &s) -> var { return var((long long)s.hash()); }},
                {"sentence_case", [](var &s) -> var { return s.sentence_case(); }},
            };

            static const std::unordered_map<std::string, Method1> methods1 = {
                {"append", [](var &s, const var &a) -> var { s.append(a); return s; }},
                {"contains", [](var &s, const var &a) -> var { return var(s.contains(a) ? 1 : 0); }},
                {"has", [](var &s, const var &a) -> var { return var(s.has(a) ? 1 : 0); }},
                {"remove", [](var &s, const var &a) -> var { s.remove(a); return s; }},
                {"count", [](var &s, const var &a) -> var { return s.count(a); }},
                {"index", [](var &s, const var &a) -> var { return s.index(a); }},
                {"find", [](var &s, const var &a) -> var { return s.find(a); }},
                {"startswith", [](var &s, const var &a) -> var { return var(s.startswith(a) ? 1 : 0); }},
                {"endswith", [](var &s, const var &a) -> var { return var(s.endswith(a) ? 1 : 0); }},
                {"split", [](var &s, const var &a) -> var { return s.split(a); }},
                {"join", [](var &s, const var &a) -> var { return s.join(a); }},
                {"extend", [](var &s, const var &a) -> var { s.extend(a); return s; }},
                {"update", [](var &s, const var &a) -> var { s.update(a); return s; }},
                {"insert", [](var &s, const var &a) -> var { s.insert(a); return s; }},
                {"zfill", [](var &s, const var &a) -> var { return s.zfill(a); }},
                {"ljust", [](var &s, const var &a) -> var { return s.ljust(a); }},
                {"rjust", [](var &s, const var &a) -> var { return s.rjust(a); }},
            };

            static const std::unordered_map<std::string, Method2> methods2 = {
                {"replace", [](var &s, const var &a, const var &b) -> var { return s.replace(a, b); }},
                {"center", [](var &s, const var &a, const var &b) -> var { return s.center(a, b); }},
                {"slice", [](var &s, const var &a, const var &b) -> var { return s.slice(a, b); }},
            };

            if (argc == 0)
            {
                auto it = methods0.find(method);
                if (it != methods0.end())
                {
                    stk.push(it->second(self));
                    continue;
                }
                // Try 1-arg method with no arg (like split with default)
                if (method == "split")
                {
                    stk.push(self.split(var(" ")));
                    continue;
                }
            }
            else if (argc == 1)
            {
                auto it = methods1.find(method);
                if (it != methods1.end())
                {
                    stk.push(it->second(self, args[0]));
                    continue;
                }
                // Some 0-arg methods called with arg could be an error
                auto it0 = methods0.find(method);
                if (it0 != methods0.end())
                {
                    throw std::runtime_error("Method '" + method + "' takes 0 arguments but got 1");
                }
            }
            else if (argc == 2)
            {
                auto it = methods2.find(method);
                if (it != methods2.end())
                {
                    stk.push(it->second(self, args[0], args[1]));
                    continue;
                }
            }

            throw std::runtime_error("Unknown method '" + method + "' with " + std::to_string(argc) + " argument(s)");
        }
        // Function calls (built-in or user-defined)
        else if (token.type == TokenType::KeywordFn)
        {
            std::string fname = token.value;
            int argc = token.position;

            // --- Built-in math functions ---
            if (is_math_function(fname))
            {
                if (fname == "min" || fname == "max")
                {
                    if (stk.size() < 2)
                        throw std::runtime_error("Missing args for " + fname);
                    var b = stk.top();
                    stk.pop();
                    var a = stk.top();
                    stk.pop();
                    stk.push(fname == "min" ? pythonic::math::min(a, b) : pythonic::math::max(a, b));
                }
                else
                {
                    // Dispatch map using pythonic::math where available, std:: for the rest
                    using MathFn = std::function<var(const var &)>;
                    static const std::unordered_map<std::string, MathFn> mathOps = {
                        {"sin", [](const var &x)
                         { return pythonic::math::sin(x); }},
                        {"cos", [](const var &x)
                         { return pythonic::math::cos(x); }},
                        {"tan", [](const var &x)
                         { return pythonic::math::tan(x); }},
                        {"asin", [](const var &x)
                         { return pythonic::math::asin(x); }},
                        {"acos", [](const var &x)
                         { return pythonic::math::acos(x); }},
                        {"atan", [](const var &x)
                         { return pythonic::math::atan(x); }},
                        {"log", [](const var &x)
                         { return pythonic::math::log(x); }},
                        {"log2", [](const var &x)
                         { return pythonic::math::log2(x); }},
                        {"log10", [](const var &x)
                         { return pythonic::math::log10(x); }},
                        {"sqrt", [](const var &x)
                         { return pythonic::math::sqrt(x); }},
                        {"abs", [](const var &x)
                         { return pythonic::math::fabs(x); }},
                        {"ceil", [](const var &x)
                         { return pythonic::math::ceil(x); }},
                        {"floor", [](const var &x)
                         { return pythonic::math::floor(x); }},
                        {"round", [](const var &x)
                         { return pythonic::math::round(x); }},
                        {"cot", [](const var &x)
                         { return pythonic::math::cot(x); }},
                        {"sec", [](const var &x)
                         { return pythonic::math::sec(x); }},
                        {"csc", [](const var &x)
                         { return pythonic::math::csc(x); }}};
                    if (stk.empty())
                        throw std::runtime_error("Missing arg for " + fname);
                    var arg = stk.top();
                    stk.pop();
                    stk.push(mathOps.at(fname)(arg));
                }
                continue;
            }

            // --- Built-in utility functions (dispatch map) ---
            using BuiltinFn = std::function<void(std::stack<var> &, int)>;
            static const std::unordered_map<std::string, BuiltinFn> builtins = {
                {"print", [](std::stack<var> &s, int argc)
                 {
                     std::vector<var> args;
                     for (int i = 0; i < argc; ++i)
                     {
                         if (s.empty())
                             throw std::runtime_error("Stack underflow for print");
                         args.push_back(s.top());
                         s.pop();
                     }
                     std::reverse(args.begin(), args.end());
                     for (size_t i = 0; i < args.size(); ++i)
                     {
                         if (i > 0)
                             std::cout << " ";
                         std::cout << (args[i].is_string() ? args[i].as_string_unchecked() : args[i].str());
                     }
                     std::cout << std::endl;
                     s.push(var(NoneType{}));
                 }},
                {"pprint", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("pprint() takes 1 argument");
                     var a = s.top();
                     s.pop();
                     std::cout << a.pretty_str() << std::endl;
                     s.push(var(NoneType{}));
                 }},
                {"len", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("len() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(a.len());
                 }},
                {"type", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("type() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(a.type()));
                 }},
                {"str", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("str() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(a.str()));
                 }},
                {"int", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("int() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(a.toInt()));
                 }},
                {"float", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("float() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(a.toDouble()));
                 }},
                {"append", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 2)
                         throw std::runtime_error("append(list, item) takes exactly 2 arguments");
                     var item = s.top();
                     s.pop();
                     var lst = s.top();
                     s.pop();
                     if (!lst.is_list())
                         throw std::runtime_error("append() requires a list as first argument");
                     lst.append(item);
                     s.push(lst);
                 }},
                {"pop", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("pop() takes exactly 1 argument");
                     var lst = s.top();
                     s.pop();
                     if (!lst.is_list())
                         throw std::runtime_error("pop() requires a list");
                     s.push(lst.pop());
                 }},
                {"list", [](std::stack<var> &s, int argc)
                 {
                     if (argc == 0)
                     {
                         s.push(var(List{}));
                         return;
                     }
                     var a = s.top();
                     s.pop();
                     s.push(a);
                 }},
                {"set", [](std::stack<var> &s, int argc)
                 {
                     if (argc == 0)
                     {
                         s.push(var(Set{}));
                         return;
                     }
                     var a = s.top();
                     s.pop();
                     s.push(a);
                 }},
                {"read", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("read(filename) takes exactly 1 argument");
                     var fn = s.top();
                     s.pop();
                     if (!fn.is_string())
                         throw std::runtime_error("read() expects a string filename");
                     std::ifstream f(fn.as_string_unchecked());
                     if (!f.is_open())
                         throw std::runtime_error("Cannot open file: " + fn.as_string_unchecked());
                     std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                     s.push(var(content));
                 }},
                {"readLine", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("readLine(filename) takes exactly 1 argument");
                     var fn = s.top();
                     s.pop();
                     if (!fn.is_string())
                         throw std::runtime_error("readLine() expects a string filename");
                     std::ifstream f(fn.as_string_unchecked());
                     if (!f.is_open())
                         throw std::runtime_error("Cannot open file: " + fn.as_string_unchecked());
                     List lines;
                     std::string line;
                     while (std::getline(f, line))
                         lines.push_back(var(line));
                     s.push(var(std::move(lines)));
                 }},
                {"write", [](std::stack<var> &s, int argc)
                 {
                     if (argc < 2 || argc > 3)
                         throw std::runtime_error("write(filename, data [, mode]) takes 2-3 arguments");
                     std::string mode = "w";
                     if (argc == 3)
                     {
                         mode = s.top().is_string() ? s.top().as_string_unchecked() : "w";
                         s.pop();
                     }
                     var data = s.top();
                     s.pop();
                     var fn = s.top();
                     s.pop();
                     if (!fn.is_string())
                         throw std::runtime_error("write() expects a string filename");
                     std::ios_base::openmode om = std::ios_base::out;
                     if (mode == "a")
                         om |= std::ios_base::app;
                     std::ofstream f(fn.as_string_unchecked(), om);
                     if (!f.is_open())
                         throw std::runtime_error("Cannot open file for writing: " + fn.as_string_unchecked());
                     f << (data.is_string() ? data.as_string_unchecked() : data.str());
                     s.push(var(NoneType{}));
                 }},
                {"input", [](std::stack<var> &s, int argc)
                 {
                     std::string prompt;
                     if (argc >= 1)
                     {
                         var pv = s.top();
                         s.pop();
                         prompt = pv.is_string() ? pv.as_string_unchecked() : pv.str();
                     }
                     std::cout << prompt;
                     std::cout.flush();
                     std::string inputLine;
                     std::getline(std::cin, inputLine);
                     s.push(var(inputLine));
                 }},
                {"bool", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("bool() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(static_cast<bool>(a) ? 1 : 0));
                 }},
                {"repr", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("repr() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(var(a.pretty_str()));
                 }},
                {"isinstance", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 2)
                         throw std::runtime_error("isinstance(obj, type_name) takes exactly 2 arguments");
                     var typeName = s.top();
                     s.pop();
                     var obj = s.top();
                     s.pop();
                     std::string tn = typeName.is_string() ? typeName.as_string_unchecked() : typeName.str();
                     s.push(var(obj.type() == tn ? 1 : 0));
                 }},
                {"sum", [](std::stack<var> &s, int argc)
                 {
                     if (argc < 1 || argc > 2)
                         throw std::runtime_error("sum(iterable[, start]) takes 1-2 arguments");
                     var start = var(0);
                     if (argc == 2)
                     {
                         start = s.top();
                         s.pop();
                     }
                     var lst = s.top();
                     s.pop();
                     if (!lst.is_list())
                         throw std::runtime_error("sum() requires a list");
                     var total = start;
                     for (const auto &item : lst)
                         total = pythonic::math::add(total, item, Overflow::Promote);
                     s.push(total);
                 }},
                {"sorted", [](std::stack<var> &s, int argc)
                 {
                     if (argc < 1 || argc > 2)
                         throw std::runtime_error("sorted(iterable[, reverse]) takes 1-2 arguments");
                     bool rev = false;
                     if (argc == 2)
                     {
                         rev = static_cast<bool>(s.top());
                         s.pop();
                     }
                     var lst = s.top();
                     s.pop();
                     var copy = lst;
                     copy.sort();
                     if (rev)
                         copy.reverse();
                     s.push(copy);
                 }},
                {"reversed", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("reversed() takes exactly 1 argument");
                     var lst = s.top();
                     s.pop();
                     var copy = lst;
                     copy.reverse();
                     s.push(copy);
                 }},
                {"all", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("all() takes exactly 1 argument");
                     var lst = s.top();
                     s.pop();
                     if (!lst.is_list())
                         throw std::runtime_error("all() requires a list");
                     for (const auto &item : lst)
                     {
                         if (!static_cast<bool>(item))
                         {
                             s.push(var(0));
                             return;
                         }
                     }
                     s.push(var(1));
                 }},
                {"any", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("any() takes exactly 1 argument");
                     var lst = s.top();
                     s.pop();
                     if (!lst.is_list())
                         throw std::runtime_error("any() requires a list");
                     for (const auto &item : lst)
                     {
                         if (static_cast<bool>(item))
                         {
                             s.push(var(1));
                             return;
                         }
                     }
                     s.push(var(0));
                 }},
                {"dict", [](std::stack<var> &s, int argc)
                 {
                     if (argc == 0)
                     {
                         s.push(var(Dict{}));
                         return;
                     }
                     var a = s.top();
                     s.pop();
                     s.push(a);
                 }},
                {"range_list", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 2)
                         throw std::runtime_error("range_list(start, end) takes exactly 2 arguments");
                     var endVal = s.top();
                     s.pop();
                     var startVal = s.top();
                     s.pop();
                     int sv = startVal.toInt();
                     int ev = endVal.toInt();
                     List result;
                     if (sv <= ev)
                     {
                         for (int i = sv; i <= ev; ++i)
                             result.push_back(var(i));
                     }
                     else
                     {
                         for (int i = sv; i >= ev; --i)
                             result.push_back(var(i));
                     }
                     s.push(var(std::move(result)));
                 }},
                {"enumerate", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("enumerate() takes exactly 1 argument");
                     var lst = s.top();
                     s.pop();
                     List result;
                     int idx = 0;
                     for (const auto &item : lst)
                     {
                         List pair;
                         pair.push_back(var(idx));
                         pair.push_back(item);
                         result.push_back(var(std::move(pair)));
                         idx++;
                     }
                     s.push(var(std::move(result)));
                 }},
                {"zip", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 2)
                         throw std::runtime_error("zip() takes exactly 2 arguments");
                     var lst2 = s.top();
                     s.pop();
                     var lst1 = s.top();
                     s.pop();
                     if (!lst1.is_list() || !lst2.is_list())
                         throw std::runtime_error("zip() requires two lists");
                     List result;
                     size_t minLen = std::min(lst1.len().toInt(), lst2.len().toInt());
                     for (size_t i = 0; i < minLen; ++i)
                     {
                         List pair;
                         pair.push_back(lst1[var((int)i)]);
                         pair.push_back(lst2[var((int)i)]);
                         result.push_back(var(std::move(pair)));
                     }
                     s.push(var(std::move(result)));
                 }},
                {"map", [](std::stack<var> &s, int argc)
                 {
                     // map(func_name, list) — Note: limited, uses string func name
                     if (argc != 2)
                         throw std::runtime_error("map() takes exactly 2 arguments");
                     var lst = s.top();
                     s.pop();
                     var funcName = s.top();
                     s.pop();
                     // For map, we just return the list (can't easily apply user functions here)
                     // This is a placeholder — user can use for-in loops
                     s.push(lst);
                 }},
                {"abs", [](std::stack<var> &s, int argc)
                 {
                     if (argc != 1)
                         throw std::runtime_error("abs() takes exactly 1 argument");
                     var a = s.top();
                     s.pop();
                     s.push(pythonic::math::fabs(a));
                 }}};

            auto builtinIt = builtins.find(fname);
            if (builtinIt != builtins.end())
            {
                builtinIt->second(stk, argc);
                continue;
            }

            // --- User-defined function call ---
            try
            {
                FunctionDef def = scope.getFunction(fname);
                if ((int)def.params.size() != argc)
                {
                    throw std::runtime_error("Function argument mismatch: expected " +
                                             std::to_string(def.params.size()) + " but got " + std::to_string(argc));
                }
                if ((int)stk.size() < argc)
                    throw std::runtime_error("Stack underflow for args");
                Scope funcScope(&scope, true);
                std::vector<var> args;
                for (int i = 0; i < argc; ++i)
                {
                    args.push_back(stk.top());
                    stk.pop();
                }
                std::reverse(args.begin(), args.end());
                for (size_t i = 0; i < def.params.size(); ++i)
                    funcScope.define(def.params[i], args[i]);
                try
                {
                    def.body->execute(funcScope);
                }
                catch (ReturnException &ret)
                {
                    stk.push(ret.value);
                    continue;
                }
                stk.push(var(0)); // Void return
            }
            catch (const std::runtime_error &e)
            {
                if (std::string(e.what()).find("Unknown function") != std::string::npos)
                    throw std::runtime_error("Unknown function call: " + fname);
                throw;
            }
        }
    }

    if (stk.empty())
        return var(0);
    return stk.top();
}

// --- Statement Implementations ---

void BlockStmt::execute(Scope &scope)
{
    Scope blockScope(&scope, false);
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
    Scope loopScope(&scope);
    loopScope.define(iteratorName, var(start));
    double step = (end >= start) ? 1.0 : -1.0;
    double current = start;
    while ((step > 0 && current <= end) || (step < 0 && current >= end))
    {
        loopScope.set(iteratorName, var(current));
        body->execute(loopScope);
        current += step;
    }
}

void ForInStmt::execute(Scope &scope)
{
    var iterable = iterableExpr->evaluate(scope);
    if (!iterable.is_list() && !iterable.is_string() && !iterable.is_set())
    {
        throw std::runtime_error("for-in requires a list, string, or set; got " + iterable.type());
    }
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
    FunctionDef def;
    def.name = name;
    def.params = params;
    def.body = body;
    scope.defineFunction(name, def);
}

void ReturnStmt::execute(Scope &scope)
{
    var val = expr->evaluate(scope);
    throw ReturnException(val);
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
    {
        std::cout << format_output(val) << std::endl;
    }
}

// --- Parser ---

class Parser
{
    const std::vector<Token> &tokens;
    size_t pos = 0;

public:
    Parser(const std::vector<Token> &t) : tokens(t) {}

    std::shared_ptr<BlockStmt> parseProgram()
    {
        auto block = std::make_shared<BlockStmt>();
        while (!isAtEnd())
            block->statements.push_back(parseStatement());
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

        // let x be expr.
        if (match(TokenType::KeywordLet))
        {
            Token name = consume(TokenType::Identifier, "Expected identifier after let");
            consume(TokenType::KeywordBe, "Expected 'be' after let <name>");
            auto expr = parseExpression();
            consumeDotOrForgive();
            auto assign = std::make_shared<AssignStmt>();
            assign->name = name.value;
            assign->expr = expr;
            assign->isDeclaration = true;
            return assign;
        }

        // var declarations — unified handling for all patterns:
        // var a.                    → None
        // var a, b, c.              → all None
        // var a = expr.             → single with value
        // var a = expr, b = expr.   → comma-separated with values
        // var a = expr, b.          → mixed: a has value, b is None
        // var a = 3 b = 3.          → space-separated (dot terminates)
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
                {
                    assign->expr = parseExpression();
                }
                else
                {
                    // No initializer → None
                    auto noneExpr = std::make_shared<Expression>();
                    noneExpr->rpn.push_back(Token(TokenType::Identifier, "None", -1, varName.line));
                    assign->expr = noneExpr;
                }
                return assign;
            };

            multi->assignments.push_back(parseOneVar());

            // Continue parsing more declarations separated by comma or space
            while (true)
            {
                if (match(TokenType::Comma))
                {
                    // After comma, expect another var name
                    multi->assignments.push_back(parseOneVar());
                }
                else if (check(TokenType::Identifier) && !is_builtin_function(peek().value) &&
                         peek().value != "True" && peek().value != "False" && peek().value != "None" &&
                         (peekNext().type == TokenType::Equals ||
                          peekNext().type == TokenType::Dot ||
                          peekNext().type == TokenType::Comma ||
                          peekNext().type == TokenType::Identifier ||
                          peekNext().type == TokenType::Eof))
                {
                    // Space-separated: var a=3 b=3.
                    multi->assignments.push_back(parseOneVar());
                }
                else
                {
                    break;
                }
            }
            consumeDotOrForgive();

            if (multi->assignments.size() == 1)
                return multi->assignments[0]; // Single var → return directly
            return multi;
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

        // for i in range(from x to y): → range-based
        if (check(TokenType::KeywordRange))
        {
            consume(TokenType::KeywordRange, "Expected range");
            consume(TokenType::LeftParen, "Expected (");
            consume(TokenType::KeywordFrom, "Expected from");
            auto start = parseExpression();
            consume(TokenType::KeywordTo, "Expected to");
            auto end = parseExpression();
            consume(TokenType::RightParen, "Expected )");
            consume(TokenType::Colon, "Expected :");
            auto body = parseBlock({TokenType::Semicolon});
            consume(TokenType::Semicolon, "Expected ; after loop");
            auto stmt = std::make_shared<ForStmt>();
            stmt->iteratorName = iter.value;
            stmt->startExpr = start;
            stmt->endExpr = end;
            stmt->body = body;
            return stmt;
        }

        // for i in <expr>: → iterate over list/string/set
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
        std::vector<TokenType> terminators = {TokenType::Semicolon};
        stmt->body = parseBlock(terminators);
        consume(TokenType::Semicolon, "Expected ; after while body");
        return stmt;
    }

    std::shared_ptr<FunctionDefStmt> parseFunction()
    {
        auto stmt = std::make_shared<FunctionDefStmt>();
        stmt->name = consume(TokenType::Identifier, "Expected function name").value;
        consume(TokenType::At, "Expected @ after function name");
        consume(TokenType::LeftParen, "Expected ( for params");
        if (!check(TokenType::RightParen))
        {
            do
            {
                stmt->params.push_back(consume(TokenType::Identifier, "Expected param name").value);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "Expected ) after params");
        consume(TokenType::Colon, "Expected : start of function body");
        stmt->body = parseBlock({TokenType::Semicolon});
        consume(TokenType::Semicolon, "Expected ; after function body");
        if (stmt->body->statements.empty())
            throw std::runtime_error("Empty function body not allowed, use 'pass'.");
        return stmt;
    }

    std::shared_ptr<ReturnStmt> parseReturn()
    {
        consume(TokenType::LeftParen, "Expected ( after give");
        auto expr = parseExpression();
        consume(TokenType::RightParen, "Expected ) after give expr");
        consumeDotOrForgive();
        auto stmt = std::make_shared<ReturnStmt>();
        stmt->expr = expr;
        return stmt;
    }

    std::shared_ptr<BlockStmt> parseBlock(const std::vector<TokenType> &terminators)
    {
        auto block = std::make_shared<BlockStmt>();
        while (!isAtEnd())
        {
            for (auto t : terminators)
                if (check(t))
                    return block;
            block->statements.push_back(parseStatement());
        }
        return block;
    }

    // --- Expression Parsing (with short-circuit) ---

    std::shared_ptr<Expression> parseExpression() { return parseLogicalOr(); }

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

    // Shunting-Yard for everything except top-level && and ||
    std::shared_ptr<Expression> parsePrimaryExpr()
    {
        auto expr = std::make_shared<Expression>();
        std::queue<Token> out;
        std::stack<Token> opStack;
        TokenType lastTokenType = TokenType::Eof;

        while (!isAtEnd())
        {
            Token t = peek();

            // End of expression — but check for dot-method call first
            if (t.type == TokenType::Dot)
            {
                // Check if this is obj.method(...) — look ahead
                if (pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::Identifier &&
                    pos + 2 < tokens.size() && tokens[pos + 2].type == TokenType::LeftParen)
                {
                    advance(); // consume '.'
                    Token methodName = advance(); // consume method name
                    consume(TokenType::LeftParen, "(");
                    int argCount = 0;
                    if (!check(TokenType::RightParen))
                    {
                        do
                        {
                            auto argExpr = parseExpression();
                            if (argExpr->logicalOp.empty())
                            {
                                for (auto &at : argExpr->rpn)
                                    out.push(at);
                            }
                            else
                            {
                                flattenExprToQueue(argExpr, out);
                            }
                            argCount++;
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RightParen, ")");
                    // Push a METHODCALL token: value=method name, position=argCount (not counting self)
                    out.push(Token(TokenType::At, methodName.value, argCount, methodName.line));
                    lastTokenType = TokenType::Identifier;
                    continue;
                }
                break; // regular dot terminator
            }
            if (t.type == TokenType::Colon || t.type == TokenType::Semicolon ||
                t.type == TokenType::KeywordIn || t.type == TokenType::KeywordTo ||
                t.type == TokenType::KeywordElif || t.type == TokenType::KeywordElse ||
                t.type == TokenType::KeywordBe || t.type == TokenType::Equals)
                break;

            // Stop at && / || outside parens
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
            // Stop at , / ) outside parens
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
            // Stop at ] / } outside brackets
            if (t.type == TokenType::RightBracket || t.type == TokenType::RightBrace)
            {
                break;
            }

            Token token = advance();

            // Implicit multiplication: Number/RightParen followed by Number/Identifier/LeftParen
            if ((token.type == TokenType::Number || token.type == TokenType::Identifier || token.type == TokenType::LeftParen) &&
                (lastTokenType == TokenType::Number || lastTokenType == TokenType::RightParen))
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
            {
                out.push(token);
            }
            else if (token.type == TokenType::String)
            {
                out.push(token);
            }
            else if (token.type == TokenType::Identifier)
            {
                if (check(TokenType::LeftParen))
                {
                    // Function call
                    consume(TokenType::LeftParen, "(");
                    int argCount = 0;
                    if (!check(TokenType::RightParen))
                    {
                        do
                        {
                            auto argExpr = parseExpression();
                            if (argExpr->logicalOp.empty())
                            {
                                for (auto &at : argExpr->rpn)
                                    out.push(at);
                            }
                            else
                            {
                                flattenExprToQueue(argExpr, out);
                            }
                            argCount++;
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RightParen, ")");
                    out.push(Token(TokenType::KeywordFn, token.value, argCount, token.line));
                    token.type = TokenType::Identifier; // for lastTokenType
                }
                else
                {
                    out.push(token);
                }
            }
            else if (token.type == TokenType::LeftBracket)
            {
                // List literal: [expr, expr, ...]
                int count = 0;
                if (!check(TokenType::RightBracket))
                {
                    do
                    {
                        auto elemExpr = parseExpression();
                        if (elemExpr->logicalOp.empty())
                        {
                            for (auto &at : elemExpr->rpn)
                                out.push(at);
                        }
                        else
                        {
                            flattenExprToQueue(elemExpr, out);
                        }
                        count++;
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RightBracket, "Expected ] to close list");
                out.push(Token(TokenType::LeftBracket, "LIST", count, token.line));
                lastTokenType = TokenType::RightBracket;
                continue; // skip normal lastTokenType assignment
            }
            else if (token.type == TokenType::LeftBrace)
            {
                // Set literal: {expr, expr, ...}
                int count = 0;
                if (!check(TokenType::RightBrace))
                {
                    do
                    {
                        auto elemExpr = parseExpression();
                        if (elemExpr->logicalOp.empty())
                        {
                            for (auto &at : elemExpr->rpn)
                                out.push(at);
                        }
                        else
                        {
                            flattenExprToQueue(elemExpr, out);
                        }
                        count++;
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RightBrace, "Expected } to close set");
                out.push(Token(TokenType::LeftBrace, "SET", count, token.line));
                lastTokenType = TokenType::RightBrace;
                continue;
            }
            else if (token.type == TokenType::Operator)
            {
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
            else if (token.type == TokenType::LeftParen)
            {
                opStack.push(token);
            }
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
        {
            for (auto &t : expr->rpn)
                out.push(t);
        }
        else
        {
            flattenExprToQueue(expr->lhs, out);
            flattenExprToQueue(expr->rhs, out);
            out.push(Token(TokenType::Operator, expr->logicalOp, -1, -1));
        }
    }

private:
    int lastConsumedLine = 1; // Track line of last consumed token
    Token peek() { return pos < tokens.size() ? tokens[pos] : tokens.back(); }
    Token peekNext() { return pos + 1 < tokens.size() ? tokens[pos + 1] : tokens.back(); }
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
        if (check(t))
        {
            advance();
            return true;
        }
        return false;
    }

    // Check a sequence of upcoming tokens without consuming
    bool checkSequence(const std::vector<TokenType> &seq)
    {
        for (size_t i = 0; i < seq.size(); ++i)
        {
            size_t idx = pos + i;
            if (idx >= tokens.size())
                return false;
            if (tokens[idx].type != seq[i])
                return false;
        }
        return true;
    }

    Token consume(TokenType t, std::string err)
    {
        if (check(t))
            return advance();
        throw std::runtime_error(err + " at line " + std::to_string(peek().line));
    }

    // Forgive missing '.' at scope/line boundaries
    // Forgive at: EOF, ;, elif, else, OR when next token is on a different line
    Token consumeDotOrForgive()
    {
        if (check(TokenType::Dot))
            return advance();
        if (isAtEnd() || check(TokenType::Semicolon) ||
            check(TokenType::KeywordElif) || check(TokenType::KeywordElse))
            return Token(TokenType::Dot, ".", -1, peek().line);
        // Forgive if the next token is on a different line (end-of-line forgiveness)
        if (peek().line > lastConsumedLine)
            return Token(TokenType::Dot, ".", -1, lastConsumedLine);
        throw std::runtime_error("Expected '.' at line " + std::to_string(lastConsumedLine));
    }
};

// --- Execute Script Helper ---

void executeScript(const std::string &content)
{
    if (content.empty())
        return;
    try
    {
        Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(content);
        Parser parser(tokens);
        auto program = parser.parseProgram();
        Scope globalScope;
        globalScope.define("PI", var(3.14159265));
        globalScope.define("e", var(2.7182818));
        for (auto &stmt : program->statements)
            stmt->execute(globalScope);
    }
    catch (ReturnException &e)
    {
        std::cout << format_output(e.value) << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// --- Main ---

int main(int argc, char *argv[])
{
    // scriptit --test
    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        std::string source =
            "var a = 10. \n"
            "--> Comment Test <-- \n"
            "fn add @(x, y): give(x+y). ; \n"
            "var result = add(a, 20). \n"
            "if result > 20: \n"
            "   result = result + 1. \n"
            "; \n"
            "var loopSum = 0. \n"
            "for i in range(from 1 to 5): \n"
            "   loopSum = loopSum + i. \n"
            "; ";
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(source);
            Parser parser(tokens);
            auto program = parser.parseProgram();
            Scope global;
            global.define("PI", var(3.14159));
            for (auto &stmt : program->statements)
                stmt->execute(global);
            std::cout << "Result: " << global.get("result").str() << " (Expected 31)" << std::endl;
            std::cout << "LoopSum: " << global.get("loopSum").str() << " (Expected 15)" << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Test Failed: " << e.what() << std::endl;
        }
        return 0;
    }

    // scriptit --script (read from stdin)
    if (argc > 1 && std::string(argv[1]) == "--script")
    {
        std::string content((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        executeScript(content);
        return 0;
    }

    // scriptit <file.sit> or scriptit <any_file> (run script file)
    if (argc > 1)
    {
        std::string filename = argv[1];
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
            return 1;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        executeScript(content);
        return 0;
    }

    // --- Interactive REPL ---
    std::cout << "ScriptIt REPL v2 (powered by pythonic::var)" << std::endl;
    std::cout << "Type 'exit' to quit, 'clear' to clear screen, 'wipe' for fresh start." << std::endl;
    std::string line;
    Scope globalScope;
    globalScope.define("PI", var(3.14159265));
    globalScope.define("e", var(2.7182818));
    globalScope.define("ans", var(0));

    while (true)
    {
        std::cout << ">> ";
        if (!std::getline(std::cin, line) || line == "exit")
            break;
        if (line.empty())
            continue;
        if (line == "clear")
        {
#ifdef _WIN32
            std::system("cls");
#else
            std::system("clear");
#endif
            continue;
        }
        if (line == "wipe")
        {
#ifdef _WIN32
            std::system("cls");
#else
            std::system("clear");
#endif
            globalScope.clear();
            globalScope.define("PI", var(3.14159265));
            globalScope.define("e", var(2.7182818));
            globalScope.define("ans", var(0));
            std::cout << "Session wiped. All variables and functions cleared." << std::endl;
            continue;
        }
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(line);
            Parser parser(tokens);
            auto program = parser.parseProgram();
            for (auto &stmt : program->statements)
                stmt->execute(globalScope);
        }
        catch (ReturnException &e)
        {
            std::cout << format_output(e.value) << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}
