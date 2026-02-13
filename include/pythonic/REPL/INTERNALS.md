# 🔬 ScriptIt v2 — Complete Interpreter Internals

> **Goal of this document:** If you know nothing about how programming languages work when you start reading, by the time you finish you will understand every piece of ScriptIt v2 well enough to build your own interpreter from scratch.

---

## Table of Contents

1. [What Is an Interpreter?](#1-what-is-an-interpreter)
2. [The Big Picture — Architecture](#2-the-big-picture--architecture)
3. [The Type System — `pythonic::vars::var`](#3-the-type-system--pythonicvarsvar)
4. [File-by-File Breakdown](#4-file-by-file-breakdown)
   - 4.1 [scriptit_types.hpp — The Foundation](#41-scriptit_typeshpp--the-foundation)
   - 4.2 [perser.hpp — The Brain (Tokenizer + Evaluator + Parser)](#42-perserhpp--the-brain)
   - 4.3 [scriptit_methods.hpp — Dot-Method Dispatch](#43-scriptit_methodshpp--dot-method-dispatch)
   - 4.4 [scriptit_builtins.hpp — Built-in Functions](#44-scriptit_builtinshpp--built-in-functions)
   - 4.5 [json_and_kernel.hpp — Notebook Integration](#45-json_and_kernelhpp--notebook-integration)
   - 4.6 [ScriptIt.cpp — The Entry Point](#46-scriptitcpp--the-entry-point)
5. [The Language — Syntax Reference](#5-the-language--syntax-reference)
6. [How the Pieces Connect — The Full Pipeline](#6-how-the-pieces-connect--the-full-pipeline)
7. [Deep Dive: The Tokenizer](#7-deep-dive-the-tokenizer)
8. [Deep Dive: The Parser (Recursive Descent + Shunting-Yard)](#8-deep-dive-the-parser)
9. [Deep Dive: The Evaluator (Stack-Based RPN)](#9-deep-dive-the-evaluator)
10. [Deep Dive: Scope and Variable Resolution](#10-deep-dive-scope-and-variable-resolution)
11. [Deep Dive: Functions, Overloading, and Pass-by-Reference](#11-deep-dive-functions-overloading-and-pass-by-reference)
12. [Deep Dive: Two-Pass Execution and Forward Declarations](#12-deep-dive-two-pass-execution-and-forward-declarations)
13. [Operator Precedence Table](#13-operator-precedence-table)
14. [How to Build ScriptIt](#14-how-to-build-scriptit)
15. [Glossary](#15-glossary)

---

## 1. What Is an Interpreter?

An **interpreter** is a program that reads source code and executes it directly, without first compiling it to machine code. Think of it as a translator that reads a foreign language sentence by sentence and speaks it aloud in real time, rather than translating the entire book first.

ScriptIt v2 is specifically a **tree-walking interpreter**. Here's what that means:

1. **Read** the source code (a string of characters)
2. **Tokenize** — break the string into meaningful chunks called _tokens_ (like words in a sentence)
3. **Parse** — arrange tokens into a tree structure (called an _Abstract Syntax Tree_ or AST)
4. **Walk the tree** — visit each node and execute the corresponding operation

This is the simplest and most intuitive kind of interpreter. It's not the fastest (a bytecode compiler + VM would be faster), but it's the easiest to understand and build.

---

## 2. The Big Picture — Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Source Code                              │
│                  "var x = 3 + 4."                               │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                  STAGE 1: Tokenizer                             │
│        (perser.hpp — class Tokenizer)                           │
│                                                                 │
│  Reads characters one by one. Produces a flat list of tokens:   │
│  [KeywordVar,"var"] [Identifier,"x"] [Equals,"="]              │
│  [Number,"3"] [Operator,"+"] [Number,"4"] [Dot,"."] [Eof,""]   │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                  STAGE 2: Parser                                │
│        (perser.hpp — class Parser)                              │
│                                                                 │
│  Reads tokens left-to-right. Builds an Abstract Syntax Tree:   │
│                                                                 │
│              BlockStmt                                          │
│                 │                                               │
│            AssignStmt                                           │
│           ┌────┼────────┐                                       │
│        name="x"  isDecl  Expression                             │
│                    =true   rpn: [3] [4] [+]                     │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                  STAGE 3: Evaluator                             │
│        (perser.hpp — Expression::evaluate)                      │
│        (perser.hpp — Statement::execute)                        │
│                                                                 │
│  Walks the tree. For each node:                                 │
│    AssignStmt → evaluate the expression, store in scope         │
│    Expression [3] [4] [+] → push 3, push 4, pop both, add,     │
│                              push 7 → result is var(7)          │
│                                                                 │
│  Scope now contains: { "x" → var(7) }                          │
└─────────────────────────────────────────────────────────────────┘
```

### The 6 Source Files

| File                    | Lines | Role                                                         |
| ----------------------- | ----- | ------------------------------------------------------------ |
| `scriptit_types.hpp`    | ~423  | Token types, AST node structs, Scope class, helper functions |
| `perser.hpp`            | ~1844 | **The main file** — Tokenizer, Expression evaluator, Parser  |
| `scriptit_methods.hpp`  | ~624  | Dot-method dispatch (e.g., `"hello".upper()`)                |
| `scriptit_builtins.hpp` | ~764  | Built-in functions (`print`, `input`, `len`, `open`, etc.)   |
| `json_and_kernel.hpp`   | ~248  | JSON helpers + kernel mode for notebook integration          |
| `ScriptIt.cpp`          | ~165  | `main()` function — entry point, REPL, file execution        |

### The Value Type

Every single value in ScriptIt is a `pythonic::vars::var`. This is a C++ class from the `pythonic` library that acts like a dynamic type — it can hold an int, double, string, list, set, dict, bool, or None, and you can change its type at any time. Think of it like Python's `object` — everything is a `var`.

### Arithmetic Engine

All math operations go through `pythonic::math` with **overflow promotion**:

```
int → long → long_long → float → double → long_double
```

If multiplying two ints would overflow, the result is automatically promoted to a `long_long`. If that overflows, it becomes a `double`, and so on. You never get silent overflow bugs.

---

## 3. The Type System — `pythonic::vars::var`

The `var` class is the heart of ScriptIt's runtime. Every variable, every expression result, every function argument and return value is a `var`.

### What Types Can `var` Hold?

| ScriptIt Literal | Internal C++ Type | `var` constructor      | `.type()` returns |
| ---------------- | ----------------- | ---------------------- | ----------------- |
| `42`             | `int`             | `var(42)`              | `"int"`           |
| `3.14`           | `double`          | `var(3.14)`            | `"double"`        |
| `3.14f`          | `float`           | `var(3.14f)`           | `"float"`         |
| `100000000000`   | `long long`       | `var((long long)1e11)` | `"long_long"`     |
| `"hello"`        | `std::string`     | `var("hello")`         | `"string"`        |
| `True` / `False` | `bool`            | `var(true)`            | `"bool"`          |
| `None` (no init) | `NoneType`        | `var(NoneType{})`      | `"NoneType"`      |
| `[1, 2, 3]`      | `List`            | `var(List{...})`       | `"list"`          |
| `{1, 2, 3}`      | `Set`             | `var(Set{...})`        | `"set"`           |
| `dict(...)`      | `Dict`            | `var(Dict{...})`       | `"dict"`          |

### Key `var` Operations Used in the Interpreter

```cpp
var x(42);
x.is_int()              // true — type checking
x.as_int_unchecked()    // 42 — fast access (no type check)
x.str()                 // "42" — string representation
x.type()                // "int" — type name as string
static_cast<bool>(x)    // true — truthiness (0 and None are false)
x.is_none()             // false
x.len()                 // throws for non-container types
```

### Overflow Promotion in Action

```
ScriptIt:  var x = 1000000 * 1000000.
           --> int * int would overflow 32-bit int
           --> pythonic::math::mul auto-promotes
           --> result: var(1000000000000) with type long_long
```

The promotion chain is: `int → long → long_long → float → double → long_double`. The `pythonic::math` functions (`add`, `sub`, `mul`, `div`, `mod`, `pow`) all take an `Overflow::Promote` flag that enables this behavior.

---

## 4. File-by-File Breakdown

### 4.1 `scriptit_types.hpp` — The Foundation

**Purpose:** Defines every data structure the interpreter uses — tokens, AST nodes, the scope/environment, and helper utilities. Think of this as the "vocabulary" that all other files speak.

#### TokenType Enum

Every token has a **type** that tells the parser what kind of thing it is. The full enum:

```cpp
enum class TokenType {
    // Literals and identifiers
    Number,          // 42, 3.14, .5
    String,          // "hello", 'world'
    Identifier,      // x, myFunc, long_double

    // Operators
    Operator,        // +, -, *, /, %, ^, ==, !=, <, >, <=, >=, &&, ||, !, ~

    // Delimiters
    LeftParen,       // (
    RightParen,      // )
    LeftBrace,       // {
    RightBrace,      // }
    LeftBracket,     // [
    RightBracket,    // ]

    // Keywords
    KeywordVar,      // var
    KeywordFn,       // fn
    KeywordGive,     // give (return)
    KeywordIf,       // if
    KeywordElif,     // elif
    KeywordElse,     // else
    KeywordFor,      // for
    KeywordIn,       // in
    KeywordRange,    // range
    KeywordFrom,     // from
    KeywordTo,       // to
    KeywordStep,     // step
    KeywordPass,     // pass
    KeywordWhile,    // while
    KeywordAre,      // are
    KeywordNew,      // new
    KeywordLet,      // let
    KeywordBe,       // be
    KeywordOf,       // of (postfix method syntax)
    KeywordIs,       // is (identity comparison)
    KeywordPoints,   // points (reference comparison)

    // Assignment variants
    Equals,          // =
    PlusEquals,      // +=
    MinusEquals,     // -=
    StarEquals,      // *=
    SlashEquals,     // /=
    PercentEquals,   // %=
    PlusPlus,        // ++
    MinusMinus,      // --

    // Punctuation
    Comma,           // ,
    Dot,             // .  (statement terminator)
    Colon,           // :  (block opener)
    Semicolon,       // ;  (block closer)
    At,              // @  (method call marker in RPN, also ref-param prefix)

    // Special
    CommentStart,    // -->
    CommentEnd,      // <--
    Newline,         // \n (implicit statement terminator)
    Eof              // end of input
};
```

#### Token Struct

Each token carries four pieces of information:

```cpp
struct Token {
    TokenType type;      // What kind of token (Number, String, Operator, etc.)
    std::string value;   // The actual text ("42", "+", "hello", "var")
    int position;        // Character offset in source (for error messages)
    int line;            // Line number (for error messages)
};
```

**Example:** The source `var x = 42.` produces these tokens:

| #   | type       | value   | position | line |
| --- | ---------- | ------- | -------- | ---- |
| 0   | KeywordVar | `"var"` | 0        | 1    |
| 1   | Identifier | `"x"`   | 4        | 1    |
| 2   | Equals     | `"="`   | 6        | 1    |
| 3   | Number     | `"42"`  | 8        | 1    |
| 4   | Dot        | `"."`   | 10       | 1    |
| 5   | Eof        | `""`    | -1       | 1    |

#### Helper Functions

```cpp
// Is this identifier a math function? (sin, cos, sqrt, abs, etc.)
bool is_math_function(const std::string &str);

// Is this identifier any built-in function? (print, len, type, etc.)
bool is_builtin_function(const std::string &str);

// What's the precedence of this operator? (1=lowest, 8=highest)
int get_operator_precedence(const std::string &op);

// Convert any numeric var to a C++ double
double var_to_double(const var &v);

// Format a var for display (backward compatible with v1 output)
std::string format_output(const var &v);
```

The `is_math_function` set includes: `sin`, `cos`, `tan`, `cot`, `sec`, `csc`, `asin`, `acos`, `atan`, `acot`, `asec`, `acsc`, `log`, `log2`, `log10`, `sqrt`, `abs`, `min`, `max`, `ceil`, `floor`, `round`.

The `is_builtin_function` set includes: `print`, `pprint`, `read`, `write`, `readLine`, `input`, `len`, `type`, `str`, `int`, `float`, `double`, `bool`, `repr`, `isinstance`, `long`, `long_long`, `long_double`, `uint`, `ulong`, `ulong_long`, `auto_numeric`, `append`, `pop`, `list`, `set`, `dict`, `range_list`, `sum`, `sorted`, `reversed`, `all`, `any`, `enumerate`, `zip`, `map`, `abs`, `min`, `max`, `open`, `close` — plus every math function.

The `format_output` function formats values for display. It handles the special cases: `None` prints as `"None"`, booleans as `"True"`/`"False"`, strings as their raw content (no quotes), and floating-point numbers via `std::ostringstream` to avoid trailing zeros.

#### AST Node Hierarchy

AST stands for **Abstract Syntax Tree**. Each node represents one piece of your program. Here's the inheritance hierarchy:

```
ASTNode (base — virtual destructor)
├── Statement (has execute(Scope&))
│   ├── BlockStmt          — a sequence of statements
│   ├── IfStmt             — if/elif/else chain
│   ├── ForStmt            — for i in range(...)
│   ├── ForInStmt          — for x in collection
│   ├── WhileStmt          — while condition
│   ├── FunctionDefStmt    — fn name(params): body ;
│   ├── ReturnStmt         — give expr.
│   ├── AssignStmt         — x = expr. (or var x = expr.)
│   ├── ExprStmt           — standalone expression (e.g., print("hi").)
│   ├── PassStmt           — pass. (no-op)
│   ├── MultiVarStmt       — var a = 1 b = 2 c = 3.
│   └── LetContextStmt     — let f be open(...): ... ;
│
└── Expression (has evaluate(Scope&) → var)
        Contains: rpn (list of tokens in RPN order)
                  logicalOp, lhs, rhs (for short-circuit &&/||)
```

**Key insight:** Statements _do things_ (assign, loop, branch). Expressions _produce values_ (3+4 → 7). Every statement contains zero or more expressions.

Let's look at each AST node in detail:

**`BlockStmt`** — A list of statements. The outermost program is a BlockStmt. Function bodies are BlockStmts. If/for/while bodies are BlockStmts. It has special two-pass execution logic (covered in Section 12).

**`IfStmt`** — Contains a list of `Branch` structs (each has a condition expression and a body block), plus an optional `elseBlock`. Branches are tested in order; the first truthy one executes.

**`ForStmt`** — Range-based loop. Has `iteratorName` (the loop variable), `startExpr`, `endExpr`, and optional `stepExpr`. Executes by computing start/end/step, then looping with a double counter.

**`ForInStmt`** — Collection iteration. Has `iteratorName` and `iterableExpr` (must evaluate to a list, string, or set). Uses the `var`'s iterator support to loop over elements.

**`FunctionDefStmt`** — Has `name`, `params` (list of parameter names), `isRefParam` (parallel bool list for @ params), and `body` (null for forward declarations).

**`ReturnStmt`** — Wraps an expression. On execute, evaluates the expression and throws `ReturnException`.

**`AssignStmt`** — Has `name`, `expr`, and `isDeclaration` flag. If `isDeclaration` is true, calls `scope.define()` (create new). Otherwise calls `scope.set()` (update existing).

**`ExprStmt`** — A standalone expression. Evaluates it, and if the result is not None, prints it to stdout. This is what makes the REPL work — typing `3 + 4` prints `7`.

**`PassStmt`** — Does literally nothing. Used as a placeholder in empty function bodies.

**`MultiVarStmt`** — Contains a list of `AssignStmt`s. Created by `var a = 1 b = 2 c = 3.` syntax.

**`LetContextStmt`** — Context manager. Has `name` (variable to bind), `expr` (the resource expression, e.g., `open(...)`), and `body` (the block to execute). After the block completes (or throws), it auto-closes file resources.

#### The `ReturnException`

ScriptIt uses a C++ exception to implement the `give` (return) statement:

```cpp
struct ReturnException : public std::exception {
    var value;  // The returned value
    ReturnException(var v) : value(std::move(v)) {}
};
```

When `give expr.` executes, it **throws** a `ReturnException`. The function call handler **catches** it and extracts the return value. This is a common technique in tree-walking interpreters because it lets a return statement escape from deep nesting (loops inside ifs inside other blocks) without complex control flow.

#### The `FunctionDef` Struct

```cpp
struct FunctionDef {
    std::string name;              // "add", "fibonacci", etc.
    std::vector<std::string> params;     // ["a", "b"]
    std::vector<bool> isRefParam;        // [false, true] → b is pass-by-ref
    std::shared_ptr<BlockStmt> body;     // The function body (null = forward declaration)
};
```

#### The `Scope` Class — Variable and Function Storage

This is how ScriptIt knows what variables and functions exist. Scopes form a **chain** (linked list) from inner to outer:

```
┌──────────────────────────┐
│  Global Scope            │
│  values: {PI, e, x}     │    parent = nullptr
│  functions: {add/2}     │
└──────────┬───────────────┘
           │ parent
┌──────────┴───────────────┐
│  Function Scope          │
│  values: {a, b}          │    barrier = true (can't see outer vars for mutation)
│  functions: {}           │
└──────────┬───────────────┘
           │ parent
┌──────────┴───────────────┐
│  Block Scope (if body)   │
│  values: {temp}          │    barrier = false
│  functions: {}           │
└──────────────────────────┘
```

**Key methods:**

```cpp
scope.define("x", var(42));           // Create new variable in THIS scope
scope.set("x", var(99));             // Update existing variable (walks up chain)
scope.get("x");                      // Read variable (walks up chain; returns None if not found)
scope.defineFunction("add", def);    // Register function with key "add/2" (name/arity)
scope.getFunction("add", 2);        // Look up function by name + arg count
scope.hasFunction("add", 2);        // Check if function exists
scope.declareFunction("add", params); // Forward-declare (stub, no body yet)
scope.clear();                       // Wipe everything (used by REPL "wipe" command)
```

**The barrier flag:** When `barrier = true`, the `set()` method will NOT walk up to the parent scope to mutate variables. This prevents functions from accidentally modifying global variables. Functions create scopes with `barrier = true`. Blocks inside functions create scopes with `barrier = false` (so they CAN see the function's local variables).

**Function keys:** Functions are stored with a key like `"add/2"` — the name plus the number of parameters. This allows **function overloading by arity**: `fn add(a, b)` and `fn add(a, b, c)` are different functions stored under `"add/2"` and `"add/3"`.

The `funcKey` static method generates these keys:

```cpp
static std::string funcKey(const std::string &name, int arity) {
    return name + "/" + std::to_string(arity);
}
```

---

### 4.2 `perser.hpp` — The Brain

This ~1844-line file is the beating heart of the interpreter. It contains three major components, and each is a completely different algorithm. Let's look at each one.

**(Detailed deep dives in Sections 7, 8, and 9 below.)**

#### Component 1: `class Tokenizer`

The tokenizer (also called a **lexer** or **scanner**) reads the source string character by character and groups characters into tokens. For example, it sees the characters `v`, `a`, `r` and groups them into the single token `[KeywordVar, "var"]`.

#### Component 2: `Expression::evaluate(Scope&)`

The evaluator takes an expression's **RPN** (Reverse Polish Notation) token list and evaluates it using a **stack machine**. It processes tokens left to right: numbers and identifiers get pushed onto the stack, operators pop their operands, compute, and push the result.

#### Component 3: `class Parser`

The parser reads tokens left to right and builds AST nodes. It uses **recursive descent** for statements (if, for, while, fn) and the **Shunting-Yard algorithm** for expressions (arithmetic, comparisons, function calls).

#### Statement Implementations (also in perser.hpp)

After the three major classes, `perser.hpp` also contains the `execute()` implementations for all AST node types:

- `BlockStmt::execute()` — Two-pass: hoist functions, then run statements
- `IfStmt::execute()` — Test branches in order, run first truthy one
- `ForStmt::execute()` — Compute start/end/step, loop with counter
- `ForInStmt::execute()` — Iterate over list/string/set elements
- `WhileStmt::execute()` — Loop while condition is truthy
- `FunctionDefStmt::execute()` — Register function in scope (or stub for forward decl)
- `ReturnStmt::execute()` — Throw `ReturnException`
- `AssignStmt::execute()` — Evaluate RHS, define or set variable
- `ExprStmt::execute()` — Evaluate expression, print non-None results
- `LetContextStmt::execute()` — Create child scope, bind resource, auto-close on exit

---

### 4.3 `scriptit_methods.hpp` — Dot-Method Dispatch

**Purpose:** When you write `"hello".upper()` or `myList.append(42)`, this file decides which C++ function to call.

#### Architecture: Method Tables

Methods are organized by **argument count** (0, 1, 2, or 3 args beyond `self`):

```cpp
using Method0 = std::function<var(var &)>;            // e.g., .upper()
using Method1 = std::function<var(var &, const var &)>;        // e.g., .append(x)
using Method2 = std::function<var(var &, const var &, const var &)>;  // e.g., .replace(old, new)
using Method3 = std::function<var(var &, const var &, const var &, const var &)>;  // e.g., .slice(start, end, step)

struct MethodTable {
    std::unordered_map<std::string, Method0> m0;
    std::unordered_map<std::string, Method1> m1;
    std::unordered_map<std::string, Method2> m2;
    std::unordered_map<std::string, Method3> m3;
};
```

#### The Dispatch Function

```cpp
var dispatch_method(var &self, const std::string &method, const std::vector<var> &args);
```

This function:

1. Checks if the method is in the **universal** table (works on all types)
2. Checks the **type-specific** table based on `self.type()` (string, list, set, dict, file)
3. Throws an error if not found

#### Method Categories

**Universal methods** (available on ALL types):

| Method              | Args | Description                       |
| ------------------- | ---- | --------------------------------- |
| `type()`            | 0    | Returns the type name as a string |
| `str()`             | 0    | String representation             |
| `pretty_str()`      | 0    | Pretty-printed string             |
| `len()`             | 0    | Length / size                     |
| `hash()`            | 0    | Hash value                        |
| `is_none()`         | 0    | Type check — is this None?        |
| `is_bool()`         | 0    | Type check — is this a boolean?   |
| `is_int()`          | 0    | Type check — is this an int?      |
| `is_string()`       | 0    | Type check — is this a string?    |
| `is_list()`         | 0    | Type check — is this a list?      |
| `is_dict()`         | 0    | Type check — is this a dict?      |
| `is_set()`          | 0    | Type check — is this a set?       |
| `is_any_numeric()`  | 0    | Is it any numeric type?           |
| `is_any_integral()` | 0    | Is it any integer type?           |
| `is_any_floating()` | 0    | Is it any float type?             |
| `toInt()`           | 0    | Convert to int                    |
| `toDouble()`        | 0    | Convert to double                 |
| `toFloat()`         | 0    | Convert to float                  |
| `toLong()`          | 0    | Convert to long                   |
| `toLongLong()`      | 0    | Convert to long long              |

**String methods:**

| Method             | Args | Example                    | Result          |
| ------------------ | ---- | -------------------------- | --------------- |
| `upper()`          | 0    | `"hello".upper()`          | `"HELLO"`       |
| `lower()`          | 0    | `"HELLO".lower()`          | `"hello"`       |
| `strip()`          | 0    | `" hi ".strip()`           | `"hi"`          |
| `lstrip()`         | 0    | `" hi ".lstrip()`          | `"hi "`         |
| `rstrip()`         | 0    | `" hi ".rstrip()`          | `" hi"`         |
| `split(sep)`       | 1    | `"a,b,c".split(",")`       | `["a","b","c"]` |
| `join(list)`       | 1    | `",".join(["a","b"])`      | `"a,b"`         |
| `find(sub)`        | 1    | `"hello".find("ll")`       | `2`             |
| `rfind(sub)`       | 1    | `"hello".rfind("l")`       | `3`             |
| `replace(old,new)` | 2    | `"hi".replace("i","ey")`   | `"hey"`         |
| `contains(sub)`    | 1    | `"hello".contains("ell")`  | `True`          |
| `startswith(s)`    | 1    | `"hello".startswith("he")` | `True`          |
| `endswith(s)`      | 1    | `"hello".endswith("lo")`   | `True`          |
| `count(sub)`       | 1    | `"aaa".count("a")`         | `3`             |
| `repeat(n)`        | 1    | `"ab".repeat(3)`           | `"ababab"`      |
| `center(w)`        | 1    | `"hi".center(6)`           | `"  hi  "`      |
| `ljust(w)`         | 1    | `"hi".ljust(6)`            | `"hi    "`      |
| `rjust(w)`         | 1    | `"hi".rjust(6)`            | `"    hi"`      |
| `zfill(w)`         | 1    | `"42".zfill(5)`            | `"00042"`       |
| `title()`          | 0    | `"hello world".title()`    | `"Hello World"` |
| `capitalize()`     | 0    | `"hello".capitalize()`     | `"Hello"`       |
| `swapcase()`       | 0    | `"Hello".swapcase()`       | `"hELLO"`       |
| `isdigit()`        | 0    | `"123".isdigit()`          | `True`          |
| `isalpha()`        | 0    | `"abc".isalpha()`          | `True`          |
| `isalnum()`        | 0    | `"abc123".isalnum()`       | `True`          |
| `isspace()`        | 0    | `"  ".isspace()`           | `True`          |
| `islower()`        | 0    | `"abc".islower()`          | `True`          |
| `isupper()`        | 0    | `"ABC".isupper()`          | `True`          |
| `reverse()`        | 0    | `"hello".reverse()`        | `"olleh"`       |
| `slice(s,e)`       | 2    | `"hello".slice(1,3)`       | `"el"`          |

**List methods:**

| Method             | Args | Description                           |
| ------------------ | ---- | ------------------------------------- |
| `append(x)`        | 1    | Add element to end                    |
| `pop()`            | 0    | Remove and return last element        |
| `insert(i, x)`     | 2    | Insert element at index               |
| `remove(x)`        | 1    | Remove first occurrence               |
| `clear()`          | 0    | Remove all elements                   |
| `sort()`           | 0    | Sort in place                         |
| `reverse()`        | 0    | Reverse in place                      |
| `contains(x)`      | 1    | Check membership                      |
| `index(x)`         | 1    | Find index of element                 |
| `count(x)`         | 1    | Count occurrences                     |
| `slice(start,end)` | 2    | Extract sub-list                      |
| `join(sep)`        | 1    | Join elements into string             |
| `copy()`           | 0    | Shallow copy                          |
| `extend(other)`    | 1    | Append all elements from another list |

**Set methods:**

| Method                    | Args | Description                            |
| ------------------------- | ---- | -------------------------------------- |
| `add(x)`                  | 1    | Insert element                         |
| `remove(x)`               | 1    | Remove element (error if not found)    |
| `discard(x)`              | 1    | Remove element (no error if not found) |
| `contains(x)`             | 1    | Check membership                       |
| `union_with(s)`           | 1    | Union of two sets                      |
| `intersection(s)`         | 1    | Intersection of two sets               |
| `difference(s)`           | 1    | Elements in self but not in s          |
| `symmetric_difference(s)` | 1    | Elements in either but not both        |
| `is_subset(s)`            | 1    | Is self a subset of s?                 |
| `is_superset(s)`          | 1    | Is self a superset of s?               |

**Dict methods:**

| Method          | Args | Description                         |
| --------------- | ---- | ----------------------------------- |
| `keys()`        | 0    | List of all keys                    |
| `values()`      | 0    | List of all values                  |
| `items()`       | 0    | List of [key, value] pairs          |
| `contains(k)`   | 1    | Does key exist?                     |
| `get(k)`        | 1    | Get value for key (None if missing) |
| `pop(k)`        | 1    | Remove and return value for key     |
| `clear()`       | 0    | Remove all entries                  |
| `update(other)` | 1    | Merge other dict into this one      |
| `merge(other)`  | 1    | Same as update                      |
| `size()`        | 0    | Number of entries                   |

**File methods:**

| Method         | Args | Description                 |
| -------------- | ---- | --------------------------- |
| `read()`       | 0    | Read entire file contents   |
| `readLine()`   | 0    | Read one line               |
| `readLines()`  | 0    | Read all lines as a list    |
| `write(s)`     | 1    | Write string to file        |
| `writeLine(s)` | 1    | Write string + newline      |
| `close()`      | 0    | Close the file              |
| `isOpen()`     | 0    | Check if file is still open |

---

### 4.4 `scriptit_builtins.hpp` — Built-in Functions

**Purpose:** Implements all free-standing functions like `print(x)`, `len(x)`, `input()`, `open("file.txt", "r")`.

#### FileRegistry — Managing Open Files

Files in ScriptIt are represented as `Dict` objects with special keys:

```
{ "__type__": "file", "__id__": 1, "path": "data.txt", "mode": "r" }
```

The `FileRegistry` singleton manages the actual `std::fstream` objects:

```cpp
struct FileRegistry {
    int open(path, mode);        // Open file, return handle ID
    std::fstream& get(id);       // Get stream by ID
    std::string getPath(id);     // Get file path by ID
    std::string getMode(id);     // Get open mode by ID
    void close(id);              // Close and remove
};
```

Supported file modes:
| Mode | Description |
|------|-------------|
| `"r"` | Read only |
| `"w"` | Write (truncate) |
| `"a"` | Append |
| `"rw"` or `"r+"` | Read and write |
| `"w+"` | Read and write (truncate) |
| `"a+"` | Read and append |

#### `dispatch_math()` — Math Function Router

Routes function names to `pythonic::math` implementations:

```cpp
var dispatch_math(const std::string &name, std::stack<var> &stk);
```

It pops the required number of arguments from the evaluation stack, calls the corresponding `pythonic::math` function, and pushes the result back.

Available math functions:

| Function   | Description    | Example             |
| ---------- | -------------- | ------------------- |
| `sin(x)`   | Sine (radians) | `sin(3.14159)` → ~0 |
| `cos(x)`   | Cosine         | `cos(0)` → 1        |
| `tan(x)`   | Tangent        | `tan(0.785)` → ~1   |
| `cot(x)`   | Cotangent      | `cot(0.785)` → ~1   |
| `sec(x)`   | Secant         | `sec(0)` → 1        |
| `csc(x)`   | Cosecant       | `csc(1.5708)` → ~1  |
| `asin(x)`  | Arc sine       | `asin(1)` → 1.5708  |
| `acos(x)`  | Arc cosine     | `acos(1)` → 0       |
| `atan(x)`  | Arc tangent    | `atan(1)` → 0.7854  |
| `log(x)`   | Natural log    | `log(e)` → 1        |
| `log2(x)`  | Log base 2     | `log2(8)` → 3       |
| `log10(x)` | Log base 10    | `log10(100)` → 2    |
| `sqrt(x)`  | Square root    | `sqrt(16)` → 4      |
| `abs(x)`   | Absolute value | `abs(-5)` → 5       |
| `ceil(x)`  | Ceiling        | `ceil(2.3)` → 3     |
| `floor(x)` | Floor          | `floor(2.9)` → 2    |
| `round(x)` | Round          | `round(2.5)` → 3    |
| `min(a,b)` | Minimum        | `min(3, 7)` → 3     |
| `max(a,b)` | Maximum        | `max(3, 7)` → 7     |

#### `get_builtins()` — The Builtin Function Map

Returns a `std::unordered_map<std::string, BuiltinFn>` mapping names to implementations. Each builtin operates directly on the evaluation stack — it pops its arguments and pushes its result.

**Builtin function categories:**

| Category         | Functions                                                                                                                                                       |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| I/O              | `print`, `pprint`, `read`, `write`, `readLine`, `input`                                                                                                         |
| Type/Conversion  | `len`, `type`, `str`, `int`, `float`, `double`, `bool`, `repr`, `isinstance`, `long`, `long_long`, `long_double`, `uint`, `ulong`, `ulong_long`, `auto_numeric` |
| Containers       | `append`, `pop`, `list`, `set`, `dict`, `range_list`                                                                                                            |
| Functional       | `sum`, `sorted`, `reversed`, `all`, `any`, `enumerate`, `zip`, `map`                                                                                            |
| Math (free-form) | `abs`, `min`, `max`                                                                                                                                             |
| File I/O         | `open`, `close`                                                                                                                                                 |

---

### 4.5 `json_and_kernel.hpp` — Notebook Integration

**Purpose:** Allows ScriptIt to run as a **Jupyter-compatible kernel** that reads JSON commands from stdin and returns JSON responses.

#### JSON Helpers

```cpp
// Parse a simple JSON object: {"key": "value", ...}
std::unordered_map<std::string, std::string> parse_json_object(const std::string &json);

// Escape special characters for JSON strings
std::string json_escape(const std::string &s);

// Build a standard JSON response
std::string make_json_response(cellId, status, stdout, stderr, result, execCount);
```

The JSON parser is intentionally minimal — it only parses flat string→string maps. It handles escape sequences (`\n`, `\t`, `\\`, `\"`, `\/`) and non-string values (read as raw text).

#### `runKernel()` — The Kernel Loop

```
1. Create a persistent globalScope (survives between cell executions)
2. Print {"status": "kernel_ready", "version": "2.0"}
3. Loop forever, reading JSON lines from stdin:
   a. "execute" → tokenize+parse+execute the code, capture stdout,
                   return JSON with output/errors
   b. "reset"   → clear the globalScope, start fresh
   c. "shutdown" → exit the loop
   d. "complete" → return variable names matching a prefix (tab-completion)
```

**stdout capture trick:** The kernel temporarily replaces `std::cout`'s stream buffer with a `std::ostringstream` so that any `print()` calls during execution are captured into a string, then returned in the JSON response:

```cpp
// Before execution:
std::ostringstream capturedOut;
std::streambuf *oldBuf = std::cout.rdbuf(capturedOut.rdbuf());

// ... execute code (any print() goes to capturedOut) ...

// After execution:
std::cout.rdbuf(oldBuf);  // Restore stdout
std::string stdoutStr = capturedOut.str();  // Get captured output
```

---

### 4.6 `ScriptIt.cpp` — The Entry Point

This ~165-line file is the glue that ties everything together.

#### `executeScript(content)`

The core helper function that runs the full pipeline:

```cpp
void executeScript(const std::string &content) {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize(content);    // Stage 1: Tokenize
    Parser parser(tokens);
    auto program = parser.parseProgram();          // Stage 2: Parse
    Scope globalScope;
    globalScope.define("PI", var(3.14159265));     // Pre-defined constants
    globalScope.define("e", var(2.7182818));
    for (auto &stmt : program->statements)
        stmt->execute(globalScope);                // Stage 3: Execute
}
```

Note: `ReturnException` at the top level is caught and its value is printed. Other exceptions are caught and displayed as `"Error: ..."`.

#### Command-Line Modes

```bash
scriptit --kernel          # Notebook kernel mode (JSON stdin/stdout)
scriptit --test            # Run built-in test suite
scriptit --script          # Read code from stdin (piped input)
scriptit myfile.sit        # Execute a .sit script file
scriptit                   # Interactive REPL
```

#### The REPL (Read-Eval-Print Loop)

```
ScriptIt REPL v2 (powered by pythonic::var)
Type 'exit' to quit, 'clear' to clear screen, 'wipe' for fresh start.
>> var x = 10.
>> x + 5.
15
>> exit
```

Special REPL commands:

- `exit` — quit
- `clear` — clear the terminal screen
- `wipe` — clear screen AND reset all variables/functions (calls `globalScope.clear()`)

The REPL maintains a **persistent** `globalScope` across lines, so variables defined on one line are available on the next. It also pre-defines `PI`, `e`, and `ans`.

---

## 5. The Language — Syntax Reference

### Statement Termination

The **dot** (`.`) terminates statements, like a semicolon in C:

```
var x = 10.
print(x).
```

But dots are **optional** in many cases:

- At end of file / end of input
- Before block terminators (`;`, `elif`, `else`)
- When a **newline** separates statements

So this also works:

```
var x = 10
print(x)
```

### Blocks

Blocks start with `:` and end with `;`:

```
if x > 5:
    print("big")
;
```

### Comments

Comments use arrow syntax and can span multiple lines:

```
--> This is a comment <--

-->
   This is a
   multi-line comment
<--
```

### Line Continuation

Use a backtick (`` ` ``) before a newline to continue a statement on the next line:

```
var result = very_long_expression + `
             another_part.
```

The backtick tells the tokenizer to suppress the newline token, so the two physical lines are treated as one logical line.

### Variable Declarations

```
var x = 10.              --> declare x with value 10
var y.                    --> declare y as None
var a = 1 b = 2 c = 3.   --> multi-variable declaration (no comma needed)
var p, q = 5, r = 10.    --> with commas also works
let name be "Alice".      --> alternative syntax (always a declaration)
```

### Assignment

```
x = 5.                   --> simple assignment (x must already exist in scope)
x += 1.                  --> compound: x = x + 1
x -= 1.                  --> compound: x = x - 1
x *= 2.                  --> compound: x = x * 2
x /= 2.                  --> compound: x = x / 2
x %= 3.                  --> compound: x = x % 3
x++.                     --> post-increment: x = x + 1
x--.                     --> post-decrement: x = x - 1
++x.                     --> pre-increment: x = x + 1
--x.                     --> pre-decrement: x = x - 1
```

**How compound assignment works internally:** The parser rewrites `x += expr` into an `AssignStmt` with an expression that means `x + expr`. For example, `x += 3` becomes:

- `AssignStmt { name="x", expr=Expression{ rpn: [Identifier "x"] [Number "3"] [Operator "+"] } }`

Increment/decrement works similarly: `x++` becomes `x = x + 1`.

### Functions

```
--> Definition <--
fn greet(name):
    print("Hello, " + name)
;

--> Call <--
greet("Alice").

--> Return values with give <--
fn add(a, b):
    give a + b.
;
var result = add(3, 4).   --> result = 7

--> Parentheses around give's expression are optional <--
fn square(x):
    give(x * x).          --> parentheses for grouping, same as give x * x.
;

--> Forward declaration (promise the function will exist later) <--
fn helper(x).

--> Pass-by-reference with @ prefix <--
fn swap(@a, @b):
    var temp = a.
    a = b.
    b = temp.
;
var x = 1 y = 2.
swap(x, y).    --> now x=2, y=1

--> Overloading by arity <--
fn add(a, b):
    give a + b.
;
fn add(a, b, c):
    give a + b + c.
;
add(1, 2).      --> calls add/2 → 3
add(1, 2, 3).   --> calls add/3 → 6

--> Functions without give return None <--
fn sayHi():
    print("Hi!")
;
var result = sayHi().    --> prints "Hi!", result is None
```

### Conditionals

```
if score >= 90:
    print("A")
elif score >= 80:
    print("B")
elif score >= 70:
    print("C")
else:
    print("F")
;
```

### Loops

```
--> Range loop: 0 to N-1 (N is exclusive here, but the actual behavior is inclusive) <--
for i in range(5):
    print(i)
;
--> Prints: 0 1 2 3 4 5  (range goes from 0 up to the value)

--> Range with from/to <--
for i in range(from 1 to 5):
    print(i)
;
--> Prints: 1 2 3 4 5

--> Range with step <--
for i in range(from 0 to 10 step 2):
    print(i)
;
--> Prints: 0 2 4 6 8 10

--> For-in loop (iterate over a collection) <--
var names = ["Alice", "Bob", "Charlie"].
for name in names:
    print(name)
;

--> While loop <--
var count = 0.
while count < 5:
    print(count)
    count++
;
```

### The `of` Keyword — Postfix Method Syntax

Instead of `name.upper()`, you can write:

```
upper() of name          --> same as name.upper()
split(",") of csv_line   --> same as csv_line.split(",")
```

**How it works internally:** The parser sees `upper()` first, building an expression for a zero-arg function call. Then it sees `of`, parses the target expression, and **rewrites** the RPN so that the target becomes the object and the function call becomes a method call (At token).

### Implicit Multiplication

ScriptIt supports math-style implicit multiplication:

```
var x = 5.
var result = 3x.       --> 15 (same as 3 * x)
var area = 2(3 + 4).   --> 14 (same as 2 * (3 + 4))
```

The parser detects this when two "value" tokens are adjacent without an operator between them:

- Number followed by Identifier: `3x`
- Number followed by LeftParen: `2(3+4)`
- RightParen followed by LeftParen: `(a)(b)`
- Identifier followed by LeftParen (when not a function call): context-dependent

### Comparison Operators

```
x == y       --> equality (with tolerance 1e-9 for numbers)
x != y       --> inequality
x < y        --> less than
x > y        --> greater than
x <= y       --> less than or equal
x >= y       --> greater than or equal
x is y       --> value equality (same as ==)
x is not y   --> value inequality (same as !=)
x points y   --> strict identity: same type AND exact value (no tolerance)
x not points y --> strict non-identity
```

The difference between `==`/`is` and `points`:

- `==` and `is` use a tolerance of 1e-9 for floating-point numbers and allow cross-type comparison
- `points` requires the same type AND exact value match (no tolerance). If types differ, it's always `False`.

### Logical Operators

```
x and y      --> logical AND (short-circuit)
x or y       --> logical OR (short-circuit)
not x        --> logical NOT
```

These are translated to `&&`, `||`, `!` by the tokenizer. Short-circuit means: `False and expensive()` never calls `expensive()`, and `True or expensive()` never calls `expensive()`.

### List, Set, and Dict Literals

```
--> Lists use square brackets <--
var nums = [1, 2, 3, 4, 5].
var empty = [].

--> Sets use curly braces <--
var unique = {1, 2, 3}.

--> Dicts via the dict() builtin <--
var person = dict().
```

### Context Manager (File I/O)

```
let f be open("data.txt", "w"):
    write(f, "Hello, World!")
    write(f, "Second line")
;
--> File is automatically closed when the block ends, even if an error occurs
```

---

## 6. How the Pieces Connect — The Full Pipeline

Let's trace exactly what happens when you run this program:

```
var name = "Alice".
fn greet(who):
    give "Hello, " + who.
;
print(greet(name)).
```

### Stage 1: Tokenization

The `Tokenizer::tokenize()` method scans the source string character by character:

```
Input: 'v','a','r',' ','n','a','m','e',' ','=',' ','"','A','l','i','c','e','"','.',...

Output tokens:
  [KeywordVar, "var",  pos=0,  line=1]
  [Identifier, "name", pos=4,  line=1]
  [Equals,     "=",    pos=9,  line=1]
  [String,     "Alice",pos=11, line=1]
  [Dot,        ".",    pos=18, line=1]
  [Newline,    "\n",   pos=19, line=1]
  [KeywordFn,  "fn",   pos=20, line=2]
  [Identifier, "greet",pos=23, line=2]
  [LeftParen,  "(",    pos=28, line=2]
  [Identifier, "who",  pos=29, line=2]
  [RightParen, ")",    pos=32, line=2]
  [Colon,      ":",    pos=33, line=2]
  [Newline,    "\n",   pos=34, line=2]
  [KeywordGive,"give", pos=39, line=3]
  [String,     "Hello, ",pos=44, line=3]
  [Operator,   "+",    pos=53, line=3]
  [Identifier, "who",  pos=55, line=3]
  [Dot,        ".",    pos=58, line=3]
  [Newline,    "\n",   pos=59, line=3]
  [Semicolon,  ";",    pos=60, line=4]
  [Newline,    "\n",   pos=61, line=4]
  [Identifier, "print",pos=62, line=5]
  [LeftParen,  "(",    pos=67, line=5]
  [Identifier, "greet",pos=68, line=5]
  [LeftParen,  "(",    pos=73, line=5]
  [Identifier, "name", pos=74, line=5]
  [RightParen, ")",    pos=78, line=5]
  [RightParen, ")",    pos=79, line=5]
  [Dot,        ".",    pos=80, line=5]
  [Eof,        "",     pos=-1, line=5]
```

### Stage 2: Parsing

The `Parser::parseProgram()` method reads tokens and builds an AST:

```
BlockStmt
├── AssignStmt { name="name", isDeclaration=true }
│   └── Expression { rpn: [String "Alice"] }
│
├── FunctionDefStmt { name="greet", params=["who"] }
│   └── BlockStmt (body)
│       └── ReturnStmt
│           └── Expression { rpn: [String "Hello, "] [Identifier "who"] [Operator "+"] }
│
└── ExprStmt
    └── Expression { rpn: [Identifier "name"] [KeywordFn "greet"/1] [KeywordFn "print"/1] }
```

Notice how the parser converts the expression `print(greet(name))` into **Reverse Polish Notation** (RPN): first the argument `name`, then the inner function call `greet` (with 1 arg), then the outer function call `print` (with 1 arg).

### Stage 3: Execution (Tree Walking)

The executor walks the AST top-down:

**Step 3a — Two-Pass Hoisting:**
`BlockStmt::execute()` first scans all children for `FunctionDefStmt` nodes and registers them in the scope. This means `greet` is available even before line 1 executes. Then it executes all statements in order.

**Step 3b — Execute `AssignStmt` for `name`:**

1. Evaluate the expression: rpn = `[String "Alice"]` → push `var("Alice")` → result = `var("Alice")`
2. Since `isDeclaration = true`, call `scope.define("name", var("Alice"))`

**Step 3c — Execute `FunctionDefStmt` for `greet`:**
Already registered during hoisting pass — this is a no-op on the second pass.

**Step 3d — Execute `ExprStmt` for `print(greet(name))`:**
Evaluate the expression using the RPN stack machine:

```
RPN: [Identifier "name"] [KeywordFn "greet"/1] [KeywordFn "print"/1]

Stack: []

1. Process [Identifier "name"]
   → scope.get("name") → var("Alice")
   → Push var("Alice")
   Stack: ["Alice"]

2. Process [KeywordFn "greet"/1]  (user function, 1 arg)
   → Pop 1 arg: var("Alice")
   → Look up scope.getFunction("greet", 1) → found
   → Create child scope (barrier=true): { "who" → var("Alice") }
   → Execute function body:
      → ReturnStmt → evaluate "Hello, " + who
        → rpn: [String "Hello, "] [Identifier "who"] [Operator "+"]
        → Push "Hello, ", push "Alice" (from scope.get("who")), pop both,
          string + string → concatenate → "Hello, Alice"
      → Throw ReturnException(var("Hello, Alice"))
   → Catch ReturnException → push var("Hello, Alice")
   Stack: ["Hello, Alice"]

3. Process [KeywordFn "print"/1]  (builtin, 1 arg)
   → Pop 1 arg: var("Hello, Alice")
   → Built-in "print" → std::cout << "Hello, Alice" << std::endl
   → Push var(NoneType{}) (print returns None)
   Stack: [None]
```

**Output:** `Hello, Alice`

Since `ExprStmt` checks `!val.is_none()` before printing, and the result is None, nothing additional is printed.

---

## 7. Deep Dive: The Tokenizer

The tokenizer lives in `class Tokenizer` inside `perser.hpp`. It's a single method: `std::vector<Token> tokenize(const std::string &source)`.

### The Scanning Loop

```cpp
for (size_t i = 0; i < source.length(); ++i) {
    char c = source[i];
    // Handle each character type...
}
```

The tokenizer processes characters in this priority order:

### 1. Line Continuation (Backtick)

```cpp
if (c == '`') {
    // Skip whitespace until newline
    // If newline found, skip it (continue on next line)
    // Otherwise, ignore stray backtick
}
```

The backtick says "the next newline doesn't count." This lets you split long lines:

```
var x = a + b + `
        c + d.
```

Without the backtick, the newline would terminate the statement after `b +`.

### 2. Newlines

```cpp
if (c == '\n') {
    tokens.emplace_back(TokenType::Newline, "\\n", i, line);
    line++;
}
```

Newlines are **emitted as tokens** because they serve as implicit statement terminators (so you don't always need a dot). The `match()` and `consume()` methods in the parser skip over Newline tokens when looking for other tokens, but `consumeDotOrForgive()` recognizes them as valid statement endings.

### 3. Whitespace (spaces, tabs)

Simply skipped — `if (std::isspace(c)) continue;`

### 4. Comments: `-->` ... `<--`

```cpp
if (c == '-' && source[i+1] == '-' && source[i+2] == '>') {
    // Skip everything until we find '<--'
    // Track line numbers for multi-line comments
}
```

The entire comment content is discarded. No tokens are generated.

### 5. String Literals

```cpp
if (c == '"' || c == '\'') {
    // Read until matching quote
    // Handle escape sequences: \n, \t, \\, \", \'
    // Error on unterminated string
}
```

Both single and double quotes work. Strings can contain newlines (the line counter tracks them). Escape sequences produce the actual character (not the two-character sequence). An unterminated string (reaching end of input without closing quote) throws an error.

### 6. Numbers

```cpp
if (std::isdigit(c) || (c == '.' && next_is_digit)) {
    // Read digits and at most one decimal point
}
```

Important rules:

- `.5` is valid (leading-dot decimal) — produces `var(0.5)`
- `42` is an integer — produces `var(42)`
- `42.5` is a decimal — produces `var(42.5)`
- `42.` → the `.` is NOT part of the number (it's the statement terminator). The tokenizer stops at the `.` because the next character after `.` is not a digit.
- If an integer literal is too large for `int`, the evaluator will catch the exception and parse it as `long long` instead.

### 7. Identifiers and Keywords

```cpp
if (std::isalpha(c) || c == '_') {
    // Read alphanumeric + underscore characters
    // Check for multi-word types
    // Check keyword map
    // Check logical aliases
}
```

**Multi-word type handling:** When the tokenizer sees `long`, it peeks ahead to check if the next word is `double` or `long`. If so, it merges them into a single identifier:

- `long double` → `Identifier "long_double"`
- `long long` → `Identifier "long_long"`
- `unsigned int` → `Identifier "uint"`
- `unsigned long` → `Identifier "ulong"`
- `unsigned long long` → `Identifier "ulong_long"`

**Keyword map:** These words become their own token types:

```
var → KeywordVar    fn → KeywordFn      give → KeywordGive
if → KeywordIf      elif → KeywordElif  else → KeywordElse
for → KeywordFor    in → KeywordIn      range → KeywordRange
from → KeywordFrom  to → KeywordTo      step → KeywordStep
pass → KeywordPass  while → KeywordWhile are → KeywordAre
new → KeywordNew    let → KeywordLet    be → KeywordBe
of → KeywordOf      is → KeywordIs      points → KeywordPoints
```

**Logical aliases:** These words are translated to operator tokens:

- `and` → `Operator "&&"`
- `or` → `Operator "||"`
- `not` → `Operator "!"`

### 8. Multi-Character Operators

Checked **before** single-character operators to avoid mismatches:

| Characters | Token             | Note                                            |
| ---------- | ----------------- | ----------------------------------------------- |
| `+=`       | PlusEquals        |                                                 |
| `++`       | PlusPlus          |                                                 |
| `--`       | MinusMinus        | NOT if followed by `>` (that's a comment `-->`) |
| `-=`       | MinusEquals       |                                                 |
| `*=`       | StarEquals        |                                                 |
| `/=`       | SlashEquals       |                                                 |
| `%=`       | PercentEquals     |                                                 |
| `==`       | Operator `"=="`   |                                                 |
| `!=`       | Operator `"!="`   |                                                 |
| `<=`       | Operator `"<="`   |                                                 |
| `>=`       | Operator `">="`   |                                                 |
| `&&`       | Operator `"&&"`   |                                                 |
| `\|\|`     | Operator `"\|\|"` |                                                 |

### 9. Single-Character Symbols

Looked up from a map:

```
+  *  /  ^  %  → Operator
(  )  {  }  [  ]  → LeftParen/RightParen/LeftBrace/RightBrace/LeftBracket/RightBracket
,  .  :  ;  @  → Comma/Dot/Colon/Semicolon/At
```

Remaining single characters handled as fallthrough: `-` → Operator, `=` → Equals, `!` → Operator, `<` → Operator, `>` → Operator.

Any unrecognized character throws: `"Unexpected character '?' at line N"`.

---

## 8. Deep Dive: The Parser

The parser in `class Parser` uses **recursive descent** — each grammar rule is a method that calls other methods for sub-rules.

### Parser Infrastructure

```cpp
Token peek();        // Look at current token without consuming
Token peekNext();    // Look at next meaningful token (skips newlines)
Token advance();     // Consume and return current token
bool check(type);    // Is current token of this type?
bool match(type);    // If current token matches, consume it and return true
Token consume(type, error);  // Consume expected token or throw error
```

**Important:** `match()` and `consume()` automatically **skip Newline tokens** before checking the expected type (unless you're specifically matching for Newline). This is what makes newlines transparent to most grammar rules while still allowing them as statement terminators.

`peekNext()` also skips Newline tokens to find the next meaningful token. This is used for lookahead decisions like "is this an assignment?" (checking if the token after the identifier is `=`).

### `parseProgram()` — The Entry Point

```cpp
std::shared_ptr<BlockStmt> parseProgram() {
    auto block = std::make_shared<BlockStmt>();
    while (!isAtEnd()) {
        while (check(TokenType::Newline)) advance();  // Skip blank lines
        if (isAtEnd()) break;
        block->statements.push_back(parseStatement());
    }
    return block;
}
```

### `parseStatement()` — The Dispatcher

This is a big method that looks at the current token and decides which kind of statement to parse. The order matters — it checks for keywords first, then compound assignments, then simple assignments, then falls through to expression statements.

Here's the dispatch order:

1. **`if`** → `parseIf()`
2. **`for`** → `parseFor()`
3. **`while`** → `parseWhile()`
4. **`fn`** → `parseFunction()`
5. **`give`** → `parseReturn()`
6. **`pass`** → `parsePass()`
7. **`let`** → parses `let NAME be EXPR.` or `let NAME be EXPR: BLOCK ;`
8. **`var`** → parses variable declaration(s)
9. **Identifier followed by `+=`, `-=`, `*=`, `/=`, `%=`** → compound assignment
10. **`++` or `--` followed by Identifier** → pre-increment/decrement
11. **Identifier followed by `=`** → simple assignment
12. **Identifier followed by `++` or `--`** → post-increment/decrement
13. **Anything else** → expression statement

### `parseIf()`

```
Grammar: if EXPR : BLOCK (elif EXPR : BLOCK)* (else : BLOCK)? ;
```

Each `if`/`elif` becomes a `Branch` with a condition expression and a body block. The `else` block is optional. The entire structure ends with `;`.

### `parseFor()` — Three Forms

```
Form 1: for IDENT in range(EXPR):                    BLOCK ;  → ForStmt (0 to N)
Form 2: for IDENT in range(from EXPR to EXPR):        BLOCK ;  → ForStmt (A to B)
Form 3: for IDENT in range(from EXPR to EXPR step EXPR): BLOCK ;  → ForStmt (A to B, step S)
Form 4: for IDENT in EXPR:                           BLOCK ;  → ForInStmt (iterate collection)
```

The parser checks if the token after `in` is `range` to distinguish range loops from for-in loops. For `range(N)` (Form 1), a literal `0` is synthesized as the start expression.

### `parseFunction()` — Function Definitions

```
Grammar: fn NAME ( PARAMS ) : BLOCK ;    → full definition
         fn NAME ( PARAMS ) .            → forward declaration
```

Parameters:

- Each parameter is an identifier
- The `@` prefix marks pass-by-reference: `fn swap(@a, @b)`
- Duplicate parameter names are detected at parse time and cause an error
- Parameters are separated by commas

The parser distinguishes forward declarations from full definitions by what follows the `)`:

- If `.` or `Newline` or `Eof` → forward declaration (`body = nullptr`)
- If `:` → full definition (parse body block until `;`)
- Empty bodies throw an error ("use `pass`")

### `parseReturn()` — The `give` Statement

```
give expr.       → return the value of expr
give(expr).      → same thing (parens are for grouping, not required)
give().          → return None (if expression is empty)
```

The parser calls `parseExpression()` after `give`. If the expression turns out empty (no tokens), it substitutes a `None` literal.

### `parsePrimaryExpr()` — The Shunting-Yard Algorithm

This is the most complex part of the parser. It converts **infix** expressions (like `3 + 4 * 2`) into **Reverse Polish Notation** (like `3 4 2 * +`) using Dijkstra's Shunting-Yard algorithm.

#### How Shunting-Yard Works (The Basics)

Imagine two lanes: an **output queue** and an **operator stack**.

**Rules:**

1. Numbers/identifiers/strings → go directly to the output queue
2. Operators → compare precedence with the top of the operator stack:
   - If the stack operator has **higher or equal** precedence, pop it to output first
   - Then push the new operator onto the stack
3. `(` → push onto operator stack
4. `)` → pop operators to output until `(` is found, then discard the `(`
5. End of input → pop all remaining operators to output

**Example: `3 + 4 * 2`**

| Step | Token | Action           | Output Queue | Op Stack |
| ---- | ----- | ---------------- | ------------ | -------- |
| 1    | `3`   | To output        | `3`          |          |
| 2    | `+`   | Push (prec 5)    | `3`          | `+`      |
| 3    | `4`   | To output        | `3 4`        | `+`      |
| 4    | `*`   | prec 6 > 5, push | `3 4`        | `+ *`    |
| 5    | `2`   | To output        | `3 4 2`      | `+ *`    |
| 6    | End   | Flush            | `3 4 2 * +`  |          |

**Result RPN:** `3 4 2 * +` → evaluates as `3 + (4*2) = 11` ✓

#### ScriptIt's Extensions to Shunting-Yard

The standard algorithm only handles numbers and operators. ScriptIt extends it with:

**Expression terminators:** The Shunting-Yard loop breaks when it encounters tokens that signal the end of an expression: `:`, `;`, `in`, `to`, `step`, `elif`, `else`, `be`, `=`, `Newline`, `of`, `+=`, `-=`, etc.

**Function calls:** When the parser sees `Identifier(`, it recursively parses the arguments (each is a sub-expression via `parseExpression()`), then pushes a special token `[KeywordFn, "funcName", argCount]` to the output. The `argCount` is stored in the token's `position` field (reusing it since position is irrelevant in RPN).

**Method calls:** When the parser sees `.identifier(` (with no gap between the dot and identifier), it's a method call. It parses arguments and pushes `[At, "methodName", argCount]` to the output. At evaluation time, the At token pops `argCount` arguments AND one more value (the object) from the stack.

**List literals:** `[1, 2, 3]` → parses each element as a sub-expression, pushes them all, then pushes `[LeftBracket, "LIST", 3]` which tells the evaluator to pop 3 values and make a list.

**Set literals:** `{1, 2, 3}` → same pattern with `[LeftBrace, "SET", 3]`.

**Unary operators:** `-x` becomes `~x` internally (tilde replaces minus to distinguish from binary minus). `!x` stays as `!`. The parser detects unary context by checking the previous token type: a `-` or `!` is unary if the previous token was `Eof` (start), `(`, `,`, another Operator, `=`, `:`, `if`, `elif`, or `give`.

**Implicit multiplication:** The parser detects adjacent value tokens and inserts a `*` operator. Triggers when:

- A Number, Identifier, or LeftParen appears, AND
- The previous token was a Number, RightParen, Identifier, or RightBracket

**"is not" compound operator:** When the parser sees `KeywordIs` followed by `Operator "!"` (which was `not`), it combines them into the single operator `"is not"` with precedence 3.

**"not points" compound operator:** When `Operator "!"` is followed by `KeywordPoints`, they combine into `"not points"` with precedence 3.

**Comma and RightParen handling:** These only terminate the expression if there's no open `(` on the operator stack. This allows commas to separate function arguments without prematurely ending the expression.

**`of` keyword:** After `parsePrimaryExpr()` completes, the caller (`parseExpression()`) checks for `of`. If found, it parses the target and rewrites the RPN to transform `func(args) of target` into `target.func(args)`.

### `consumeDotOrForgive()` — The Forgiving Terminator

This is one of ScriptIt's most user-friendly features. It makes the dot statement terminator optional in many contexts:

```cpp
Token consumeDotOrForgive() {
    // Skip any newline tokens first
    while (check(TokenType::Newline)) advance();

    if (check(TokenType::Dot))
        return advance();                        // Explicit dot — consume it

    if (isAtEnd())
        return Token(Dot, ".", -1, ...);         // End of input — forgive

    if (check(Semicolon) || check(KeywordElif) || check(KeywordElse))
        return Token(Dot, ".", -1, ...);         // Block terminator — forgive

    if (peek().line > lastConsumedLine)
        return Token(Dot, ".", -1, ...);         // Different line — forgive

    throw std::runtime_error("Expected '.'");    // Same line, no dot — error
}
```

This means you MUST use a dot (or newline) if two statements are on the same line:

```
var x = 5. var y = 10.    --> dots required (same line)
var x = 5                  --> no dot needed (newline separates)
var y = 10
```

### Logical Expression Parsing

Logical `&&` and `||` are parsed separately from the Shunting-Yard algorithm to enable **short-circuit evaluation**. The parser uses a recursive descent approach:

```
parseExpression()   → parseLogicalOr() → checks for 'of' rewriting
parseLogicalOr()    → parseLogicalAnd() (|| parseLogicalAnd())*
parseLogicalAnd()   → parsePrimaryExpr() (&& parsePrimaryExpr())*
```

When `||` or `&&` is found, instead of putting it into the RPN, the parser creates an `Expression` node with `logicalOp` set and `lhs`/`rhs` pointing to the sub-expressions. This tree structure enables the evaluator to short-circuit.

---

## 9. Deep Dive: The Evaluator

`Expression::evaluate(Scope &scope)` is the stack-based RPN evaluator. It's the code that actually computes values.

### Short-Circuit Logic

Before processing RPN tokens, the evaluator checks for short-circuit `&&`/`||`:

```cpp
if (!logicalOp.empty() && lhs && rhs) {
    var leftVal = lhs->evaluate(scope);
    if (logicalOp == "&&") {
        if (!bool(leftVal)) return var(false);   // false && anything = false
        return var(bool(rhs->evaluate(scope)));
    } else {  // ||
        if (bool(leftVal)) return var(true);     // true || anything = true
        return var(bool(rhs->evaluate(scope)));
    }
}
```

If the left side determines the result, the right side is **never evaluated**. This is important for expressions like `x != 0 and 10/x > 2` — if x is 0, the division is never attempted.

### The Two Stacks

```cpp
std::stack<var> stk;          // Value stack
std::stack<std::string> nameStk;  // Tracks variable names (for pass-by-ref)
```

The `nameStk` runs in parallel with `stk`. When a variable `x` is pushed, its name `"x"` is also pushed. When a literal `42` is pushed, an empty string `""` is pushed. This is needed for pass-by-reference: when calling `swap(@a, @b)` with `swap(x, y)`, the evaluator needs to know that the first argument came from variable `x` and the second from `y`, so it can write the modified values back after the function returns.

### Token Processing Rules

| Token Type           | Action                                                                                                                              |
| -------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| `Number`             | Parse string to `var`. If contains `.`, use `stod` → double. Otherwise try `stoi` → int, fallback `stoll` → long long. Push result. |
| `String`             | Push `var(string)`.                                                                                                                 |
| `Identifier "True"`  | Push `var(true)`.                                                                                                                   |
| `Identifier "False"` | Push `var(false)`.                                                                                                                  |
| `Identifier "None"`  | Push `var(NoneType{})`.                                                                                                             |
| `Identifier other`   | Push `scope.get(name)` + push name to nameStk.                                                                                      |
| `Operator "~"`       | Pop 1 value, negate it, push result. (Int stays int, others convert to double.)                                                     |
| `Operator "!"`       | Pop 1 value, apply logical not (`!bool(val)`), push `var(bool)`.                                                                    |
| `Operator (binary)`  | Pop 2 values (b then a), apply operation, push result.                                                                              |
| `LeftBracket "LIST"` | Pop N values (count in position field), reverse, make `List`, push `var(list)`.                                                     |
| `LeftBrace "SET"`    | Pop N values, make `Set`, push `var(set)`.                                                                                          |
| `At "method"`        | Pop N args + 1 object, call `dispatch_method()`, push result.                                                                       |
| `KeywordFn "func"`   | Pop N args, dispatch to math/builtin/user function, push result.                                                                    |

### Binary Operator Behaviors — Complete Reference

| Operator          | Numeric                            | String                  | List                        | None                | Mixed                                          |
| ----------------- | ---------------------------------- | ----------------------- | --------------------------- | ------------------- | ---------------------------------------------- |
| `+`               | `pythonic::math::add` with Promote | Concatenation           | Concatenation               | Error               | String + any → convert other to string, concat |
| `-`               | `pythonic::math::sub` with Promote | Error                   | Error                       | Error               | Error                                          |
| `*`               | `pythonic::math::mul` with Promote | `"ab" * 3 → "ababab"`   | `[1,2] * 3 → [1,2,1,2,1,2]` | Error               | String \* int = repetition                     |
| `/`               | `pythonic::math::div` with Promote | Error                   | Error                       | Error               | Divide by zero → error                         |
| `%`               | `pythonic::math::mod` with Promote | Error                   | Error                       | Error               | Modulo by zero → error                         |
| `^`               | `pythonic::math::pow` with Promote | Error                   | Error                       | Error               | —                                              |
| `==`              | Numeric compare (ε=1e-9)           | String compare          | Container compare           | None == None only   | —                                              |
| `!=`              | Numeric compare (ε=1e-9)           | String compare          | Container compare           | —                   | —                                              |
| `<` `>` `<=` `>=` | Numeric compare                    | Error                   | Error                       | Error               | Convert to double                              |
| `is`              | Same as `==`                       | Same as `==`            | Same as `==`                | Same as `==`        | Same as `==`                                   |
| `is not`          | Same as `!=`                       | Same as `!=`            | Same as `!=`                | Same as `!=`        | Same as `!=`                                   |
| `points`          | Same type + exact value            | Same type + exact match | Same type + container eq    | Both None → True    | Different types → False                        |
| `not points`      | Inverse of `points`                | Inverse of `points`     | Inverse of `points`         | Inverse of `points` | Different types → True                         |
| `&&`              | Logical AND                        | Logical AND             | Logical AND                 | Logical AND         | Truthiness-based                               |
| `\|\|`            | Logical OR                         | Logical OR              | Logical OR                  | Logical OR          | Truthiness-based                               |

### User-Defined Function Call — Step by Step

When the evaluator encounters `[KeywordFn, "funcName", argCount]`:

```
1. Check math functions → dispatch_math() if found
2. Check builtins → get_builtins()[name]() if found
3. Look up user function: scope.getFunction(name, arity)
4. If body is nullptr → error ("forward-declared but never defined")
5. Pop argCount values + their names from both stacks
6. Reverse args (stack pops in reverse order)
7. Create child Scope (parent=current, barrier=true)
8. Bind each arg to its parameter name in child scope
9. Execute function body (def.body->execute(funcScope))
10. If ReturnException caught:
    a. Write back ref params: for each @param, scope.set(callerName, funcScope.get(paramName))
    b. Push returned value
11. If body completes without give:
    a. Write back ref params (same as above)
    b. Push var(NoneType{}) — function returns None
12. If error (unknown function) → throw with line number
```

---

## 10. Deep Dive: Scope and Variable Resolution

### How Variable Lookup Works (`get`)

When `scope.get("x")` is called:

```
1. Check current scope's values map for "x"
2. If found → return it
3. If not found AND parent exists → call parent.get("x")  (ignores barrier!)
4. If not found anywhere → return var(NoneType{})  (undefined = None)
```

Note: `get()` **ignores** the barrier flag. This means you can always READ global variables from inside functions.

### How Variable Mutation Works (`set`)

When `scope.set("x", value)` is called:

```
1. Check current scope's values map for "x"
2. If found HERE → update it, done
3. If not found AND parent exists AND barrier is false → try parent.set("x", value)
4. If barrier is true → throw "Undefined variable 'x' (cannot mutate outer scope)"
5. If no parent → throw "Undefined variable 'x'"
```

The **barrier** is the key difference. `set()` respects barriers, `get()` doesn't.

### Why This Design?

```
var x = 10.
fn foo():
    print(x).     --> WORKS: get() ignores barrier, reads global x = 10
    x = 99.       --> ERROR: set() hits barrier, cannot modify global x
    var x = 99.   --> WORKS: define() creates NEW local x, doesn't touch global
;
```

This prevents accidental global state mutation while still allowing read access. If you want to modify something from a function, use pass-by-reference parameters.

### The Scope Chain Visualized

```
Program:
    var x = 10.
    fn foo(a):
        if a > 5:
            var temp = a * 2.
            print(x + temp).
        ;
    ;
    foo(7).

Scope chain during print(x + temp):

┌──────────────┐
│ Block (if)   │  temp = 14     barrier = false
│   parent ────┼──►┌──────────────┐
└──────────────┘   │ Function     │  a = 7       barrier = true
                   │   parent ────┼──►┌──────────────┐
                   └──────────────┘   │ Block (prog) │  (hoisted fns)  barrier = false
                                      │   parent ────┼──►┌──────────────┐
                                      └──────────────┘   │ Global       │  x = 10, PI, e
                                                         └──────────────┘

Looking up "temp":  found in if-block scope ✓
Looking up "x":     not in if → not in function → not in block → found in global ✓
Setting "x":        not in if → BARRIER in function scope → ERROR ✗
```

---

## 11. Deep Dive: Functions, Overloading, and Pass-by-Reference

### Function Storage Keys

Functions are stored with composite keys: `"name/arity"`:

```
fn add(a, b): give a + b. ;         → stored as "add/2"
fn add(a, b, c): give a + b + c. ;  → stored as "add/3"
fn greet(): print("hi"). ;          → stored as "greet/0"
```

When calling `add(1, 2)`, the interpreter looks up `"add/2"`. When calling `add(1, 2, 3)`, it looks up `"add/3"`. This is **overloading by arity** (number of parameters).

### Pass-by-Reference — Complete Example

```
fn increment(@x):
    x = x + 1.
;
var n = 10.
increment(n).
print(n).       --> 11 (not 10!)
```

**How it works internally, step by step:**

1. `increment(n)` is parsed into RPN: `[Identifier "n"] [KeywordFn "increment"/1]`
2. Evaluator pushes `var(10)` with name `"n"` (from `scope.get("n")`)
3. Function call begins: pop 1 arg → value=`var(10)`, name=`"n"`
4. Look up `scope.getFunction("increment", 1)` → found, with `isRefParam = [true]`
5. Create child scope `funcScope` (barrier=true)
6. `funcScope.define("x", var(10))` — x gets a COPY of n's value
7. Execute body: `x = x + 1` → `funcScope.set("x", var(11))` — modifies local x
8. Body completes without `give`
9. **Write-back:** `isRefParam[0]` is true, and argName[0] is `"n"`, so:
   `scope.set("n", funcScope.get("x"))` → caller's `n` becomes `var(11)`
10. Push `var(NoneType{})` as return value

### Multiple Ref Params

```
fn swap(@a, @b):
    var temp = a.
    a = b.
    b = temp.
;
var x = 1 y = 2.
swap(x, y).
--> x = 2, y = 1
```

Both `a` and `b` are ref params. After the function body runs, both are written back to `x` and `y` respectively.

### What If You Pass a Literal to a Ref Param?

```
fn increment(@x):
    x = x + 1.
;
increment(42).    --> nameStk has "" for the literal 42
                  --> write-back is skipped because argName is empty
                  --> no error, but the increment is lost
```

---

## 12. Deep Dive: Two-Pass Execution and Forward Declarations

### The Problem

Without two-pass execution, this code would fail:

```
fn isEven(n):
    if n == 0: give True. ;
    give isOdd(n - 1).       --> ERROR: isOdd not defined yet!
;

fn isOdd(n):
    if n == 0: give False. ;
    give isEven(n - 1).
;

print(isEven(4)).
```

### The Solution: Two-Pass Hoisting

`BlockStmt::execute()` does two passes:

```cpp
void BlockStmt::execute(Scope &scope) {
    Scope blockScope(&scope, false);

    // PASS 1: Register ALL function definitions first
    for (auto &stmt : statements) {
        auto funcDef = std::dynamic_pointer_cast<FunctionDefStmt>(stmt);
        if (funcDef && funcDef->body) {
            FunctionDef def;
            def.name = funcDef->name;
            def.params = funcDef->params;
            def.body = funcDef->body;
            blockScope.defineFunction(funcDef->name, def);
        }
    }

    // PASS 2: Execute all statements in order
    for (auto &stmt : statements)
        stmt->execute(blockScope);
}
```

After Pass 1, both `isEven` and `isOdd` are registered in the scope. When Pass 2 reaches the call to `isOdd` inside `isEven`, the function is already available.

**Important:** Pass 1 only registers functions that have bodies. Forward declarations (body=null) are not hoisted — they're only registered when their `FunctionDefStmt::execute()` runs in Pass 2.

### Forward Declarations

You can also explicitly forward-declare a function:

```
fn helper(x).          --> "I promise helper(x) will be defined later"

fn main():
    give helper(42).   --> OK — forward declaration exists
;

fn helper(x):          --> Now providing the actual body
    give x * 2.
;
```

A forward declaration:

1. Creates a stub in the scope: `FunctionDef { name="helper", params=["x"], body=nullptr }`
2. Adds the key `"helper/1"` to `declaredFunctions` set
3. If you try to CALL a forward-declared function that was never defined, you get: `"Function 'helper' was forward-declared but never defined"`
4. When the real definition appears, it replaces the stub and removes the key from `declaredFunctions`
5. Re-declaring an already-defined function throws an error

### When Do You Need Forward Declarations?

Usually you don't — the two-pass hoisting handles mutual recursion within the same block. Forward declarations are useful when:

- A function is called in a scope BEFORE the scope where it's defined
- You want to document that a function will be provided by external code
- You're writing a header-like section at the top of your script

---

## 13. Operator Precedence Table

Higher precedence = binds tighter. Precedence 8 is the tightest.

| Precedence | Operators                                          | Description         | Associativity |
| :--------: | -------------------------------------------------- | ------------------- | ------------- |
|   **8**    | `~` (unary minus), `!` (logical not)               | Unary operators     | Right         |
|   **7**    | `^`                                                | Exponentiation      | Left          |
|   **6**    | `*`, `/`, `%`                                      | Multiplicative      | Left          |
|   **5**    | `+`, `-`                                           | Additive            | Left          |
|   **4**    | `<`, `<=`, `>`, `>=`                               | Relational          | Left          |
|   **3**    | `==`, `!=`, `is`, `is not`, `points`, `not points` | Equality / Identity | Left          |
|   **2**    | `&&` (`and`)                                       | Logical AND         | Left          |
|   **1**    | `\|\|` (`or`)                                      | Logical OR          | Left          |

### Precedence in Action

```
Expression:  2 + 3 * 4         → 2 + (3 * 4) = 14      (* before +)
Expression:  2 ^ 3 + 1         → (2 ^ 3) + 1 = 9       (^ before +)
Expression:  !True || False     → (!True) || False       (! before ||)
Expression:  a == b && c > d    → (a == b) && (c > d)   (== and > before &&)
Expression:  x is not None      → (x) is not (None)     (compound operator, prec 3)
Expression:  a + b * c ^ d      → a + (b * (c ^ d))     (^ first, then *, then +)
```

---

## 14. How to Build ScriptIt

### Prerequisites

- **C++20 compiler** (GCC 10+, Clang 10+, or MSVC 2019+)
- The `pythonic` library headers (the parent project this lives in)

### Build Command

```bash
g++ -std=c++20 \
    -I/path/to/pythonic/include \
    -o scriptit \
    /path/to/pythonic/include/pythonic/REPL/ScriptIt.cpp \
    /path/to/pythonic/src/pythonicDispatchStubs.cpp \
    -O2
```

**Explanation:**

- `-std=c++20` — Required for C++20 features (concepts, ranges, etc.)
- `-I/path/to/pythonic/include` — Tells the compiler where to find the `pythonic/` headers
- `-o scriptit` — Output executable name
- `ScriptIt.cpp` — The main source file (it `#include`s all the `.hpp` files)
- `pythonicDispatchStubs.cpp` — Required link-time definitions for the `pythonic` library
- `-O2` — Optimization level (optional but recommended for performance)

### Running

```bash
# Execute a script file
./scriptit myprogram.sit

# Interactive REPL
./scriptit

# Pipe code via stdin
echo 'print("Hello, World!").' | ./scriptit --script

# Run as notebook kernel (for Jupyter integration)
./scriptit --kernel

# Run built-in test
./scriptit --test
```

### File Extension

ScriptIt files use the `.sit` extension by convention.

---

## 15. Glossary

| Term                        | Definition                                                                                                                                                            |
| --------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --- | -------------------------------------------------------------------------------------- |
| **AST**                     | Abstract Syntax Tree — a tree data structure representing the hierarchical structure of source code                                                                   |
| **Token**                   | A meaningful chunk of source code: a number, keyword, operator, string, etc. The smallest unit the parser works with                                                  |
| **Tokenizer / Lexer**       | The component that breaks raw source text into a flat list of tokens                                                                                                  |
| **Parser**                  | The component that reads tokens and builds an AST — determines the program's structure                                                                                |
| **Evaluator**               | The component that walks the AST and executes it, producing output and side effects                                                                                   |
| **RPN**                     | Reverse Polish Notation — operators come after their operands: `3 4 +` instead of `3 + 4`. Eliminates the need for parentheses and precedence rules during evaluation |
| **Shunting-Yard**           | An algorithm invented by Edsger Dijkstra that converts infix notation to RPN, respecting operator precedence and parentheses                                          |
| **Recursive Descent**       | A parsing technique where each grammar rule becomes a function that can call other rule functions recursively                                                         |
| **Tree-Walking**            | An interpreter strategy that directly traverses the AST to execute code, visiting each node and performing its operation                                              |
| **Scope**                   | A mapping of variable names to values and function names to definitions. Scopes chain together (inner → outer) to form the environment                                |
| **Barrier**                 | A flag on a scope that prevents `set()` from reaching parent scopes. Used for function isolation so functions can't accidentally modify global variables              |
| **Arity**                   | The number of parameters a function takes. Used with function name to form unique keys like `"add/2"`                                                                 |
| **Hoisting**                | Pre-registering function definitions before executing any statements in a block. Enables mutual recursion and forward references                                      |
| **Pass-by-Reference**       | A calling convention (marked with `@`) where changes to a parameter inside a function are written back to the caller's variable                                       |
| **Pass-by-Value**           | The default calling convention where the function receives a copy of the argument                                                                                     |
| **Short-Circuit**           | Skipping evaluation of the right operand of `&&`/`                                                                                                                    |     | `when the left operand determines the result.`false && X` → false without evaluating X |
| **Overflow Promotion**      | Automatically using a wider numeric type when an operation would overflow: `int → long → long_long → float → double → long_double`                                    |
| **`var`**                   | The universal value type in ScriptIt, from `pythonic::vars::var`. Can hold int, double, float, long, long_long, string, bool, list, set, dict, or None                |
| **`give`**                  | ScriptIt's keyword for returning a value from a function. Equivalent to `return` in other languages. Implemented via C++ exception (`ReturnException`)                |
| **Forward Declaration**     | Announcing that a function will exist before providing its body: `fn name(params).` Creates a stub that must be defined later                                         |
| **Context Manager**         | The `let ... be ... : ... ;` pattern that automatically cleans up resources (like file handles) after the block exits                                                 |
| **NoneType**                | ScriptIt's null/nil value — represents "no value". Undefined variables are None. Functions without `give` return None                                                 |
| **Dispatch**                | The process of choosing which implementation to call based on the object's type and the method name. Used for dot-methods and builtins                                |
| **`of` keyword**            | Postfix syntax sugar: `upper() of name` is rewritten to `name.upper()` by the parser                                                                                  |
| **Implicit Multiplication** | Math-style shorthand: `3x` means `3 * x`, `2(3+4)` means `2 * (3+4)`. Detected by the parser when adjacent value tokens lack an operator                              |

---

## Appendix A: Complete Expression Evaluation Example

Let's trace through a complex expression step by step.

**Source:** `var result = (3 + 4) * 2 ^ 2.`

**After tokenization:**

```
[KeywordVar] [Identifier "result"] [Equals] [LeftParen] [Number "3"]
[Operator "+"] [Number "4"] [RightParen] [Operator "*"] [Number "2"]
[Operator "^"] [Number "2"] [Dot] [Eof]
```

**The parser sees `var result = ...` and starts Shunting-Yard for `(3 + 4) * 2 ^ 2`:**

| Step | Token | Action                          | Output Queue    | Op Stack |
| ---- | ----- | ------------------------------- | --------------- | -------- |
| 1    | `(`   | Push to op stack                |                 | `(`      |
| 2    | `3`   | To output                       | `3`             | `(`      |
| 3    | `+`   | Push (inside parens)            | `3`             | `( +`    |
| 4    | `4`   | To output                       | `3 4`           | `( +`    |
| 5    | `)`   | Pop until `(`                   | `3 4 +`         |          |
| 6    | `*`   | Push (prec 6)                   | `3 4 +`         | `*`      |
| 7    | `2`   | To output                       | `3 4 + 2`       | `*`      |
| 8    | `^`   | Push (prec 7 > 6, don't pop \*) | `3 4 + 2`       | `* ^`    |
| 9    | `2`   | To output                       | `3 4 + 2 2`     | `* ^`    |
| 10   | `.`   | Terminates expression — flush   | `3 4 + 2 2 ^ *` |          |

**RPN:** `[3] [4] [+] [2] [2] [^] [*]`

**Stack evaluation:**

| Step | Token | Action                | Stack       |
| ---- | ----- | --------------------- | ----------- |
| 1    | `3`   | Push 3                | `[3]`       |
| 2    | `4`   | Push 4                | `[3, 4]`    |
| 3    | `+`   | Pop 4 and 3, add → 7  | `[7]`       |
| 4    | `2`   | Push 2                | `[7, 2]`    |
| 5    | `2`   | Push 2                | `[7, 2, 2]` |
| 6    | `^`   | Pop 2 and 2, pow → 4  | `[7, 4]`    |
| 7    | `*`   | Pop 4 and 7, mul → 28 | `[28]`      |

**Result:** `var(28)` → stored as `result` in scope. ✓

---

## Appendix B: Recursive Function Call Trace

**Source:**

```
fn factorial(n):
    if n <= 1:
        give 1.
    ;
    give n * factorial(n - 1).
;
print(factorial(5)).
```

**Execution trace for `factorial(5)`:**

```
call factorial(5)                              scope: {n=5}
  5 <= 1? No
  evaluate: 5 * factorial(4)
    call factorial(4)                          scope: {n=4}
      4 <= 1? No
      evaluate: 4 * factorial(3)
        call factorial(3)                      scope: {n=3}
          3 <= 1? No
          evaluate: 3 * factorial(2)
            call factorial(2)                  scope: {n=2}
              2 <= 1? No
              evaluate: 2 * factorial(1)
                call factorial(1)              scope: {n=1}
                  1 <= 1? Yes!
                  give 1  → throw ReturnException(1)
                caught → push var(1)
              2 * 1 = 2
              give 2  → throw ReturnException(2)
            caught → push var(2)
          3 * 2 = 6
          give 6  → throw ReturnException(6)
        caught → push var(6)
      4 * 6 = 24
      give 24  → throw ReturnException(24)
    caught → push var(24)
  5 * 24 = 120
  give 120  → throw ReturnException(120)
caught → push var(120)

print(120) → output: "120"
```

Each recursive call creates a new `Scope` with `barrier=true`, containing only its own `n`. The `ReturnException` cleanly unwinds through the if-block and function body at each level.

---

## Appendix C: How Dot-Method Chaining Works

**Source:** `"Hello, World!".upper().split(", ")`

**Parsing into RPN:**

The parser processes this left to right:

1. `"Hello, World!"` → push String token
2. `.upper()` → dot-method call, 0 args → push `[At "upper" 0]`
3. `.split(", ")` → dot-method call, 1 arg → push `", "` then `[At "split" 1]`

**Final RPN:**

```
[String "Hello, World!"] [At "upper" 0] [String ", "] [At "split" 1]
```

**Stack evaluation:**

| Step | Token                  | Stack                     | Action                                        |
| ---- | ---------------------- | ------------------------- | --------------------------------------------- |
| 1    | String "Hello, World!" | `["Hello, World!"]`       | Push string                                   |
| 2    | At "upper" (0 args)    | `["HELLO, WORLD!"]`       | Pop 0 args + 1 object, dispatch upper(), push |
| 3    | String ", "            | `["HELLO, WORLD!", ", "]` | Push string                                   |
| 4    | At "split" (1 arg)     | `[["HELLO", " WORLD!"]]`  | Pop 1 arg + 1 object, dispatch split(), push  |

**Final result:** `var(List{"HELLO", " WORLD!"})`

Method chaining works naturally because each method call pops the previous result off the stack and pushes a new result. The next method call then operates on that new result.

---

## Appendix D: Architecture Summary Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          ScriptIt.cpp                                   │
│  main() → choose mode → executeScript() / REPL / runKernel()           │
└─────────────┬───────────────────┬───────────────────┬───────────────────┘
              │                   │                   │
    ┌─────────▼─────────┐  ┌─────▼──────┐  ┌────────▼────────┐
    │  perser.hpp        │  │ json_and   │  │  (REPL loop)    │
    │                    │  │ _kernel.hpp│  │  persistent      │
    │ ┌────────────────┐ │  │            │  │  globalScope    │
    │ │  Tokenizer     │ │  │  runKernel │  └─────────────────┘
    │ │  source→tokens │ │  │  JSON I/O  │
    │ └───────┬────────┘ │  └────────────┘
    │         │          │
    │ ┌───────▼────────┐ │
    │ │  Parser        │ │
    │ │  tokens→AST    │ │
    │ │  (rec. descent │ │
    │ │  + Shunting-   │ │
    │ │  Yard)         │ │
    │ └───────┬────────┘ │
    │         │          │
    │ ┌───────▼────────┐ │     ┌───────────────────────┐
    │ │  Evaluator     │ │     │  scriptit_methods.hpp  │
    │ │  (RPN stack    │◄├────►│  .upper(), .append()   │
    │ │  machine)      │ │     │  dot-method dispatch   │
    │ └───────┬────────┘ │     └───────────────────────┘
    │         │          │
    └─────────┼──────────┘     ┌───────────────────────┐
              │                │  scriptit_builtins.hpp │
              ├───────────────►│  print(), input()      │
              │                │  open(), math funcs    │
              │                │  FileRegistry          │
              │                └───────────────────────┘
              │
    ┌─────────▼──────────┐     ┌───────────────────────┐
    │ scriptit_types.hpp │     │  pythonic library      │
    │  TokenType enum    │────►│  pythonic::vars::var   │
    │  AST node structs  │     │  pythonic::math::*     │
    │  Scope class       │     │  Overflow::Promote     │
    │  helpers           │     └───────────────────────┘
    └────────────────────┘
```

---

_This document describes ScriptIt v2, powered by `pythonic::vars::var`. The interpreter is written in C++20 and is designed to be simple, readable, and educational. If you've read this far, you know everything needed to build your own tree-walking interpreter from scratch._
