#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <cmath>
#include <map>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <functional>
#include <memory>
#include <limits>
#include <cstdlib>

// --- Enums & Strutures ---

enum class TokenType
{
    Number,
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
    Equals,
    Comma,
    Dot,
    Colon,
    Semicolon,
    At,
    CommentStart,
    CommentEnd, // Internal use
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
    static const std::vector<std::string> funcs = {
        "sin", "cos", "tan", "cot", "sec", "csc",
        "asin", "acos", "atan", "acot", "asec", "acsc",
        "log", "log2", "log10", "sqrt", "abs", "min", "max", "ceil", "floor", "round"};
    return std::find(funcs.begin(), funcs.end(), str) != funcs.end();
}

int get_operator_precedence(const std::string &op)
{
    if (op == "||")
        return 1;
    if (op == "&&")
        return 2;
    if (op == "==" || op == "!=")
        return 3;
    if (op == "<" || op == "<=" || op == ">" || op == ">=")
        return 4;
    if (op == "+" || op == "-")
        return 5;
    if (op == "*" || op == "/" || op == "%")
        return 6;
    if (op == "^")
        return 7;
    if (op == "~" || op == "!")
        return 8; // Unary
    return 0;
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
            {"var", TokenType::KeywordVar}, {"fn", TokenType::KeywordFn}, {"give", TokenType::KeywordGive}, {"if", TokenType::KeywordIf}, {"elif", TokenType::KeywordElif}, {"else", TokenType::KeywordElse}, {"for", TokenType::KeywordFor}, {"in", TokenType::KeywordIn}, {"range", TokenType::KeywordRange}, {"from", TokenType::KeywordFrom}, {"to", TokenType::KeywordTo}, {"pass", TokenType::KeywordPass}, {"while", TokenType::KeywordWhile}, {"are", TokenType::KeywordAre}, {"new", TokenType::KeywordNew}};

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
                i += 2; // Consume -->
                while (i < source.length())
                {
                    if (source[i] == '\n')
                        line++;
                    if (source[i] == '<' && i + 2 < source.length() && source[i + 1] == '-' && source[i + 2] == '-')
                    {
                        i += 2; // Consume <--
                        break;
                    }
                    i++;
                }
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
                            break; // Second dot?
                        // Check if it's a number dot or just a terminator dot
                        if (i + 1 >= source.length() || !std::isdigit(source[i + 1]))
                        {
                            break; // e.g. "1." -> Number(1), Dot
                        }
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
                    tokens.emplace_back(TokenType::Identifier, value, startPos, line);
                }
                continue;
            }

            // Symbols (Map + Lookahead)
            if (c == '-' && i + 1 < source.length() && std::isdigit(source[i + 1]))
            {
                // Negative number hard check? No, handled by Parser unary usually.
                // Treat as operator '-'
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
                std::string val(1, c);
                tokens.emplace_back(simpleSymbols.at(c), val, i, line);
                continue;
            }

            // Remaining Operators/Special cases
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
    std::vector<Token> rpn; // Compiled RPN
    // Short-circuit support: if logicalOp is set, this is a lazy node
    std::string logicalOp; // "&&" or "||" or empty
    std::shared_ptr<Expression> lhs;
    std::shared_ptr<Expression> rhs;
    virtual double evaluate(Scope &scope);
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

// Range loop: for i in range(from X to Y)
struct ForStmt : Statement
{
    std::string iteratorName;
    std::shared_ptr<Expression> startExpr;
    std::shared_ptr<Expression> endExpr;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override;
};

struct FunctionDefStmt : Statement
{
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    void execute(Scope &scope) override; // Register function
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
    bool isDeclaration = false; // New flag
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
        {
            a->execute(scope);
        }
    }
};

// --- Environment / Scope ---

struct ReturnException : public std::exception
{
    double value;
    ReturnException(double v) : value(v) {}
};

struct FunctionDef
{
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
};

class FunctionRegistry
{
public:
    static std::map<std::string, FunctionDef> functions;
};
std::map<std::string, FunctionDef> FunctionRegistry::functions;

struct Scope
{
    std::map<std::string, double> values;
    std::map<std::string, FunctionDef> functions;
    Scope *parent;
    bool barrier; // If true, 'set' cannot propagate to parent.

