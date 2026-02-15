#pragma once
// scriptit_types.hpp — Token types, AST nodes, Scope, helpers for ScriptIt v0.3.0

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
using pythonic::vars::VarGraphWrapper;

// ─── Token Types ──────────────────────────────────────────

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
    KeywordOf,
    KeywordStep,
    KeywordIs,
    KeywordPoints,
    Equals,
    PlusEquals,    // +=
    MinusEquals,   // -=
    StarEquals,    // *=
    SlashEquals,   // /=
    PercentEquals, // %=
    PlusPlus,      // ++
    MinusMinus,    // --
    Arrow,         // ->  (directed edge, dict key-value)
    BiArrow,       // <-> (bidirectional edge)
    Dash,          // -   used contextually for undirected edge in add_edge(A - B)
    Comma,
    Dot,
    Colon,
    Semicolon,
    At,
    CommentStart,
    CommentEnd,
    Newline,
    Eof
};

struct Token
{
    TokenType type;
    std::string value;
    int position;
    int line;
    Token(TokenType t, std::string v, int pos = -1, int ln = -1)
        : type(t), value(std::move(v)), position(pos), line(ln) {}
};

// ─── Helper Functions ─────────────────────────────────────

inline bool is_math_function(const std::string &str)
{
    static const std::unordered_set<std::string> funcs = {
        "sin", "cos", "tan", "cot", "sec", "csc",
        "asin", "acos", "atan", "acot", "asec", "acsc",
        "log", "log2", "log10", "sqrt", "abs", "min", "max",
        "ceil", "floor", "round"};
    return funcs.count(str);
}

inline bool is_builtin_function(const std::string &str)
{
    static const std::unordered_set<std::string> funcs = {
        // I/O
        "print", "pprint", "read", "write", "readLine", "input",
        // type / conversion
        "len", "type", "str", "int", "float", "double", "bool", "repr", "isinstance",
        "long", "long_long", "long_double", "uint", "ulong", "ulong_long", "auto_numeric",
        // containers
        "append", "pop", "list", "set", "dict", "range_list", "graph",
        // functional / iteration
        "sum", "sorted", "reversed", "all", "any",
        "enumerate", "zip", "map",
        // math (free-function form)
        "abs", "min", "max",
        // file I/O
        "open", "close"};
    return funcs.count(str) || is_math_function(str);
}

inline int get_operator_precedence(const std::string &op)
{
    static const std::unordered_map<std::string, int> precedence = {
        {"||", 1}, {"&&", 2}, {"is", 3}, {"is not", 3}, {"points", 3}, {"not points", 3}, {"==", 3}, {"!=", 3}, {"<", 4}, {"<=", 4}, {">", 4}, {">=", 4}, {"->", 4}, {"<->", 4}, {"---", 4}, {"+", 5}, {"-", 5}, {"*", 6}, {"/", 6}, {"%", 6}, {"^", 7}, {"~", 8}, {"!", 8}};
    auto it = precedence.find(op);
    return it != precedence.end() ? it->second : 0;
}

inline double var_to_double(const var &v)
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
inline std::string format_output(const var &v)
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

// ─── AST Nodes ────────────────────────────────────────────

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
    std::shared_ptr<Expression> stepExpr; // optional step
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
    std::vector<bool> isRefParam; // true if param is pass-by-reference (@param)
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

struct LetContextStmt : Statement
{
    std::string name;                    // variable name for the resource
    std::shared_ptr<Expression> expr;    // the open(...) expression
    std::shared_ptr<BlockStmt> body;     // block of statements to execute
    void execute(Scope &scope) override; // defined in ScriptIt.cpp
};

// ─── Environment / Scope ──────────────────────────────────

struct ReturnException : public std::exception
{
    var value;
    ReturnException(var v) : value(std::move(v)) {}
};

struct FunctionDef
{
    std::string name;
    std::vector<std::string> params;
    std::vector<bool> isRefParam; // true if param is pass-by-reference (@param)
    std::shared_ptr<BlockStmt> body;
};

struct Scope
{
    std::map<std::string, var> values;
    std::map<std::string, FunctionDef> functions;      // key = "name/arity"
    std::unordered_set<std::string> declaredFunctions; // forward-declared keys ("name/arity")
    Scope *parent;
    bool barrier;

    Scope(Scope *p = nullptr, bool b = false) : parent(p), barrier(b) {}

    static std::string funcKey(const std::string &name, int arity) { return name + "/" + std::to_string(arity); }

    void define(const std::string &name, const var &val) { values[name] = val; }

    void defineFunction(const std::string &name, const FunctionDef &def)
    {
        std::string key = funcKey(name, (int)def.params.size());
        functions[key] = def;
        declaredFunctions.erase(key);
    }

    void declareFunction(const std::string &name, const std::vector<std::string> &params)
    {
        std::string key = funcKey(name, (int)params.size());
        if (functions.count(key) && !declaredFunctions.count(key))
            throw std::runtime_error("Function '" + name + "' with " + std::to_string(params.size()) + " params is already defined (cannot re-declare)");
        declaredFunctions.insert(key);
        // Store a stub so getFunction doesn't crash
        FunctionDef stub;
        stub.name = name;
        stub.params = params;
        stub.body = nullptr; // no body yet
        functions[key] = stub;
    }

    bool isFunctionDeclaredOnly(const std::string &name, int arity)
    {
        std::string key = funcKey(name, arity);
        if (declaredFunctions.count(key))
            return true;
        if (parent)
            return parent->isFunctionDeclaredOnly(name, arity);
        return false;
    }

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

    FunctionDef getFunction(const std::string &name, int arity)
    {
        std::string key = funcKey(name, arity);
        if (functions.count(key))
            return functions[key];
        if (parent)
            return parent->getFunction(name, arity);
        throw std::runtime_error("Unknown function: " + name + " with " + std::to_string(arity) + " arg(s)");
    }

    bool hasFunction(const std::string &name, int arity)
    {
        std::string key = funcKey(name, arity);
        if (functions.count(key))
            return true;
        if (parent)
            return parent->hasFunction(name, arity);
        return false;
    }

    void clear()
    {
        values.clear();
        functions.clear();
        declaredFunctions.clear();
    }

    const std::map<std::string, var> &getAll() const
    {
        return values;
    }
};
