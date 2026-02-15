
#pragma once

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
#include <cassert>
#include <iomanip>
#include <functional>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <limits>
#include "../pythonicPrint.hpp"

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

// namespace pythonic {
//     namespace print {
//         enum class Mode;
//         struct TextConfig;
//         enum class PrintType;
//         void print(PrintType, const std::string&, const TextConfig&);
//     }
// }

namespace pythonic
{
    namespace calculator
    {

        enum class TokenType : uint8_t
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
            Equals,
            Comma,
            Dot,
            Unknown
        };

        struct Token
        {
            TokenType type;
            std::string value;
            int position;

            Token(TokenType t, std::string v, int pos = -1)
                : type(t), value(v), position(pos) {}
            Token()
                : type(TokenType::Unknown), value(""), position(-1)
            {
            }
        };

        inline void print_title()
        {
            pythonic::print::print(pythonic::print::TextArt, "   ============= CALCULATOR APP =============", pythonic::print::TextConfig{.mode = pythonic::print::Mode::colored, .fg_r = 0, .fg_g = 255, .fg_b = 255});
            ;
        }

        inline std::unordered_map<std::string, std::pair<Token, long double>> reserved_constant;

        static void set_constants()
        {
            // Initialize defaults only once so user-updated values (e.g. ans) persist.
            if (!reserved_constant.empty())
                return;

            reserved_constant["PI"] = std::make_pair(Token(TokenType::Identifier, "PI"), 3.14159265358979323846264338327950288419716939937510L);
            reserved_constant["e"] = std::make_pair(Token(TokenType::Identifier, "e"), 2.71828182845904523536028747135266249775724709369995L);
            reserved_constant["tau"] = std::make_pair(Token(TokenType::Identifier, "tau"), 6.28318530717958647692528676655900576839433879875021L); // 2*PI
            reserved_constant["inf"] = std::make_pair(Token(TokenType::Identifier, "inf"), std::numeric_limits<long double>::infinity());
            reserved_constant["infinity"] = std::make_pair(Token(TokenType::Identifier, "infinity"), std::numeric_limits<long double>::infinity());
            reserved_constant["nan"] = std::make_pair(Token(TokenType::Identifier, "nan"), std::numeric_limits<long double>::quiet_NaN());
            reserved_constant["ans"] = std::make_pair(Token(TokenType::Identifier, "ans"), 0.0L);                                                 // Placeholder, update after each calculation
            reserved_constant["deg"] = std::make_pair(Token(TokenType::Identifier, "deg"), 57.295779513082320876798154814105170332405472466564L); // 180/PI
        }
        // Precedence:
        // 1: + -
        // 2: * /
        // 3: ^
        // 4: Unary Minus (~)
        // 5: Functions

        inline int get_operator_precedence(const std::string &op)
        {
            if (op == "+" || op == "-")
                return 1;
            if (op == "*" || op == "/")
                return 2;
            if (op == "^")
                return 3;
            if (op == "~") // Unary minus
                return 4;
            return 0;
        }

        inline long double auto_promote(long double x, long double EPS = 1e-12L)
        {
            long double rounded = std::round(x);
            if (std::abs(x - rounded) < EPS)
                return rounded;
            return x;
        }
        inline bool is_right_associative(const std::string &op)
        {
            return op == "^" || op == "~";
        }

        inline bool is_math_function(const std::string &str)
        {
            static const std::vector<std::string> funcs = {
                "sin", "cos", "tan", "cot", "sec", "csc",
                "asin", "acos", "atan", "acot", "asec", "acsc",
                "log", "log2", "log10", "sqrt", "abs"};
            return std::find(funcs.begin(), funcs.end(), str) != funcs.end();
        }

        static bool is_constant(const std::string &str)
        {
            static const std::vector<std::string> constants = {
                "ans", "e", "PI", "deg", "tau", "inf", "infinity", "nan"};
            return std::find(constants.begin(), constants.end(), str) != constants.end();
        }

