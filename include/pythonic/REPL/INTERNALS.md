# 🔬 ScriptIt v2 — Interpreter Internals

This document explains how the ScriptIt v2 interpreter works under the hood. It covers every stage from source code to output, including the new type system powered by `pythonic::vars::var`.

---

## Architecture Overview

```
Source Code → [Tokenizer] → [Parser] → [AST] → [Evaluator] → Output
  "x + 1."    Tokens        Tree       Nodes    Walks tree     "result"
```

ScriptIt v2 is a **tree-walking interpreter**. It reads source, tokenizes, parses into an AST, then walks the tree to execute.

**Key difference from v1:** All values are `pythonic::vars::var` — a dynamic type that can hold int, double, string, list, set, dict, bool, None, and more. Arithmetic uses `pythonic::math` with overflow promotion.

---

## Type System

### The `var` Class

Every value in ScriptIt is a `pythonic::vars::var`. This single type replaces v1's raw `double`:

| ScriptIt Literal | `var` Type | Example        |
| ---------------- | ---------- | -------------- |
| `42`             | `INT`      | `var(42)`      |
| `3.14`           | `DOUBLE`   | `var(3.14)`    |
| `"hello"`        | `STRING`   | `var("hello")` |
| `True` / `False` | `BOOL`     | `var(true)`    |
| `None` (no init) | `NONE`     | `var()`        |
| `[1, 2, 3]`      | `LIST`     | List-typed var |
| `{1, 2, 3}`      | `SET`      | Set-typed var  |

### Overflow Promotion

All arithmetic uses `pythonic::math` with `Overflow::Promote`:

```
int → long → long_long → float → double → long_double
```

When an operation would overflow, the result is automatically promoted to the next wider type. For example:

```
1000000 * 1000000.    --> promoted from int to long_long, result: 1000000000000
```

---

## Stage 1: Tokenizer

**Class:** `Tokenizer`

Reads raw source text character-by-character and produces a flat list of `Token` objects.

### Token Types

```
Number, String, Identifier, Operator,
LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
KeywordVar, KeywordFn, KeywordGive, KeywordIf, KeywordElif, KeywordElse,
KeywordFor, KeywordIn, KeywordRange, KeywordFrom, KeywordTo,
KeywordPass, KeywordWhile, KeywordLet, KeywordBe,
Equals, Comma, Dot, Colon, Semicolon, At, Eof
```

### String Tokenization (New in v2)

The tokenizer recognizes both `"double"` and `'single'` quoted strings:

```
Input:  "hello world"
Token:  [String: "hello world"]
```

**Escape sequences** are processed during tokenization:

| Escape | Result       |
| ------ | ------------ |
| `\n`   | newline      |
| `\t`   | tab          |
| `\\`   | backslash    |
| `\"`   | double quote |
| `\'`   | single quote |

### Bracket Tokenization (New in v2)

Square brackets `[` `]` for lists and curly braces `{` `}` for sets/dicts are tokenized as their own types, enabling container literal parsing.

### The Dot Ambiguity

The `.` character serves two purposes:

- **Statement terminator:** `x.` means "end of statement"
- **Decimal point:** `3.14` is a float

Resolution: If the next character after `.` is a digit, it's a decimal point. Otherwise, it's a terminator.

### Keyword Map (Dispatch)

Keywords are recognized via a `static const std::unordered_map`:

```cpp
static const std::unordered_map<std::string, TokenType> keywords = {
    {"var", TokenType::KeywordVar}, {"fn", TokenType::KeywordFn},
    {"let", TokenType::KeywordLet}, {"be", TokenType::KeywordBe},
    // ... etc
};
```

### Implicit Multiplication

When a number is immediately followed by `(`, the tokenizer inserts an implicit `*` token:

```
3(4 + 1)  →  3 * (4 + 1)  →  15
```

---

## Stage 2: Parser

**Class:** `Parser`

Reads the token list and builds an **Abstract Syntax Tree (AST)**.

### AST Node Types

| Node              | Purpose                            | Pattern                                 |
| ----------------- | ---------------------------------- | --------------------------------------- |
| `BlockStmt`       | A sequence of statements           | Multiple statements                     |
| `AssignStmt`      | Variable declaration or assignment | `var x = expr.` or `x = expr.`          |
| `MultiVarStmt`    | Multi-variable declaration         | `var a, b, c.`                          |
| `ExprStmt`        | Expression with auto-print         | `expr.`                                 |
| `IfStmt`          | Conditional branching              | `if expr: body [elif ...] [else ...] ;` |
| `ForStmt`         | Range-based for loop               | `for i in range(from X to Y): body ;`   |
| `ForInStmt`       | Iterable for loop (new in v2)      | `for x in iterable: body ;`             |
| `WhileStmt`       | While loop                         | `while expr: body ;`                    |
| `FunctionDefStmt` | Function definition                | `fn name @(params): body ;`             |
| `ReturnStmt`      | Return from function               | `give(expr).`                           |
| `PassStmt`        | No-op placeholder                  | `pass.`                                 |