    Scope(Scope *p = nullptr, bool b = false) : parent(p), barrier(b) {}

    void define(const std::string &name, double val)
    {
        values[name] = val;
    }

    void defineFunction(const std::string &name, const FunctionDef &def)
    {
        functions[name] = def;
    }

    void set(const std::string &name, double val)
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

    double get(const std::string &name)
    {
        if (values.count(name))
        {
            return values[name];
        }
        if (parent)
        {
            return parent->get(name);
        }
        throw std::runtime_error("Undefined variable: " + name);
    }

    FunctionDef getFunction(const std::string &name)
    {
        if (functions.count(name))
            return functions[name];
        if (parent)
            return parent->getFunction(name);
        throw std::runtime_error("Unknown function: " + name);
    }
};

// --- Evaluator Implementation ---

double Expression::evaluate(Scope &scope)
{
    // Short-circuit evaluation for logical operators
    if (!logicalOp.empty() && lhs && rhs)
    {
        double leftVal = lhs->evaluate(scope);
        if (logicalOp == "&&")
        {
            if (std::abs(leftVal) < 1e-9)
                return 0.0; // LHS is false, skip RHS
            double rightVal = rhs->evaluate(scope);
            return (std::abs(rightVal) > 1e-9) ? 1.0 : 0.0;
        }
        else
        { // ||
            if (std::abs(leftVal) > 1e-9)
                return 1.0; // LHS is true, skip RHS
            double rightVal = rhs->evaluate(scope);
            return (std::abs(rightVal) > 1e-9) ? 1.0 : 0.0;
        }
    }

    std::stack<double> stack;

    for (const auto &token : rpn)
    {
        if (token.type == TokenType::Number)
        {
            stack.push(std::stod(token.value));
        }
        else if (token.type == TokenType::Identifier)
        {
            stack.push(scope.get(token.value));
        }
        else if (token.type == TokenType::Operator)
        {
            if (token.value == "~")
            { // Unary Minus
                if (stack.empty())
                    throw std::runtime_error("Stack underflow for unary '~'");
                double a = stack.top();
                stack.pop();
                stack.push(-a);
            }
            else if (token.value == "!")
            { // Unary Not
                if (stack.empty())
                    throw std::runtime_error("Stack underflow for unary '!'");
                double a = stack.top();
                stack.pop();
                stack.push(a == 0.0 ? 1.0 : 0.0);
            }
            else
            { // Binary operators
                if (stack.size() < 2)
                    throw std::runtime_error("Stack underflow for binary operator '" + token.value + "'");
                double b = stack.top();
                stack.pop();
                double a = stack.top();
                stack.pop();
                if (token.value == "+")
                    stack.push(a + b);
                else if (token.value == "-")
                    stack.push(a - b);
                else if (token.value == "*")
                    stack.push(a * b);
                else if (token.value == "/")
                {
                    if (std::abs(b) < 1e-9)
                        throw std::runtime_error("Div by 0");
                    stack.push(a / b);
                }
                else if (token.value == "%")
                {
                    if (std::abs(b) < 1e-9)
                        throw std::runtime_error("Mod by 0");
                    stack.push(std::fmod(a, b));
                }
                else if (token.value == "^")
                    stack.push(std::pow(a, b));
                else if (token.value == "<")
                    stack.push(a < b ? 1.0 : 0.0);
                else if (token.value == ">")
                    stack.push(a > b ? 1.0 : 0.0);
                else if (token.value == "<=")
                    stack.push(a <= b ? 1.0 : 0.0);
                else if (token.value == ">=")
                    stack.push(a >= b ? 1.0 : 0.0);
                else if (token.value == "==")
                    stack.push(std::abs(a - b) < 1e-9 ? 1.0 : 0.0);
                else if (token.value == "!=")
                    stack.push(std::abs(a - b) > 1e-9 ? 1.0 : 0.0);
                else if (token.value == "&&")
                    stack.push((std::abs(a) > 1e-9 && std::abs(b) > 1e-9) ? 1.0 : 0.0);
                else if (token.value == "||")
                    stack.push((std::abs(a) > 1e-9 || std::abs(b) > 1e-9) ? 1.0 : 0.0);
            }
        }
        // Function Calls (Built-in or User)
        // In RPN, we represent call as "CALL name argCount"?
        // Or simpler: Treat function name as operator.
        // But user functions have arbitrary args.
        // Current simple RPN might need enhancing for Function Calls.
        // Re-using 'Operator' type for function calls is tricky if arg count varies.
        // Let's assume we encoded Function Call as: [Args...] [Identifier_Func] [Operator_Call]
        // Actually, easiest is: Identifier token marked as Function puts it in a customized way?
        // Let's support standard math functions directly first.
        else if (token.type == TokenType::KeywordFn)
        { // Function call (built-in or user)
            // First check if it's a built-in math function
            if (is_math_function(token.value))
            {
                int argc = token.position;
                if (token.value == "min" || token.value == "max")
                {
                    if (stack.size() < 2)
                        throw std::runtime_error("Missing args for " + token.value);
                    double b = stack.top();
                    stack.pop();
                    double a = stack.top();
                    stack.pop();
                    if (token.value == "min")
                        stack.push(std::min(a, b));
                    else
                        stack.push(std::max(a, b));
                }
                else
                {
                    if (stack.empty())
                        throw std::runtime_error("Missing arg for " + token.value);
                    double arg = stack.top();
                    stack.pop();
                    if (token.value == "sin")
                        stack.push(std::sin(arg));
                    else if (token.value == "cos")
                        stack.push(std::cos(arg));
                    else if (token.value == "tan")
                        stack.push(std::tan(arg));
                    else if (token.value == "asin")
                        stack.push(std::asin(arg));
                    else if (token.value == "acos")
                        stack.push(std::acos(arg));
                    else if (token.value == "atan")
                        stack.push(std::atan(arg));
                    else if (token.value == "log")
                        stack.push(std::log(arg));
                    else if (token.value == "log2")
                        stack.push(std::log2(arg));
                    else if (token.value == "log10")
                        stack.push(std::log10(arg));
                    else if (token.value == "sqrt")
                        stack.push(std::sqrt(arg));
                    else if (token.value == "abs")
                        stack.push(std::abs(arg));
                    else if (token.value == "ceil")
                        stack.push(std::ceil(arg));
                    else if (token.value == "floor")
                        stack.push(std::floor(arg));
                    else if (token.value == "round")
                        stack.push(std::round(arg));
                    else if (token.value == "cot")
                        stack.push(1.0 / std::tan(arg));
                    else if (token.value == "sec")
                        stack.push(1.0 / std::cos(arg));
                    else if (token.value == "csc")
                        stack.push(1.0 / std::sin(arg));
                    else
                        stack.push(0);
                }
                continue;
            }
            // User-defined function call
            // Format: value=name, position=argCount
            std::string fname = token.value;
            int argc = token.position;

            try
            {
                FunctionDef def = scope.getFunction(fname);

                // Check Arity
                if (def.params.size() != argc)
                {
                    throw std::runtime_error("Function argument mismatch: expected " +
                                             std::to_string(def.params.size()) + " but got " + std::to_string(argc));
                }

                if (stack.size() < argc)
                    throw std::runtime_error("Stack underflow for args");

                // Find Global Root for scope parent (to emulate lexical scope at top level)
                // NOTE: With Nested Functions, we might want Closure?
                // User said "if yes -> inner can access outer".
                // Currently, we define function in current scope.
                // If we execute it, what is its parent?
                // If we just use `&scope` (the Caller's scope) -> Dynamic Scope.
                // If we want Lexical, we need to capture definition scope.
                // `FunctionDef` struct doesn't capture scope.
                // So we are forcing Dynamic Scope for now (Caller's variables are visible).
                // User said "Functions are first-class... Inner functions can access outer scope variables".
                // Dynamic linking satisfies visibility (if called from inside).
                // But if I return inner function and call it outside?
                // We don't support returning functions yet.
                // So Dynamic Scope is acceptable + Barrier.

                // Wait, Barrier prevents writing.
                // Ideally: Scope funcScope(&scope, true); (Caller as parent).
                // This allows reading Caller's vars.
                // This matches "inner function can access outer".

                Scope funcScope(&scope, true);

                std::vector<double> args;
                for (int i = 0; i < argc; ++i)
                {
                    args.push_back(stack.top());
                    stack.pop();
                }
                std::reverse(args.begin(), args.end());

                for (size_t i = 0; i < def.params.size(); ++i)
                {
                    funcScope.define(def.params[i], args[i]);
                }

                try
                {
                    def.body->execute(funcScope);
                }
                catch (ReturnException &ret)
                {
                    stack.push(ret.value);
                    continue;
                }
                stack.push(0); // Void/Implicit return
            }
            catch (const std::runtime_error &e)
            {
                // Check if it's the unknown function error or something else
                if (std::string(e.what()).find("Unknown function") != std::string::npos)
                {
                    throw std::runtime_error("Unknown function call: " + fname);
                }
                throw;
            }
        }
    }

    if (stack.empty())
        return 0;
    return stack.top();
}