        static bool DEBUG = false;
        class Tokenizer
        {
        public:
            std::vector<Token> tokenize(const std::string &expression)
            {
                std::vector<Token> tokens;
                std::string expr = expression;

                for (size_t i = 0; i < expr.length(); ++i)
                {
                    char c = expr[i];

                    // NOTE: If later i want to support space aware perser  i will need to tweak this part
                    if (std::isspace(c))
                        continue;

                    if (std::isdigit(c) || c == '.')
                    {
                        // If it is a dot, it COULD be a number (.5) or a delimiter (.).
                        // Check if it is followed by digit -> Number.
                        // If not followed by digit -> Delimiter (dot token).
                        if (c == '.')
                        {
                            bool nextIsDigit = (i + 1 < expr.length() && std::isdigit(expr[i + 1]));
                            if (!nextIsDigit)
                            {
                                tokens.emplace_back(TokenType::Dot, ".", i);
                                continue;
                            }
                        }

                        // for cases like (2+3)4 --> (2+3)*4
                        if (!tokens.empty())
                        {
                            TokenType lastType = tokens.back().type;
                            if (lastType == TokenType::RightParen || lastType == TokenType::RightBrace || lastType == TokenType::RightBracket)
                            {
                                tokens.emplace_back(TokenType::Operator, "*", i);
                            }
                        }

                        std::string numStr;
                        bool hasDecimal = false;
                        int startPos = i;

                        while (i < expr.length() && (std::isdigit(expr[i]) || expr[i] == '.'))
                        {
                            if (expr[i] == '.')
                            {
                                // Check if dot is part of number (followed by digit)
                                bool nextIsDigit = (i + 1 < expr.length() && std::isdigit(expr[i + 1]));
                                if (!nextIsDigit)
                                {
                                    // Dot is a delimiter, not decimal point. Stop number parsing.
                                    break;
                                }

                                if (hasDecimal)
                                {
                                    break;
                                }
                                hasDecimal = true;
                            }
                            numStr += expr[i];
                            i++;
                        }
                        i--; // Backtrack
                        tokens.emplace_back(TokenType::Number, numStr, startPos);
                    }
                    // Identifiers (Variables, Functions, 'var')
                    else if (std::isalpha(c) || c == '_')
                    {
                        // Implicit multiplication check for identifiers
                        if (!tokens.empty())
                        {
                            TokenType lastType = tokens.back().type;
                            if (lastType == TokenType::Number || lastType == TokenType::RightParen || lastType == TokenType::RightBrace || lastType == TokenType::RightBracket)
                            {
                                tokens.emplace_back(TokenType::Operator, "*", i);
                            }
                        }

                        std::string idStr;
                        int startPos = i;
                        while (i < expr.length() && (std::isalnum(expr[i]) || expr[i] == '_'))
                        {
                            idStr += expr[i];
                            i++;
                        }
                        i--; // Backtrack

                        // TODO: Support 'var -x = ...' syntax (allow unary minus after 'var' keyword for variable names)
                        if (idStr == "var")
                        {
                            tokens.emplace_back(TokenType::KeywordVar, idStr, startPos);
                        }
                        else
                        {
                            tokens.emplace_back(TokenType::Identifier, idStr, startPos);
                        }
                    }
                    // Operators and Punctuation
                    else
                    {
                        std::string opStr(1, c);
                        int pos = i;

                        // Implicit mult for brackets
                        if (c == '(' || c == '{' || c == '[')
                        {
                            if (!tokens.empty())
                            {
                                TokenType lastType = tokens.back().type;
                                bool lastIsFunc = (lastType == TokenType::Identifier && is_math_function(tokens.back().value));

                                if (lastType == TokenType::Number ||
                                    lastType == TokenType::RightParen ||
                                    lastType == TokenType::RightBrace ||
                                    lastType == TokenType::RightBracket ||
                                    (lastType == TokenType::Identifier && !lastIsFunc))
                                {
                                    tokens.emplace_back(TokenType::Operator, "*", pos);
                                }
                            }
                        }

                        switch (c)
                        {
                        case '+':
                            tokens.emplace_back(TokenType::Operator, "+", pos);
                            break;
                        case '-':
                            if (tokens.empty() ||
                                tokens.back().type == TokenType::Operator ||
                                tokens.back().type == TokenType::LeftParen ||
                                tokens.back().type == TokenType::LeftBrace ||
                                tokens.back().type == TokenType::LeftBracket ||
                                tokens.back().type == TokenType::Equals ||
                                tokens.back().type == TokenType::Comma ||
                                tokens.back().type == TokenType::Dot ||
                                tokens.back().type == TokenType::KeywordVar)
                            {
                                tokens.emplace_back(TokenType::Operator, "~", pos); // Unary minus
                            }
                            else
                            {
                                tokens.emplace_back(TokenType::Operator, "-", pos);
                            }
                            break;
                        case '*':
                            tokens.emplace_back(TokenType::Operator, "*", pos);
                            break;
                        case '/':
                            tokens.emplace_back(TokenType::Operator, "/", pos);
                            break;
                        case '^':
                            tokens.emplace_back(TokenType::Operator, "^", pos);
                            break;
                        case '=':
                            tokens.emplace_back(TokenType::Equals, "=", pos);
                            break;
                        case ',':
                            tokens.emplace_back(TokenType::Comma, ",", pos);
                            break;
                        case '.':
                            tokens.emplace_back(TokenType::Dot, ".", pos);
                            break; // Explicit dot delimiter
                        case '(':
                            tokens.emplace_back(TokenType::LeftParen, "(", pos);
                            break;
                        case ')':
                            tokens.emplace_back(TokenType::RightParen, ")", pos);
                            break;
                        case '{':
                            tokens.emplace_back(TokenType::LeftBrace, "{", pos);
                            break;
                        case '}':
                            tokens.emplace_back(TokenType::RightBrace, "}", pos);
                            break;
                        case '[':
                            tokens.emplace_back(TokenType::LeftBracket, "[", pos);
                            break;
                        case ']':
                            tokens.emplace_back(TokenType::RightBracket, "]", pos);
                            break;
                        default:
                            throw std::runtime_error("Unknown character '" + std::string(1, c) + "' at position " + std::to_string(pos));
                        }
                    }
                }
                return tokens;
            }
        };