### Statement Parsing

The parser recognizes statements by their leading token:

1. **`var`** → `parseVarDeclaration()` — checks for comma-separated names (→ `MultiVarStmt`) or single `var name = expr.` (→ `AssignStmt`)
2. **`let`** → `parseLetBe()` — parses `let name be expr.` (→ `AssignStmt` with `isDeclaration=true`)
3. **`fn`** → `parseFunctionDef()` — parses function definition
4. **`if`** → `parseIf()` — parses if/elif/else chain
5. **`while`** → `parseWhile()` — parses while loop
6. **`for`** → checks for `range(from...to...)` pattern or iterable → `ForStmt` or `ForInStmt`
7. **`give`** → `parseReturn()` — parses return statement
8. **`pass`** → `parsePass()` — no-op
9. **Identifier followed by `=`** → assignment
10. **Anything else** → `ExprStmt` (expression, result auto-printed if non-None)

### Expression Parsing: Three-Layer System

Expressions are parsed via a three-layer system that handles operator precedence and short-circuit evaluation:

```
parseExpression()        → Top level: delegates to parseLogicalOr()
  parseLogicalOr()       → Handles || with lazy evaluation
    parseLogicalAnd()    → Handles && with lazy evaluation
      parsePrimaryExpr() → Shunting-Yard for everything else
```

### The Shunting-Yard Algorithm

The core expression parser uses Dijkstra's Shunting-Yard algorithm to convert infix to **RPN** (Reverse Polish Notation):

```
Input:  2 + 3 * 4
RPN:    2 3 4 * +

Input:  (2 + 3) * 4
RPN:    2 3 + 4 *
```

Uses two data structures:

- **Output queue:** collects operands and operators in RPN order
- **Operator stack:** temporarily holds operators, respecting precedence

**Operator precedence** is looked up from a dispatch map:

| Precedence | Operators            |
| ---------- | -------------------- |
| 1          | `\|\|`               |
| 2          | `&&`                 |
| 3          | `==`, `!=`           |
| 4          | `<`, `<=`, `>`, `>=` |
| 5          | `+`, `-`             |
| 6          | `*`, `/`, `%`        |
| 7          | `^`                  |
| 8          | `~` (unary neg), `!` |

### Short-Circuit Evaluation

`&&` and `||` at the top level are **not** placed into the RPN. Instead, the parser creates a tree with:

```
Expression
├── logicalOp: "||" or "&&"
├── lhs: Expression [left side]
└── rhs: Expression [right side]
```

The evaluator checks the LHS first and **only evaluates RHS if needed**:

- `&&`: If LHS is falsy → return 0, skip RHS
- `||`: If LHS is truthy → return 1, skip RHS

### Dot Forgiveness (New in v2)

The parser calls `consumeDotOrForgive()` instead of strictly requiring `.`:

```cpp
void consumeDotOrForgive() {
    if (check(TokenType::Dot)) advance();
    else if (check(TokenType::Eof) || check(TokenType::Semicolon)
             || check(TokenType::KeywordElif) || check(TokenType::KeywordElse))
        return; // forgive missing dot at boundaries
    else
        throw std::runtime_error("Expected '.' ...");
}
```

This means the `.` can be omitted at:

- End of file (EOF)
- Before `;` (block terminator)
- Before `elif` / `else`

But it's **still required** between statements in the middle of a script.

### Container Literal Parsing (New in v2)

**Lists:** When `[` is encountered, the parser collects comma-separated expressions until `]`:

```
[1, 2, "three"]  →  ListLiteral RPN token with 3 elements
```

**Sets:** When `{` is encountered, same pattern until `}`:

```
{1, 2, 3}  →  SetLiteral RPN token with 3 elements
```

### Unary Operators

The parser detects unary `-` vs binary `-` by checking the previous token. Unary minus is renamed to `~` internally:

```
-5       →  RPN: [5, ~]       (negation)
10 - 5   →  RPN: [10, 5, -]   (subtraction)
```

---

## Stage 3: Evaluator

### Expression Evaluation (RPN Stack Machine)

The evaluator walks the RPN queue using a stack:

```
RPN: 2 3 4 * +

Step 1: push 2          Stack: [2]
Step 2: push 3          Stack: [2, 3]
Step 3: push 4          Stack: [2, 3, 4]
Step 4: * → pop 4,3     Stack: [2, 12]
Step 5: + → pop 12,2    Stack: [14]

Result: var(14)
```

### Dispatch Maps (Architecture Decision)

All operator and function evaluation uses **static dispatch maps** (unordered_maps) instead of if-else chains. This makes the code more maintainable and scalable:

#### 1. Binary Operator Dispatch

```cpp
static const std::unordered_map<std::string,
    std::function<var(var&, var&)>> binaryOps = {
    {"+", [](var& a, var& b) { return pythonic::math::add(a, b, Overflow::Promote); }},
    {"-", [](var& a, var& b) { return pythonic::math::sub(a, b, Overflow::Promote); }},
    {"*", [](var& a, var& b) { return pythonic::math::mul(a, b, Overflow::Promote); }},
    {"/", [](var& a, var& b) { return pythonic::math::div(a, b, Overflow::Promote); }},
    {"%", [](var& a, var& b) { return pythonic::math::mod(a, b, Overflow::Promote); }},
    {"^", [](var& a, var& b) { return pythonic::math::pow(a, b, Overflow::Promote); }},
    {"==", ...}, {"!=", ...}, {"<", ...}, ...
};
```

#### 2. Math Function Dispatch

```cpp
static const std::unordered_map<std::string,
    std::function<var(const var&)>> mathOps = {
    {"sin",   [](const var& x) { return pythonic::math::sin(x); }},
    {"cos",   [](const var& x) { return pythonic::math::cos(x); }},
    {"sqrt",  [](const var& x) { return pythonic::math::sqrt(x); }},
    {"log",   [](const var& x) { return pythonic::math::log(x); }},
    // ... 20 functions total
};
```

#### 3. Built-in Function Dispatch

```cpp
static const std::unordered_map<std::string,
    std::function<void(std::stack<var>&, int)>> builtinOps = {
    {"print",  [](stack& s, int argc) { /* multi-arg print */ }},
    {"len",    [](stack& s, int argc) { /* length of string/list/set */ }},
    {"type",   [](stack& s, int argc) { /* type name */ }},
    {"append", [](stack& s, int argc) { /* list append */ }},
    {"read",   [](stack& s, int argc) { /* file read */ }},
    {"write",  [](stack& s, int argc) { /* file write */ }},
    // ... 16 functions total
};
```

### Statement Execution

Each AST node has an `execute(Scope& scope)` method:

| Node              | Behavior                                                                |
| ----------------- | ----------------------------------------------------------------------- |
| `ExprStmt`        | Evaluate expression. Print result if non-None.                          |
| `AssignStmt`      | Evaluate expr, store in scope (declare or set).                         |
| `MultiVarStmt`    | Define all names as `var()` (None).                                     |
| `IfStmt`          | Evaluate condition. If truthy → execute then-block. Else try elif/else. |
| `ForStmt`         | Evaluate from/to range, iterate with counter variable.                  |
| `ForInStmt`       | Evaluate iterable (list/string/set), iterate with element var.          |
| `WhileStmt`       | While condition is truthy, execute body.                                |
| `FunctionDefStmt` | Store function definition in scope (doesn't run it yet).                |
| `ReturnStmt`      | Evaluate expr, throw `ReturnException` with the value.                  |
| `PassStmt`        | No-op.                                                                  |

### The `give` (Return) Mechanism

`give(value)` throws a C++ exception (`ReturnException`). This cleanly unwinds through nested loops and if-blocks:

```
fn find @():
    var i = 0.
    while i < 100:
        if i == 42:
            give(i).      ← throws ReturnException(42)
        ;
        i = i + 1.
    ;                     ← exception unwinds through here
;                         ← caught by function call handler
```

### Output Formatting

The `format_output(var)` function converts a `var` to its display string:

| Type         | Format               | Example              |
| ------------ | -------------------- | -------------------- |
| None         | `"None"`             | (usually suppressed) |
| String       | raw string           | `hello world`        |
| Bool         | `"True"` / `"False"` | `True`               |
| Int/Long/etc | integer string       | `42`                 |
| Double/Float | decimal string       | `3.33333`            |

**ExprStmt suppression:** When an expression evaluates to `None`, `ExprStmt` does not print anything. This prevents `print()` from outputting extra lines (since `print()` returns None).

---

## Stage 4: Scope System

### Scope Chain

Scopes form a linked list:

```
Global Scope       ← PI, e, user variables
  ├── Function Scope (barrier=true)  ← parameter locals
  │     └── If-Block Scope (barrier=false)
  └── For-Loop Scope (barrier=false)  ← loop variable
```

### The Barrier Mechanism

| Scope Type     | Barrier | Can read outer? | Can write outer? |
| -------------- | ------- | --------------- | ---------------- |
| If/Else block  | false   | ✅ Yes          | ✅ Yes           |
| For/While loop | false   | ✅ Yes          | ✅ Yes           |
| Function       | true    | ✅ Yes          | ❌ No            |

- **`get(name)`**: Walks up the chain — no barriers for reading.
- **`set(name, value)`**: Walks up but **stops at barriers**. Functions can't modify outer variables.
- **`define(name, value)`**: Always defines in the **current** scope.

### The `wipe` Command

In REPL mode, `wipe`:

1. Clears the terminal screen
2. Resets the global scope (removes all user variables and functions)
3. Re-defines built-in constants (`PI`, `e`, `ans`)

---

## Built-in Functions Reference

### I/O Functions

| Function   | Args | Description                                |
| ---------- | ---- | ------------------------------------------ |
| `print`    | 1+   | Print space-separated args with newline    |
| `pprint`   | 1    | Pretty-print (detailed var representation) |
| `input`    | 0–1  | Read line from stdin (optional prompt)     |
| `read`     | 1    | Read entire file contents as string        |
| `write`    | 2    | Write string to file                       |
| `readLine` | 1    | Read file as list of lines                 |

### Type Functions

| Function | Args | Description                    |
| -------- | ---- | ------------------------------ |
| `type`   | 1    | Returns type name as string    |
| `str`    | 1    | Convert to string              |
| `int`    | 1    | Convert to integer (truncate)  |
| `float`  | 1    | Convert to float               |
| `len`    | 1    | Length of string, list, or set |

### Collection Functions

| Function     | Args | Description                          |
| ------------ | ---- | ------------------------------------ |
| `list`       | 0    | Create empty list                    |
| `set`        | 0    | Create empty set                     |
| `append`     | 2    | Append item to list, return new list |
| `pop`        | 1    | Pop last item from list              |
| `range_list` | 2    | Create list from range               |

### Math Functions

All math functions use `pythonic::math` and operate on `var`:

| Function | Description      | Function | Description      |
| -------- | ---------------- | -------- | ---------------- |
| `sin`    | sine             | `asin`   | arc sine         |
| `cos`    | cosine           | `acos`   | arc cosine       |
| `tan`    | tangent          | `atan`   | arc tangent      |
| `cot`    | cotangent        | `sec`    | secant           |
| `csc`    | cosecant         | `sqrt`   | square root      |
| `abs`    | absolute value   | `ceil`   | ceiling          |
| `floor`  | floor            | `round`  | round            |
| `log`    | natural log      | `log2`   | log base 2       |
| `log10`  | log base 10      | `min`    | minimum (2 args) |
| `max`    | maximum (2 args) |          |                  |

---

## Summary of Key Design Decisions

| Decision                  | Choice                                    | Why                                                                 |
| ------------------------- | ----------------------------------------- | ------------------------------------------------------------------- |
| Value type                | `pythonic::vars::var` (dynamic)           | Supports strings, lists, sets, None — much richer than raw `double` |
| Arithmetic                | `pythonic::math` with `Overflow::Promote` | Auto-promotes on overflow (int→long→double), no silent data loss    |
| Expression representation | RPN (Reverse Polish Notation)             | Simple stack-based evaluation, no recursion needed                  |
| Short-circuit `&&`/`\|\|` | Tree nodes wrapping RPN sub-expressions   | Can't lazily evaluate inside flat RPN                               |
| Scope model               | Dynamic scope with barriers               | Functions can read but not mutate outer vars                        |
| Return mechanism          | C++ exceptions (`ReturnException`)        | Cleanly unwinds through nested control flow                         |
| Statement terminator      | `.` (dot) with forgiveness at boundaries  | Familiar alternative to `;`, forgive-at-EOF improves ergonomics     |
| Function dispatch         | `static const std::unordered_map`         | Scalable, maintainable, O(1) lookup vs if-else chains               |
| Math dispatch             | `pythonic::math::*` via map               | Type-safe, overflow-aware, consistent with host library             |

---

## File Structure

```
REPL/
├── ScriptIt.cpp           Main interpreter source (~1920 lines)
├── ScriptIt_v1.cpp        Backup of v1 (double-based)
├── README.md              User-facing language guide
├── INTERNALS.md           This file
├── test_interpreter.py    Test suite (200+ tests)
├── script                 Compiled binary
└── pythonicCalculator.hpp Legacy calculator REPL
```