void BlockStmt::execute(Scope &scope)
{
    // Create block scope to prevent leakage?
    // If we want Scoped blocks (like C++), yes.
    // If we want Python-like (function scope only), no.
    // Given "Scope Leakage" concerns, better to scope blocks.
    // Barrier = false.
    Scope blockScope(&scope, false);
    for (auto &stmt : statements)
    {
        stmt->execute(blockScope);
    }
}

void IfStmt::execute(Scope &scope)
{
    // std::cout << "Debug: Executing IfStmt with " << branches.size() << " branches." << std::endl;
    for (auto &branch : branches)
    {
        if (branch.condition->evaluate(scope) != 0.0)
        {
            // std::cout << "Debug: Branch condition met." << std::endl;
            branch.block->execute(scope);
            return;
        }
    }
    if (elseBlock)
    {
        // std::cout << "Debug: Else block." << std::endl;
        elseBlock->execute(scope);
    }
}

void ForStmt::execute(Scope &scope)
{
    double start = startExpr->evaluate(scope);
    double end = endExpr->evaluate(scope);

    // Create loop scope
    Scope loopScope(&scope);
    loopScope.define(iteratorName, start);

    // Inclusive range [start, end] ?
    // "from 10 to -20". Automatic step?
    double step = (end >= start) ? 1.0 : -1.0;

    // Python range does not include end, but user "from 1 to 5" implies 1,2,3,4,5 usually in natural language.
    // Let's assume Inclusive.
    double current = start;
    while ((step > 0 && current <= end) || (step < 0 && current >= end))
    {
        loopScope.set(iteratorName, current);
        body->execute(loopScope);
        current += step;
    }
}