        class Parser // Uses Shunting Yard algorithm
        {
        public:
            std::queue<Token> parse(const std::vector<Token> &tokens)
            {
                std::queue<Token> outputQueue;
                std::stack<Token> operatorStack;
                std::stack<char> bracketStack;

                for (const auto &token : tokens)
                {
                    if (token.type == TokenType::Number)
                    {
                        outputQueue.push(token);
                    }
                    else if (token.type == TokenType::Identifier)
                    {
                        if (is_math_function(token.value))
                        {
                            operatorStack.push(token);
                        }
                        else
                        {
                            outputQueue.push(token);
                        }
                    }
                    else if (token.type == TokenType::Operator)
                    {
                        int currPrec = get_operator_precedence(token.value);
                        bool rightAssoc = is_right_associative(token.value);

                        while (!operatorStack.empty() &&
                               operatorStack.top().type != TokenType::LeftParen &&
                               operatorStack.top().type != TokenType::LeftBrace &&
                               operatorStack.top().type != TokenType::LeftBracket)
                        {

                            Token top = operatorStack.top();
                            int topPrec = 0;
                            if (top.type == TokenType::Identifier && is_math_function(top.value))
                            {
                                topPrec = 5; // Function precedence
                            }
                            else if (top.type == TokenType::Operator)
                            {
                                topPrec = get_operator_precedence(top.value);
                            }

                            if (topPrec > currPrec || (topPrec == currPrec && !rightAssoc))
                            {
                                outputQueue.push(top);
                                operatorStack.pop();
                            }
                            else
                            {
                                break;
                            }
                        }
                        operatorStack.push(token);
                    }
                    else if (token.type == TokenType::LeftParen)
                    {
                        operatorStack.push(token);
                        bracketStack.push('(');
                    }
                    else if (token.type == TokenType::LeftBrace)
                    {
                        operatorStack.push(token);
                        bracketStack.push('{');
                    }
                    else if (token.type == TokenType::LeftBracket)
                    {
                        operatorStack.push(token);
                        bracketStack.push('[');
                    }
                    else if (token.type == TokenType::RightParen)
                    {
                        processClosingBracket(outputQueue, operatorStack, bracketStack, '(', token);
                    }
                    else if (token.type == TokenType::RightBrace)
                    {
                        processClosingBracket(outputQueue, operatorStack, bracketStack, '{', token);
                    }
                    else if (token.type == TokenType::RightBracket)
                    {
                        processClosingBracket(outputQueue, operatorStack, bracketStack, '[', token);
                    }
                    else
                    {
                        throw std::runtime_error("Unexpected token '" + token.value + "' at position " + std::to_string(token.position));
                    }
                }

                while (!operatorStack.empty())
                {
                    Token top = operatorStack.top();
                    if (top.type == TokenType::LeftParen || top.type == TokenType::LeftBrace || top.type == TokenType::LeftBracket)
                    {
                        throw std::runtime_error("Mismatched or unclosed brackets at end of expression");
                    }
                    outputQueue.push(top);
                    operatorStack.pop();
                }

                if (!bracketStack.empty())
                {
                    throw std::runtime_error("Mismatched or unclosed brackets found.");
                }

                return outputQueue;
            }

