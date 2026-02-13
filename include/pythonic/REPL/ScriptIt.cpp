// ScriptIt v2 — A scripting language powered by pythonic::vars::var
// Extension: .sit | Run: scriptit <file.sit> | REPL: scriptit

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
                    value += source[i++];
                i--;
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
                return var(0);
            return var(static_cast<bool>(rhs->evaluate(scope)) ? 1 : 0);
        }
        else
        {
            if (static_cast<bool>(leftVal))
                return var(1);
            return var(static_cast<bool>(rhs->evaluate(scope)) ? 1 : 0);
        }
    }

    std::stack<var> stk;

    for (const auto &token : rpn)
    {
        if (token.type == TokenType::Number)
        {
            if (token.value.find('.') != std::string::npos)
                stk.push(var(std::stod(token.value)));
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
                     { return pythonic::math::pow(a, b, Overflow::Promote); }},
                    {"==", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() == b.as_string_unchecked() ? 1 : 0);
                         if (a.is_none() || b.is_none())
                             return var((a.is_none() && b.is_none()) ? 1 : 0);
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var((a == b) ? 1 : 0);
                         double ad = var_to_double(a), bd = var_to_double(b);
                         return var(std::abs(ad - bd) < 1e-9 ? 1 : 0);
                     }},
                    {"!=", [](const var &a, const var &b) -> var
                     {
                         if (a.is_string() && b.is_string())
                             return var(a.as_string_unchecked() != b.as_string_unchecked() ? 1 : 0);
                         if (a.is_none() || b.is_none())
                             return var((a.is_none() && b.is_none()) ? 0 : 1);
                         if (a.is_list() || b.is_list() || a.is_set() || b.is_set() ||
                             a.is_dict() || b.is_dict())
                             return var((a != b) ? 1 : 0);
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
                     { return var((static_cast<bool>(a) || static_cast<bool>(b)) ? 1 : 0); }},
                };

                auto opIt = binaryOps.find(token.value);
                if (opIt != binaryOps.end())
                    stk.push(opIt->second(a, b));
                else
                    throw std::runtime_error("Unknown binary operator: " + token.value);
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
                    throw std::runtime_error("Stack underflow for list literal");
                temp.push_back(stk.top());
                stk.pop();
            }
            std::reverse(temp.begin(), temp.end());
            List items(temp.begin(), temp.end());
            stk.push(var(std::move(items)));
        }
        // Set literal
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
        // ── Method call via dtype dispatch ──
        else if (token.type == TokenType::At)
        {
            std::string method = token.value;
            int argc = token.position;

            std::vector<var> args;
            for (int i = 0; i < argc; ++i)
            {
                if (stk.empty())
                    throw std::runtime_error("Stack underflow for method args");
                args.push_back(stk.top());
                stk.pop();
            }
            std::reverse(args.begin(), args.end());

            if (stk.empty())
                throw std::runtime_error("Stack underflow for method call (no object)");
            var self = stk.top();
            stk.pop();

            stk.push(dispatch_method(self, method, args));
        }
        // ── Function calls ──
        else if (token.type == TokenType::KeywordFn)
        {
            std::string fname = token.value;
            int argc = token.position;

            if (is_math_function(fname))
            {
                stk.push(dispatch_math(fname, stk));
                continue;
            }

            const auto &builtins = get_builtins();
            auto builtinIt = builtins.find(fname);
            if (builtinIt != builtins.end())
            {
                builtinIt->second(stk, argc);
                continue;
            }

            // User-defined function call
            try
            {
                FunctionDef def = scope.getFunction(fname);
                if ((int)def.params.size() != argc)
                    throw std::runtime_error("Function argument mismatch: expected " +
                                             std::to_string(def.params.size()) + " but got " + std::to_string(argc));
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
                stk.push(var(0));
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

// ═══════════════════════════════════════════════════════════
// ──── Statement Implementations ────────────────────────────
// ═══════════════════════════════════════════════════════════

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
    FunctionDef def;
    def.name = name;
    def.params = params;
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

// ═══════════════════════════════════════════════════════════
// ──── Parser ───────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

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
                         peek().value != "True" && peek().value != "False" && peek().value != "None" &&
                         (peekNext().type == TokenType::Equals ||
                          peekNext().type == TokenType::Dot ||
                          peekNext().type == TokenType::Comma ||
                          peekNext().type == TokenType::Identifier ||
                          peekNext().type == TokenType::Eof))
                    multi->assignments.push_back(parseOneVar());
                else
                    break;
            }
            consumeDotOrForgive();

            if (multi->assignments.size() == 1)
                return multi->assignments[0];
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

    // ── Expression Parsing ──

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
                    // Only treat as method call if the dot and identifier are adjacent
                    // (no space between them). e.g. "x.upper()" has dot at pos N, ident at N+1.
                    // But "x. print(y)" has a space, so the dot is a statement terminator.
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
                t.type == TokenType::KeywordElif || t.type == TokenType::KeywordElse ||
                t.type == TokenType::KeywordBe || t.type == TokenType::Equals)
                break;

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

            Token token = advance();

            // Implicit multiplication
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
                int count = 0;
                if (!check(TokenType::RightBrace))
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
    Token consume(TokenType t, std::string err)
    {
        if (check(t))
            return advance();
        throw std::runtime_error(err + " at line " + std::to_string(peek().line));
    }
    Token consumeDotOrForgive()
    {
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

// ═══════════════════════════════════════════════════════════
// ──── Execute Script Helper ────────────────────────────────
// ═══════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════
// ──── Minimal JSON Helpers (for kernel mode) ───────────────
// ═══════════════════════════════════════════════════════════

// Minimal JSON value: string→string map parser (for kernel protocol)
inline std::unordered_map<std::string, std::string> parse_json_object(const std::string &json)
{
    std::unordered_map<std::string, std::string> result;
    size_t i = json.find('{');
    if (i == std::string::npos)
        return result;
    i++;

    auto skipWS = [&]()
    { while (i < json.size() && std::isspace(json[i])) i++; };
    auto readString = [&]() -> std::string
    {
        skipWS();
        if (i >= json.size() || json[i] != '"')
            return "";
        i++; // skip opening "
        std::string s;
        while (i < json.size() && json[i] != '"')
        {
            if (json[i] == '\\' && i + 1 < json.size())
            {
                i++;
                if (json[i] == 'n')
                    s += '\n';
                else if (json[i] == 't')
                    s += '\t';
                else if (json[i] == '\\')
                    s += '\\';
                else if (json[i] == '"')
                    s += '"';
                else if (json[i] == '/')
                    s += '/';
                else
                    s += json[i];
            }
            else
                s += json[i];
            i++;
        }
        if (i < json.size())
            i++; // skip closing "
        return s;
    };

    while (i < json.size())
    {
        skipWS();
        if (json[i] == '}')
            break;
        if (json[i] == ',')
        {
            i++;
            continue;
        }
        std::string key = readString();
        skipWS();
        if (i < json.size() && json[i] == ':')
            i++;
        skipWS();
        // Value can be string, number, or null
        if (i < json.size() && json[i] == '"')
        {
            result[key] = readString();
        }
        else
        {
            // Read non-string value as raw text until , or }
            std::string val;
            while (i < json.size() && json[i] != ',' && json[i] != '}')
                val += json[i++];
            // trim
            while (!val.empty() && std::isspace(val.back()))
                val.pop_back();
            result[key] = val;
        }
    }
    return result;
}

inline std::string json_escape(const std::string &s)
{
    std::string out;
    out.reserve(s.size() + 10);
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

inline std::string make_json_response(const std::string &cellId, const std::string &status,
                                      const std::string &stdoutStr, const std::string &stderrStr,
                                      const std::string &result, int execCount)
{
    return "{\"cell_id\":\"" + json_escape(cellId) + "\","
                                                     "\"status\":\"" +
           json_escape(status) + "\","
                                 "\"stdout\":\"" +
           json_escape(stdoutStr) + "\","
                                    "\"stderr\":\"" +
           json_escape(stderrStr) + "\","
                                    "\"result\":\"" +
           json_escape(result) + "\","
                                 "\"execution_count\":" +
           std::to_string(execCount) + "}";
}

// ═══════════════════════════════════════════════════════════
// ──── Kernel Mode ──────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

void runKernel()
{
    Scope globalScope;
    globalScope.define("PI", var(3.14159265));
    globalScope.define("e", var(2.7182818));
    int executionCount = 0;

    // Signal ready
    std::cout << "{\"status\":\"kernel_ready\",\"version\":\"2.0\"}" << std::endl;
    std::cout.flush();

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty())
            continue;

        auto cmd = parse_json_object(line);
        std::string action = cmd["action"];

        if (action == "shutdown")
        {
            std::cout << "{\"status\":\"shutdown_ok\"}" << std::endl;
            std::cout.flush();
            break;
        }

        if (action == "reset")
        {
            globalScope.clear();
            globalScope.define("PI", var(3.14159265));
            globalScope.define("e", var(2.7182818));
            executionCount = 0;
            std::cout << "{\"status\":\"reset_ok\"}" << std::endl;
            std::cout.flush();
            continue;
        }

        if (action == "execute")
        {
            std::string cellId = cmd["cell_id"];
            std::string code = cmd["code"];
            executionCount++;

            // Capture stdout
            std::ostringstream capturedOut;
            std::streambuf *oldBuf = std::cout.rdbuf(capturedOut.rdbuf());

            std::string errorStr;
            std::string resultStr;

            try
            {
                Tokenizer tokenizer;
                auto tokens = tokenizer.tokenize(code);
                Parser parser(tokens);
                auto program = parser.parseProgram();

                for (auto &stmt : program->statements)
                    stmt->execute(globalScope);
            }
            catch (ReturnException &e)
            {
                resultStr = format_output(e.value);
            }
            catch (std::exception &e)
            {
                errorStr = e.what();
            }

            // Restore stdout
            std::cout.rdbuf(oldBuf);
            std::string stdoutStr = capturedOut.str();

            std::string status = errorStr.empty() ? "ok" : "error";
            std::cout << make_json_response(cellId, status, stdoutStr, errorStr, resultStr, executionCount) << std::endl;
            std::cout.flush();
            continue;
        }

        if (action == "complete")
        {
            std::string code = cmd["code"];
            // Simple completion: list scope variables and builtins that match prefix
            std::vector<std::string> matches;
            for (auto &[name, _] : globalScope.getAll())
            {
                if (name.find(code) == 0)
                    matches.push_back(name);
            }
            // Return as JSON array
            std::string out = "{\"status\":\"ok\",\"completions\":[";
            for (size_t i = 0; i < matches.size(); i++)
            {
                if (i > 0)
                    out += ",";
                out += "\"" + json_escape(matches[i]) + "\"";
            }
            out += "]}";
            std::cout << out << std::endl;
            std::cout.flush();
            continue;
        }

        // Unknown action
        std::cout << "{\"status\":\"error\",\"stderr\":\"Unknown action: " + json_escape(action) + "\"}" << std::endl;
        std::cout.flush();
    }
}

// ═══════════════════════════════════════════════════════════
// ──── Main ─────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--kernel")
    {
        runKernel();
        return 0;
    }

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

    if (argc > 1 && std::string(argv[1]) == "--script")
    {
        std::string content((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        executeScript(content);
        return 0;
    }

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

    // ── Interactive REPL ──
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