void WhileStmt::execute(Scope &scope)
{
    while (static_cast<bool>(condition->evaluate(scope)))
    {
        // Loop scope (permeable)
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
    double val = expr->evaluate(scope);
    throw ReturnException(val);
}

void AssignStmt::execute(Scope &scope)
{
    double val = expr->evaluate(scope);
    if (isDeclaration)
    {
        scope.define(name, val);
    }
    else
    {
        scope.set(name, val);
    }
}

void ExprStmt::execute(Scope &scope)
{
    double val = expr->evaluate(scope);
    std::cout << val << std::endl;
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
        {
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
        if (match(TokenType::KeywordVar))
        {
            // Parse first var declaration
            Token name = consume(TokenType::Identifier, "Expected identifier after var");
            consume(TokenType::Equals, "Expected =");
            auto expr = parseExpression();
            auto firstAssign = std::make_shared<AssignStmt>();
            firstAssign->name = name.value;
            firstAssign->expr = expr;
            firstAssign->isDeclaration = true;

            // Check for comma-separated additional declarations
            if (check(TokenType::Comma))
            {
                auto multi = std::make_shared<MultiVarStmt>();
                multi->assignments.push_back(firstAssign);
                while (match(TokenType::Comma))
                {
                    Token nextName = consume(TokenType::Identifier, "Expected identifier after ,");
                    consume(TokenType::Equals, "Expected =");
                    auto nextExpr = parseExpression();
                    auto nextAssign = std::make_shared<AssignStmt>();
                    nextAssign->name = nextName.value;
                    nextAssign->expr = nextExpr;
                    nextAssign->isDeclaration = true;
                    multi->assignments.push_back(nextAssign);
                }
                consume(TokenType::Dot, "Expected . at end of statement");
                return multi;
            }

            consume(TokenType::Dot, "Expected . at end of statement");
            return firstAssign;
        }

        // Identifer starts assignment OR Call? "a = ..." vs "Call()"
        if (check(TokenType::Identifier))
        {
            if (peekNext().type == TokenType::Equals)
            {
                // Assignment
                Token name = advance();
                advance(); // =
                auto expr = parseExpression();
                consume(TokenType::Dot, "Expected . after assignment");
                auto assign = std::make_shared<AssignStmt>();
                assign->name = name.value;
                assign->expr = expr;
                return assign;
            }
        }

        // Expression Stmt
        auto expr = parseExpression();
        consume(TokenType::Dot, "Expected . after expression");
        auto stmt = std::make_shared<ExprStmt>();
        stmt->expr = expr;
        return stmt;
    }

    std::shared_ptr<IfStmt> parseIf()
    {
        auto stmt = std::make_shared<IfStmt>();

        // if cond :
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

    std::shared_ptr<ForStmt> parseFor()
    {
        // for i in range(from x to y):
        Token iter = consume(TokenType::Identifier, "Expected iterator name");
        consume(TokenType::KeywordIn, "Expected in");
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

    std::shared_ptr<PassStmt> parsePass()
    {
        consume(TokenType::Dot, "Expected . after pass");
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
        // fn name @ ( ... ) :
        // check "fn" was consumed by caller
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

        std::vector<TokenType> terminators = {TokenType::Semicolon};
        stmt->body = parseBlock(terminators);
        consume(TokenType::Semicolon, "Expected ; after function body");

        if (stmt->body->statements.empty())
        {
            throw std::runtime_error("Empty function body not allowed, use 'pass'.");
        }

        return stmt;
    }

    std::shared_ptr<ReturnStmt> parseReturn()
    {
        consume(TokenType::LeftParen, "Expected ( after give");
        auto expr = parseExpression();
        consume(TokenType::RightParen, "Expected ) after give expr");
        consume(TokenType::Dot, "Expected . after give");

        auto stmt = std::make_shared<ReturnStmt>();
        stmt->expr = expr;
        return stmt;
    }

    std::shared_ptr<BlockStmt> parseBlock(const std::vector<TokenType> &terminators)
    {
        auto block = std::make_shared<BlockStmt>();
        while (!isAtEnd())
        {
            // Check terminators
            for (auto t : terminators)
                if (check(t))
                    return block;

            block->statements.push_back(parseStatement());
        }
        return block;
    }

    // Top-level expression parser: handles short-circuit && and ||
    // Builds a tree of Expression nodes for lazy evaluation
    std::shared_ptr<Expression> parseExpression()
    {
        return parseLogicalOr();
    }

    // Parse || (lowest logical precedence) with short-circuit
    std::shared_ptr<Expression> parseLogicalOr()
    {
        auto left = parseLogicalAnd();
        while (!isAtEnd() && peek().type == TokenType::Operator && peek().value == "||")
        {
            advance(); // consume ||
            auto right = parseLogicalAnd();
            auto node = std::make_shared<Expression>();
            node->logicalOp = "||";
            node->lhs = left;
            node->rhs = right;
            left = node;
        }
        return left;
    }

    // Parse && with short-circuit
    std::shared_ptr<Expression> parseLogicalAnd()
    {
        auto left = parsePrimaryExpr();
        while (!isAtEnd() && peek().type == TokenType::Operator && peek().value == "&&")
        {
            advance(); // consume &&
            auto right = parsePrimaryExpr();
            auto node = std::make_shared<Expression>();
            node->logicalOp = "&&";
            node->lhs = left;
            node->rhs = right;
            left = node;
        }
        return left;
    }

    // Shunting-Yard for everything EXCEPT && and || (which are handled above)
    std::shared_ptr<Expression> parsePrimaryExpr()
    {
        auto expr = std::make_shared<Expression>();
        std::queue<Token> out;
        std::stack<Token> stack;

        TokenType lastTokenType = TokenType::Eof;

        while (!isAtEnd())
        {
            Token t = peek();

            // End of expression check
            if (t.type == TokenType::Dot || t.type == TokenType::Colon || t.type == TokenType::Semicolon ||
                t.type == TokenType::KeywordIn || t.type == TokenType::KeywordTo ||
                t.type == TokenType::KeywordElif || t.type == TokenType::KeywordElse ||
                t.type == TokenType::Equals)
            {
                break;
            }
            // Stop at && and || at the top level (not inside parens)
            // so the logical layer above can handle them with short-circuit
            if (t.type == TokenType::Operator && (t.value == "&&" || t.value == "||"))
            {
                // Only break if we're NOT inside parentheses in this expression
                bool hasParen = false;
                std::stack<Token> temp = stack;
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
                std::stack<Token> temp = stack;
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
                {
                    break;
                }
            }

            Token token = advance();

            // Implicit multiplication: Number/RightParen followed by Number/Identifier/LeftParen
            if ((token.type == TokenType::Number || token.type == TokenType::Identifier || token.type == TokenType::LeftParen) &&
                (lastTokenType == TokenType::Number || lastTokenType == TokenType::RightParen))
            {
                // Insert implicit * operator via Shunting-Yard
                Token mulOp(TokenType::Operator, "*", token.position, token.line);
                int currPrec = get_operator_precedence("*");
                while (!stack.empty() && stack.top().type == TokenType::Operator &&
                       get_operator_precedence(stack.top().value) >= currPrec)
                {
                    out.push(stack.top());
                    stack.pop();
                }
                stack.push(mulOp);
            }

            if (token.type == TokenType::Number)
            {
                out.push(token);
            }
            else if (token.type == TokenType::Identifier)
            {
                // Check if Function Call: Identifier ( ...
                if (check(TokenType::LeftParen))
                {
                    consume(TokenType::LeftParen, "(");
                    int argCount = 0;
                    if (!check(TokenType::RightParen))
                    {
                        do
                        {
                            // Parse Sub-Expression for Arg (full expression with logical)
                            auto argExpr = parseExpression();
                            // Flatten: if it's a simple RPN expr, inline it
                            if (argExpr->logicalOp.empty())
                            {
                                for (auto &at : argExpr->rpn)
                                    out.push(at);
                            }
                            else
                            {
                                // Complex logical expr as arg — store as sub-expression call
                                // We'll push a special token that holds the expression
                                Token lazyToken(TokenType::Number, "0", -1, -1);
                                lazyToken.value = "__lazy";
                                lazyToken.type = TokenType::Number; // will be replaced at eval
                                // Can't easily embed in RPN — evaluate eagerly for func args
                                // (short-circuit matters most for conditions, not func args)
                                // Fallback: evaluate now isn't possible without scope.
                                // Simpler: just flatten both sides with && in RPN for func args
                                // Actually, the cleanest solution: recursively emit RPN
                                flattenExprToQueue(argExpr, out);
                            }
                            argCount++;
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RightParen, ")");

                    out.push(Token(TokenType::KeywordFn, token.value, argCount, token.line));
                    token.type = TokenType::Identifier;
                }
                else
                {
                    out.push(token);
                }
            }
            else if (token.type == TokenType::Operator)
            {
                bool isUnary = false;
                if (token.value == "-" || token.value == "!")
                {
                    if (lastTokenType == TokenType::Eof ||
                        lastTokenType == TokenType::LeftParen ||
                        lastTokenType == TokenType::Comma ||
                        lastTokenType == TokenType::Operator ||
                        lastTokenType == TokenType::Equals ||
                        lastTokenType == TokenType::Colon ||
                        lastTokenType == TokenType::KeywordIf ||
                        lastTokenType == TokenType::KeywordElif ||
                        lastTokenType == TokenType::KeywordGive)
                    {
                        isUnary = true;
                    }
                }

                if (isUnary)
                {
                    std::string op = (token.value == "-") ? "~" : token.value;
                    stack.push(Token(TokenType::Operator, op, token.position, token.line));
                }
                else
                {
                    int currPrec = get_operator_precedence(token.value);
                    while (!stack.empty() && stack.top().type == TokenType::Operator &&
                           get_operator_precedence(stack.top().value) >= currPrec)
                    {
                        out.push(stack.top());
                        stack.pop();
                    }
                    stack.push(token);
                }
            }
            else if (token.type == TokenType::LeftParen)
            {
                stack.push(token);
            }
            else if (token.type == TokenType::RightParen)
            {
                while (!stack.empty() && stack.top().type != TokenType::LeftParen)
                {
                    out.push(stack.top());
                    stack.pop();
                }
                if (!stack.empty())
                    stack.pop(); // Pop (
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

        while (!stack.empty())
        {
            if (stack.top().type == TokenType::LeftParen)
                throw std::runtime_error("Mismatched parens at end");
            out.push(stack.top());
            stack.pop();
        }

        while (!out.empty())
        {
            expr->rpn.push_back(out.front());
            out.pop();
        }
        return expr;
    }

    // Helper: flatten a logical expression tree into RPN queue (for non-short-circuit contexts like func args)
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
    Token peek() { return pos < tokens.size() ? tokens[pos] : tokens.back(); }
    Token peekNext() { return pos + 1 < tokens.size() ? tokens[pos + 1] : tokens.back(); }
    bool isAtEnd() { return peek().type == TokenType::Eof; }
    Token advance()
    {
        if (!isAtEnd())
            pos++;
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
    Token consume(TokenType t, std::string err)
    {
        // std::cout << "Debug: consume expected " << (int)t << ", got " << (int)peek().type << std::endl;
        if (check(t))
            return advance();
        // std::cout << "Debug: Failed consume. Current token: " << peek().value << std::endl;
        throw std::runtime_error(err + " at line " + std::to_string(peek().line));
    }
};

// --- Main & Tests ---

void run_tests()
{
    std::cout << "Running Tests..." << std::endl;

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

        std::cout << "--- Tokens ---" << std::endl;
        for (const auto &t : tokens)
        {
            std::cout << "Line " << t.line << ": " << t.value << " (" << (int)t.type << ")" << std::endl;
        }
        std::cout << "--- End Tokens ---" << std::endl;

        Parser parser(tokens);
        auto program = parser.parseProgram();

        Scope global;
        global.define("PI", 3.14159);
        program->execute(global);

        std::cout << "Result: " << global.get("result") << " (Expected 31)" << std::endl;
        std::cout << "LoopSum: " << global.get("loopSum") << " (Expected 15)" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "Test Failed: " << e.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        run_tests();
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--script")
    {
        // Read entire stdin
        std::string content((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        if (content.empty())
            return 0;

        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(content);

            // Debug Tokens
            // std::cout << "--- Tokens ---" << std::endl;
            // for (const auto& t : tokens) {
            //      std::cout << "Line " << t.line << ": " << t.value << " (" << (int)t.type << ")" << std::endl;
            // }
            // std::cout << "--- End Tokens ---" << std::endl;

            Parser parser(tokens);
            auto program = parser.parseProgram();

            Scope globalScope;
            globalScope.define("PI", 3.14159265);
            globalScope.define("e", 2.7182818);

            // Execute statements directly in globalScope (no child scope)
            for (auto &stmt : program->statements)
            {
                stmt->execute(globalScope);
            }
        }
        catch (ReturnException &e)
        {
            std::cout << e.value << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
        return 0;
    }

    std::cout << "Advanced Interpreter REPL" << std::endl;
    std::string line;
    Scope globalScope;
    globalScope.define("PI", 3.14159265);
    globalScope.define("e", 2.7182818);
    globalScope.define("ans", 0);

    while (true)
    {
        std::cout << ">> ";
        if (!std::getline(std::cin, line) || line == "exit")
            break;
        else if (line == "clear") {
            #ifdef _WIN32
            std::system("cls");   // Windows
        #else
            std::system("clear"); // Linux / macOS
        #endif
        }
        else if (line.empty())
            continue;
        else if( line == "wipe" )
        {
            
        }
        try
        {
            Tokenizer tokenizer;
            auto tokens = tokenizer.tokenize(line);
            Parser parser(tokens);
            auto program = parser.parseProgram();
            // Execute statements directly in globalScope (no child scope)
            // so that variables persist across REPL lines
            for (auto &stmt : program->statements)
            {
                stmt->execute(globalScope);
            }
        }
        catch (ReturnException &e)
        {
            std::cout << e.value << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}