        private:
            void processClosingBracket(std::queue<Token> &outputQueue, std::stack<Token> &operatorStack, std::stack<char> &bracketStack, char expectedLeft, const Token &token)
            {
                bool foundLeft = false;

                if (bracketStack.empty())
                {
                    throw std::runtime_error("Unmatched closing bracket '" + token.value + "' at position " + std::to_string(token.position));
                }

                if (bracketStack.top() != expectedLeft)
                {
                    std::string msg = "Mismatched brackets: Expected closing for '";
                    msg += bracketStack.top();
                    msg += "' but found '" + token.value + "' at position " + std::to_string(token.position);
                    throw std::runtime_error(msg);
                }

                while (!operatorStack.empty())
                {
                    Token top = operatorStack.top();
                    if (top.type == TokenType::LeftParen || top.type == TokenType::LeftBrace || top.type == TokenType::LeftBracket)
                    {
                        if (getOpenChar(top.type) == expectedLeft)
                        {
                            foundLeft = true;
                            operatorStack.pop();
                            bracketStack.pop();
                            break;
                        }
                        else
                        {
                            throw std::runtime_error("Internal error: bracket sync");
                        }
                    }
                    outputQueue.push(top);
                    operatorStack.pop();
                }

                if (!foundLeft)
                {
                    throw std::runtime_error("Mismatched parentheses/brackets at position: " + std::to_string(operatorStack.top().position));
                }

                if (!operatorStack.empty() && operatorStack.top().type == TokenType::Identifier && is_math_function(operatorStack.top().value))
                {
                    outputQueue.push(operatorStack.top());
                    operatorStack.pop();
                }
            }

            char getOpenChar(TokenType t)
            {
                if (t == TokenType::LeftParen)
                    return '(';
                if (t == TokenType::LeftBrace)
                    return '{';
                if (t == TokenType::LeftBracket)
                    return '[';
                return '\0';
            }
        };

        class Evaluator
        {
        public:
            long double evaluate(std::queue<Token> rpnQueue)
            {
                std::stack<long double> values;

                while (!rpnQueue.empty())
                {
                    Token token = rpnQueue.front();
                    rpnQueue.pop();

                    if (token.type == TokenType::Number)
                    {
                        values.push(std::stold(token.value));
                    }
                    else if (token.type == TokenType::Operator)
                    {
                        if (token.value == "~")
                        {
                            if (values.empty())
                                throw std::runtime_error("Invalid expression: Missing operand for unary minus");
                            long double a = values.top();
                            values.pop();
                            values.push(-a);
                        }
                        else
                        {
                            if (values.size() < 2)
                                throw std::runtime_error("Invalid expression: Missing operands for operator " + token.value);
                            long double b = values.top();
                            values.pop();
                            long double a = values.top();
                            values.pop();

                            if (token.value == "+")
                                values.push(a + b);
                            else if (token.value == "-")
                                values.push(a - b);
                            else if (token.value == "*")
                                values.push(a * b);
                            else if (token.value == "/")
                            {
                                // Use a small epsilon to avoid division-by-zero and extreme infinities.
                                b = auto_promote(b);
                                if (b == 0)
                                {
                                    throw std::runtime_error("Error: Division by zero!");
                                }
                                values.push(a / b);
                            }
                            else if (token.value == "^")
                                values.push(powl(a, b));
                        }
                    }
                    else if (token.type == TokenType::Identifier && is_math_function(token.value))
                    {
                        if (values.empty())
                            throw std::runtime_error("Invalid expression: Missing argument for function " + token.value);
                        long double arg = values.top();
                        values.pop();

                        if (token.value == "sin")
                            values.push(sinl(arg));
                        else if (token.value == "cos")
                            values.push(cosl(arg));
                        else if (token.value == "tan")
                            values.push(tanl(arg));
                        else if (token.value == "cot")
                            values.push(1.0L / tanl(arg));
                        else if (token.value == "sec")
                            values.push(1.0L / cosl(arg));
                        else if (token.value == "csc")
                            values.push(1.0L / sinl(arg));
                        else if (token.value == "asin")
                            values.push(asinl(arg));
                        else if (token.value == "acos")
                            values.push(acosl(arg));
                        else if (token.value == "atan")
                            values.push(atanl(arg));
                        else if (token.value == "log")
                            values.push(logl(arg));
                        else if (token.value == "log10")
                            values.push(log10l(arg));
                        else if (token.value == "log2")
                            values.push(log2l(arg));
                        else if (token.value == "sqrt")
                        {
                            if (arg < 0)
                                throw std::runtime_error("Domain error: sqrt of negative number");
                            values.push(sqrtl(arg));
                        }
                        else if (token.value == "abs")
                            values.push(fabsl(arg));
                    }
                    else
                    {
                        throw std::runtime_error("Unexpected identifier in evaluator: " + token.value);
                    }
                }

                if (values.size() != 1)
                {
                    throw std::runtime_error("Invalid expression: Stack not empty after evaluation");
                }
                return values.top();
            }
        };

