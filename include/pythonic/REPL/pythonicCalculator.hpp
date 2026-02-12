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

namespace pythonic
{
    namespace calculator
    {

        // --- Enums & Structures ---

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
            Equals,
            Comma,
            Dot // .
        };

        struct Token
        {
            TokenType type;
            std::string value;
            int position;

            Token(TokenType t, std::string v, int pos = -1)
                : type(t), value(v), position(pos) {}
        };
        // --- Helper Maps ---

        // Precedence:
        // 1: + -
        // 2: * /
        // 3: ^
        // 4: Unary Minus (~)
        // 5: Functions

        int get_operator_precedence(const std::string &op)
        {
            if (op == "+" || op == "-")
                return 1;
            if (op == "*" || op == "/")
                return 2;
            if (op == "^")
                return 3;
            if (op == "~")
                return 4; // Unary minus
            return 0;
        }

        bool is_right_associative(const std::string &op)
        {
            return op == "^" || op == "~";
        }

        bool is_math_function(const std::string &str)
        {
            static const std::vector<std::string> funcs = {
                "sin", "cos", "tan", "cot", "sec", "csc",
                "asin", "acos", "atan", "acot", "asec", "acsc",
                "log", "log2", "log10", "sqrt", "abs"};
            return std::find(funcs.begin(), funcs.end(), str) != funcs.end();
        }

        // --- Tokenizer ---

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

                    if (std::isspace(c))
                        continue;

                    // Numbers
                    if (std::isdigit(c) || c == '.')
                    {
                        // CASE: '.'
                        // If it is a dot, it COULD be a number (.5) or a delimiter (.).
                        // Check if it is followed by digit -> Number.
                        // If not followed by digit -> Delimiter.
                        if (c == '.')
                        {
                            bool nextIsDigit = (i + 1 < expr.length() && std::isdigit(expr[i + 1]));
                            if (!nextIsDigit)
                            {
                                // Treat as delimiter
                                tokens.emplace_back(TokenType::Dot, ".", i);
                                continue;
                            }
                        }

                        // Implicit multiplication check
                        if (!tokens.empty())
                        {
                            TokenType lastType = tokens.back().type;
                            if (lastType == TokenType::RightParen ||
                                lastType == TokenType::RightBrace ||
                                lastType == TokenType::RightBracket)
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
                            if (lastType == TokenType::Number ||
                                lastType == TokenType::RightParen ||
                                lastType == TokenType::RightBrace ||
                                lastType == TokenType::RightBracket)
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

        // --- Parser (Shunting-yard) ---

        class Parser
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
                            // Variable value (should be replaced, but if not, treat as value)
                            operatorStack.push(token);
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
                    throw std::runtime_error("Mismatched parentheses/brackets");
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

        // --- Evaluator ---

        class Evaluator
        {
        public:
            double evaluate(std::queue<Token> rpnQueue)
            {
                std::stack<double> values;

                while (!rpnQueue.empty())
                {
                    Token token = rpnQueue.front();
                    rpnQueue.pop();

                    if (token.type == TokenType::Number)
                    {
                        values.push(std::stod(token.value));
                    }
                    else if (token.type == TokenType::Operator)
                    {
                        if (token.value == "~")
                        {
                            if (values.empty())
                                throw std::runtime_error("Invalid expression: Missing operand for unary minus");
                            double a = values.top();
                            values.pop();
                            values.push(-a);
                        }
                        else
                        {
                            if (values.size() < 2)
                                throw std::runtime_error("Invalid expression: Missing operands for operator " + token.value);
                            double b = values.top();
                            values.pop();
                            double a = values.top();
                            values.pop();

                            if (token.value == "+")
                                values.push(a + b);
                            else if (token.value == "-")
                                values.push(a - b);
                            else if (token.value == "*")
                                values.push(a * b);
                            else if (token.value == "/")
                            {
                                if (b == 0)
                                    throw std::runtime_error("Division by zero");
                                values.push(a / b);
                            }
                            else if (token.value == "^")
                                values.push(std::pow(a, b));
                        }
                    }
                    else if (token.type == TokenType::Identifier && is_math_function(token.value))
                    {
                        if (values.empty())
                            throw std::runtime_error("Invalid expression: Missing argument for function " + token.value);
                        double arg = values.top();
                        values.pop();

                        if (token.value == "sin")
                            values.push(std::sin(arg));
                        else if (token.value == "cos")
                            values.push(std::cos(arg));
                        else if (token.value == "tan")
                            values.push(std::tan(arg));
                        else if (token.value == "cot")
                            values.push(1.0 / std::tan(arg));
                        else if (token.value == "sec")
                            values.push(1.0 / std::cos(arg));
                        else if (token.value == "csc")
                            values.push(1.0 / std::sin(arg));
                        else if (token.value == "asin")
                            values.push(std::asin(arg));
                        else if (token.value == "acos")
                            values.push(std::acos(arg));
                        else if (token.value == "atan")
                            values.push(std::atan(arg));
                        else if (token.value == "log")
                            values.push(std::log(arg));
                        else if (token.value == "log10")
                            values.push(std::log10(arg));
                        else if (token.value == "log2")
                            values.push(std::log2(arg));
                        else if (token.value == "sqrt")
                        {
                            if (arg < 0)
                                throw std::runtime_error("Domain error: sqrt of negative number");
                            values.push(std::sqrt(arg));
                        }
                        else if (token.value == "abs")
                            values.push(std::abs(arg));
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

        // --- Calculator Interface ---

        class Calculator
        {
        private:
            Tokenizer tokenizer;
            Parser parser;
            Evaluator evaluator;
            std::map<std::string, double> variables;

        public:
            void process(const std::string &line)
            {
                if (line.empty())
                    return;

                // 1. Tokenize
                auto tokens = tokenizer.tokenize(line);
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
                        double result = evaluateExpression(exprTokens);
                        std::cout << result << std::endl;
                    }
                }
            }

        private:
            void handleDeclarations(const std::vector<Token> &tokens, size_t &index)
            {
                // Format: var a = 10, b = 2*a + 5  OR var a=1 b=2  OR var a=1. b=2
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
                    if (is_math_function(varName))
                    {
                        throw std::runtime_error("Cannot assign to reserved function '" + varName + "'");
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
                        if (tokens[index].type == TokenType::Comma || tokens[index].type == TokenType::Dot)
                        {
                            index++; // Consume delimiter
                            break;   // End of this assignment
                        }

                        if (tokens[index].type == TokenType::KeywordVar)
                        {
                            break;
                        }

                        exprTokens.push_back(tokens[index]);
                        index++;
                    }

                    if (exprTokens.empty())
                        throw std::runtime_error("Expected expression for variable '" + varName + "'");

                    double val = evaluateExpression(exprTokens);
                    variables[varName] = val;
                    std::cout << "Variable " << varName << " = " << val << std::endl;
                }
            }

            double evaluateExpression(std::vector<Token> tokens)
            {
                // Substitute variables
                for (auto &t : tokens)
                {
                    if (t.type == TokenType::Identifier && !is_math_function(t.value))
                    {
                        auto it = variables.find(t.value);
                        if (it != variables.end())
                        {
                            t.type = TokenType::Number;
                            t.value = std::to_string(it->second);
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
        };


        inline void calculator()
        {
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

            return ;
        }
    }
}