        class Calculator
        {
        private:
            Tokenizer tokenizer;
            Parser parser;
            Evaluator evaluator;
            std::map<std::string, long double> variables;
            std::vector<std::pair<std::string, long double>> history;
            size_t history_index = 0; // to keep track of which statement has which result for keeping history
            std::string statement_history = "";

            std::vector<std::vector<Token>> preprocess(const std::string &line)
            {
                set_constants(); // initialize the constants
                // EXAMPLE: var a = 5, b = 2. var c = a^b + sin(a*b), d = c / (a + b). log(c) + sqrt(d) + cos(a+b)
                //  Tokenize
                auto tokens = tokenizer.tokenize(line);
                std::vector<std::vector<Token>> statements;
                std::vector<Token> statement;
                for (auto const &token : tokens)
                {
                    if (token.type != TokenType::Dot)
                    {
                        statement_history += token.value + " ";
                        statement.push_back(token);
                    }
                    else
                    {
                        statements.push_back(statement);
                        statement.clear();
                        history.emplace_back(statement_history, 0.0L);
                        statement_history.clear();
                    }
                }
                if (!statement.empty())
                {
                    statements.push_back(statement);
                    history.emplace_back(statement_history, 0.0L);
                    statement_history.clear();
                }
                if (DEBUG)
                {
                    for (const auto &statement : statements)
                    {
                        std::cout << "[ \n";
                        for (const auto &token : statement)
                        {
                            std::cout << "  [" << token.value << "]";
                        }
                        std::cout << "\n]\n"
                                  << std::endl;
                    }
                }
                return statements;
            }

        public:
            void process(const std::string &line)
            {
                if (line.empty())
                    return;

                // Preprocess to tokenize sentence level:
                std::vector<std::vector<Token>> statements = preprocess(line);
                for (auto &tokens : statements)
                {
                    long double result = 0.0l;
                    if (tokens.empty())
                        return;

                    size_t index = 0;
                    // 2. Check for "var declaration" or direct Assignment
                    bool isAssignment = false;
                    if (tokens[index].type == TokenType::KeywordVar)
                    {
                        index++; // Consume 'var'
                        isAssignment = true;
                    }
                    else if (tokens.size() > 1 && tokens[index].type == TokenType::Identifier && tokens[index + 1].type == TokenType::Equals)
                    {
                        isAssignment = true;
                        // Don't increment index, handleDeclarations starts at Identifier
                    }
                    try
                    {

                        if (isAssignment)
                        {
                            handleDeclarations(tokens, index);
                        }

                        // 3. If there are tokens left, evaluate as expression
                        if (index < tokens.size())
                        {
                            // Extract remaining tokens
                            std::vector<Token> exprTokens;
                            for (size_t i = index; i < tokens.size(); ++i)
                            {
                                // Skip leading delimiters if any
                                if (exprTokens.empty() && (tokens[i].type == TokenType::Comma || tokens[i].type == TokenType::Dot))
                                    continue;
                                exprTokens.push_back(tokens[i]);
                            }

                            if (!exprTokens.empty())
                            {
                                result = evaluateExpression(exprTokens);
                                result = auto_promote(result);
                                std::cout << "Ans: " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << result << std::endl;
                                reserved_constant["ans"] = std::make_pair(Token(TokenType::Identifier, "ans"), result);
                            }
                        }
                        history[history_index].second = result;
                        // DEBUG
                        //  std::cout << "Have put history in the history with result: " << result << " at index: " << history_index << std::endl;
                        history_index++;
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "Error: " << e.what() << std::endl;
                        if (!history.empty())
                        {
                            history.pop_back();
                        }
                    }
                }
            }

        private:
            void
            handleDeclarations(const std::vector<Token> &tokens, size_t &index)
            {
                // Format: var a = 10, b = 2*a + 5 . but var a=1 b=2  OR var a=1. b=2 are invalid for declaration.
                // We are at 'a' (presumably)

                while (index < tokens.size())
                {
                    // Debug
                    // std::cout << "Debug: handleDeclarations index=" << index << " Token=" << tokens[index].value << std::endl;

                    // Expect Identifier
                    if (tokens[index].type != TokenType::Identifier)
                    {
                        // Not an identifier, return.
                        return;
                    }

                    std::string varName = tokens[index].value;
                    if (is_math_function(varName)) // TODO: when adding new constant identifier tweak this part. like i was planning to add ans keyword
                    {
                        throw std::runtime_error("Cannot assign to reserved function '" + varName + "'");
                    }
                    else if (is_constant(varName))
                    {
                        throw std::runtime_error("Cannot assign to reserved constant '" + varName + "'");
                    }
                    // Check for '='
                    if (index + 1 >= tokens.size() || tokens[index + 1].type != TokenType::Equals)
                    {
                        return;
                    }

                    index++; // consume name
                    index++; // consume '='

                    // Extract expression until Comma, Dot, KeywordVar, or end of tokens
                    std::vector<Token> exprTokens;
                    while (index < tokens.size())
                    {
                        // NOTE: Already handing statement by statement so dont need explicityly checking for dot operator
                        // if (tokens[index].type == TokenType::Dot)
                        // {
                        //     index++; // consume the dot
                        //     // // if user wrote something like var a = 12 . b = 3 and b is an identifier and not a math funciton and  not pre-existing variable and followes by equal sign --> it should be error cz dot operator terminates the statemnt so the variable assignment is over.
                        //     // if (index + 1 < tokens.size() && tokens[index].type == TokenType::Identifier && !is_math_function(tokens[index].value) && variables.find(tokens[index].value) == variables.end() && tokens[index + 1].type == TokenType::Equals)
                        //     // {
                        //     //     throw std::runtime_error("Error: Unknown variable '" + tokens[index ].value + ". Did you mean " + tokens[index - 4].value + tokens[index - 3].value + tokens[index - 2].value + "," + tokens[index].value + tokens[index + 1].value + "?");
                        //     // }
                        //     // return; // if it's a . it means end of statement so no more assignment
                        //     break;
                        // }
                        if (tokens[index].type == TokenType::Comma)

                        {
                            index++; // Consume delimiter
                            break;   // End of this assignment
                        }

                        if (tokens[index].type == TokenType::KeywordVar)
                        {
                            break;
                        }
                        if (index + 1 < tokens.size() && tokens[index].type == TokenType::Identifier && !is_math_function(tokens[index].value) && tokens[index + 1].type == TokenType::Equals)
                        {
                            break;
                        }
                        exprTokens.push_back(tokens[index]);
                        index++;
                    }

                    if (exprTokens.empty())
                        throw std::runtime_error("Expected expression for variable '" + varName + "'");

                    long double val = evaluateExpression(exprTokens);
                    variables[varName] = val;
                    std::cout << "Variable " << varName << " = " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << val << std::endl;
                }
            }

            long double evaluateExpression(std::vector<Token> tokens)
            {
                auto longDoubleToString = [](long double v)
                {
                    std::ostringstream os;
                    os << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::defaultfloat << v;
                    return os.str();
                };
                // Substitute variables
                for (auto &t : tokens)
                {
                    if (t.type == TokenType::Identifier && !is_math_function(t.value))
                    {
                        auto it = variables.find(t.value);
                        if (it != variables.end())
                        {
                            t.type = TokenType::Number;
                            t.value = longDoubleToString(it->second);
                        }
                        else if (is_constant(t.value))
                        {
                            t.type = TokenType::Number;
                            t.value = longDoubleToString(reserved_constant[t.value].second);
                        }
                        else
                        {
                            // Debug
                            // std::cout << "Debug: Unknown variable '" << t.value << "'. Available: ";
                            // for (const auto& kv : variables) std::cout << kv.first << " ";
                            // std::cout << std::endl;
                            throw std::runtime_error("Unknown variable: " + t.value);
                        }
                    }
                }

                auto rpn = parser.parse(tokens);
                return evaluator.evaluate(rpn);
            }

        public:
            inline void wipe()
            {
                variables.clear();
                reserved_constant.clear();
                set_constants();
                history.clear();
                history_index = 0;
            }

            inline void show_history()
            {
                if (history.empty())
                {
                    std::cout << "History is empty.\n";
                    return;
                }
#ifdef WIN_32
                system("cls");
#else
                system("clear");
#endif
                print_title();
                for (auto const &h : history)
                {
                    std::cout << "Statement: " << h.first << "\nAns:" << h.second << std::endl;
                }
            }
        };

        // With argument
        inline void calculator(int argc, char **argv)
        {
            if (argc > 1)
            {
                std::string expr = argv[1];
                Calculator calc;
                calc.process(expr);
                return;
            }
            Calculator calc;
            std::string line;

            std::cout << "CLI Calculator" << std::endl;
            std::cout << "Features: + - * / ^ ( ) { } [ ]" << std::endl;
            std::cout << "Functions: sin, cos, tan, log, sqrt, etc." << std::endl;
            std::cout << "Variables: var a = 10, b = a*2" << std::endl;
            std::cout << "Type 'exit' or 'quit' to stop." << std::endl;

            while (true)
            {
                std::cout << ">> ";
                if (!std::getline(std::cin, line) || line == "exit" || line == "quit")
                {
                    break;
                }
                if (line.empty())
                    continue;

                try
                {
                    calc.process(line);
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            }

            return;
        }

        inline void help()
        {
            std::cout << R"(Calculator Help

    Usage:
        - Enter expressions or variable declarations at the prompt.
        - Separate multiple statements with a dot (.)
        - Declare variables:  var a = 10, b = a*2
        - Assign: a = 5

    Expressions:
        - Operators: +  -  *  /  ^
        - Grouping: ( ) { } [ ]
        - Implicit multiplication: 2(3+4) or 2PI
        - Unary minus: -x or -(expr)

    Functions:
        - sin, cos, tan, cot, sec, csc
        - asin, acos, atan, acot, asec, acsc
        - log, log2, log10, sqrt, abs

    Constants:
        - PI, e, tau, deg, inf, infinity, nan, ans
        - 'ans' is updated after each evaluated expression

    Special Commands:
        - clear    : Clear the terminal screen.
        - wipe     : Reset all variables, constants, and history.
        - history  : Show all previous statements and their results.
        - help     : Show this help message.
        - exit/quit: Exit the calculator REPL.

    Flags (test binary):
        --eval "<expr>"   Evaluate expression(s) and exit
        --debug           Enable debug output

    Notes:
        - The calculator uses long double precision for evaluation and printing.
        - Division by very small denominators is guarded by a small epsilon to avoid infinities.
        - Reserved identifiers (functions/constants) cannot be assigned.

    Examples:
        var a = PI*2, b = sin(a) + e.
        var c = (a^2 + b^2) / (1 + cos(a/2)).
        sqrt(c) + deg

    Type 'exit' or 'quit' to stop the REPL.
    )" << std::endl;
        }

        // without argument
        inline void calculator()
        {
            Calculator calc;
            std::string line;
            print_title();
            while (true)
            {
#ifdef HAVE_READLINE
                char *input = readline(">> ");
                if (!input)
                    break; // Ctrl+D
                line = input;
                free(input);
                if (!line.empty())
                    add_history(line.c_str());
#else
                std::cout << ">> ";
                if (!std::getline(std::cin, line))
                    break;
#endif

                if (line == "exit" || line == "quit")
                    break;
                else if (line == "help")
                {
                    help();
                    continue;
                }
                else if (line == "clear")
                {
#ifdef WIN_32
                    system("cls");
#else
                    system("clear");
#endif
                    print_title();
                    continue;
                }
                else if (line == "wipe")
                {
                    calc.wipe();
#ifdef WIN_32
                    system("cls");
#else
                    system("clear");
#endif
                    print_title();
                    continue;
                }
                else if (line == "history")
                {
                    calc.show_history();
                    continue;
                }

                if (line.empty())
                    continue;

                try
                {
                    calc.process(line);
                }
                catch (const std::exception &e)
                {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            }

            return;
        }
    }
}