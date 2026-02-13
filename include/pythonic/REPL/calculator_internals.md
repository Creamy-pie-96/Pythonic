# pythonicCalculator — Internals Documentation

> **File:** `include/pythonic/REPL/pythonicCalculator.hpp`
> **Namespace:** `pythonic::calculator`
> **Lines:** ~760
> **Included from:** `pythonic/pythonic.hpp`

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Pipeline: Input → Result](#pipeline-input--result)
4. [TokenType Enum](#tokentype-enum)
5. [Token Struct](#token-struct)
6. [Helper Functions](#helper-functions)
7. [Tokenizer](#tokenizer)
8. [Parser (Shunting-Yard)](#parser-shunting-yard)
9. [Evaluator](#evaluator)
10. [Calculator Class](#calculator-class)
11. [Variable System](#variable-system)
12. [Implicit Multiplication](#implicit-multiplication)
13. [Bracket Handling](#bracket-handling)
14. [Error Handling](#error-handling)
15. [Entry Point: `calculator()`](#entry-point-calculator)
16. [Supported Operations Reference](#supported-operations-reference)
17. [Design Decisions & Trade-offs](#design-decisions--trade-offs)

---

## Overview

`pythonicCalculator` is a **self-contained, header-only CLI calculator** that lives inside the `pythonic` library. It implements a classic **three-stage expression evaluator**:

```
Input string → Tokenizer → Parser (Shunting-Yard) → Evaluator (RPN stack machine) → double result
```

It supports:

- Basic arithmetic: `+`, `-`, `*`, `/`, `^` (power)
- Parentheses, braces, and square brackets: `()`, `{}`, `[]` — all interchangeable
- Unary minus: `-5`, `-(3+2)`
- Math functions: `sin`, `cos`, `tan`, `cot`, `sec`, `csc`, `asin`, `acos`, `atan`, `acot`, `asec`, `acsc`, `log`, `log2`, `log10`, `sqrt`, `abs`
- Variables: `var a = 10, b = a * 2`
- Implicit multiplication: `2(3)` → `6`, `2x` → `2*x`

---

## Architecture

The calculator is structured as **four classes plus helper utilities**, all inside `namespace pythonic::calculator`:

```
┌─────────────────────────────────────────────────┐
│                  Calculator                      │
│  (orchestrates the pipeline, owns variables)     │
│                                                  │
│  ┌──────────┐  ┌────────┐  ┌───────────┐       │
│  │ Tokenizer│→ │ Parser │→ │ Evaluator │→ double│
│  └──────────┘  └────────┘  └───────────┘       │
│                                                  │
│  variables: map<string, double>                  │
└─────────────────────────────────────────────────┘
```

| Class        | Responsibility                                           | Input           | Output                     |
| ------------ | -------------------------------------------------------- | --------------- | -------------------------- |
| `Tokenizer`  | Lexical analysis — splits input into tokens              | `string`        | `vector<Token>`            |
| `Parser`     | Syntactic analysis — Shunting-Yard to RPN                | `vector<Token>` | `queue<Token>` (RPN order) |
| `Evaluator`  | Semantic evaluation — stack-based RPN executor           | `queue<Token>`  | `double`                   |
| `Calculator` | Orchestrator — handles variables, assignments, delegates | `string` (line) | prints result              |

Plus standalone helpers:

- `get_operator_precedence()` — returns int precedence for an operator
- `is_right_associative()` — checks if `^` or `~` (unary minus)
- `is_math_function()` — checks against the 17 built-in math functions

---

## Pipeline: Input → Result

For input `var x = 2(3+1), sqrt(x)`:

```
Step 1 — Tokenize:
  [KeywordVar:"var"] [Ident:"x"] [Equals:"="] [Num:"2"] [Op:"*"] [LeftParen:"("]
  [Num:"3"] [Op:"+"] [Num:"1"] [RightParen:")"] [Comma:","] [Ident:"sqrt"]
  [LeftParen:"("] [Ident:"x"] [RightParen:")"]

  Note: The "*" between "2" and "(" was INSERTED by the implicit-multiplication logic.

Step 2 — Split at assignment boundary:
  Assignment: x = 2 * (3 + 1) → evaluates to 8.0
  Expression: sqrt(x)         → variable substitution → sqrt(8.0) → evaluates to 2.82843

Step 3 — Parse (Shunting-Yard) for "2 * (3 + 1)":
  RPN queue: [2] [3] [1] [+] [*]

Step 4 — Evaluate RPN:
  Stack trace: push 2 → push 3 → push 1 → pop 1,3 push 4 → pop 4,2 push 8
  Result: 8.0

Step 5 — Store variable x = 8.0, print "Variable x = 8"

Step 6 — Parse & evaluate sqrt(x) with x=8 substituted → 2.82843
```

---

## TokenType Enum

```cpp
enum class TokenType {
    Number,        // 42, 3.14, .5
    Identifier,    // x, sin, myVar
    Operator,      // +, -, *, /, ^, ~ (unary minus)
    LeftParen,     // (
    RightParen,    // )
    LeftBrace,     // {
    RightBrace,    // }
    LeftBracket,   // [
    RightBracket,  // ]
    KeywordVar,    // the literal word "var"
    Equals,        // =
    Comma,         // ,  (delimiter between variable assignments)
    Dot            // .  (delimiter between variable assignments)
};
```

**Key insight:** The `~` operator is an internal representation of **unary minus**. When the tokenizer sees `-` in a position where it must be unary (after an operator, opening bracket, `=`, `,`, `.`, or at the start), it emits `~` instead of `-`. This avoids ambiguity in the parser.

---

## Token Struct

```cpp
struct Token {
    TokenType type;
    std::string value;
    int position;        // character position in original input (for error messages)
};
```

Every token records its source position, which enables precise error messages like:

> Unknown character '@' at position 5

---

## Helper Functions

### `get_operator_precedence(op)`

Returns the precedence level for the Shunting-Yard algorithm:

| Precedence |         Operators         |
| :--------: | :-----------------------: |
|     1      |         `+`, `-`          |
|     2      |         `*`, `/`          |
|     3      |            `^`            |
|     4      |     `~` (unary minus)     |
|     5      | Math functions (implicit) |

Higher number = higher precedence = binds tighter.

### `is_right_associative(op)`

Returns `true` for `^` (power) and `~` (unary minus). All other operators are left-associative.

This means `2^3^4` evaluates as `2^(3^4)` = `2^81`, not `(2^3)^4` = `8^4`.

### `is_math_function(str)`

Checks against a static list of 17 recognized math function names:

| Trig                | Inverse Trig           | Logarithmic     | Other  |
| ------------------- | ---------------------- | --------------- | ------ |
| `sin`, `cos`, `tan` | `asin`, `acos`, `atan` | `log` (natural) | `sqrt` |
| `cot`, `sec`, `csc` | `acot`, `asec`, `acsc` | `log2`, `log10` | `abs`  |

---

## Tokenizer

**Class:** `Tokenizer`
**Method:** `tokenize(const string& expression) → vector<Token>`

The tokenizer is a **single-pass, character-by-character scanner** that recognizes:

### Number Parsing

Numbers are sequences of digits optionally containing one decimal point:

- `42` → `Token(Number, "42")`
- `3.14` → `Token(Number, "3.14")`
- `.5` → `Token(Number, ".5")` (leading dot with following digit = number)
- `5.` → `Token(Number, "5")` + `Token(Dot, ".")` (trailing dot with no following digit = delimiter)

**Tricky part — the dot ambiguity:**

A `.` character can be either:

1. A decimal point (part of a number like `3.14`)
2. A delimiter (separating variable assignments like `var a=1. b=2`)

The rule: If `.` is followed by a digit, it's a decimal point. Otherwise, it's a delimiter.

### Identifier Parsing

Identifiers start with `[a-zA-Z_]` and continue with `[a-zA-Z0-9_]`. The word `"var"` is special-cased to `TokenType::KeywordVar`.

### Unary Minus Detection

When `-` is encountered, the tokenizer checks what came before:

- If **nothing** (start of expression), or the previous token is an **operator**, **opening bracket**, `=`, `,`, `.`, or `var` → it's unary minus → emit `~`
- Otherwise → it's binary subtraction → emit `-`

### Implicit Multiplication Insertion

The tokenizer automatically inserts `*` operators where multiplication is implied. See the [Implicit Multiplication](#implicit-multiplication) section below.

---

## Parser (Shunting-Yard)

**Class:** `Parser`
**Method:** `parse(const vector<Token>& tokens) → queue<Token>`

Implements Dijkstra's **Shunting-Yard algorithm** to convert infix notation into **Reverse Polish Notation (RPN)**.

### Algorithm Summary

Uses two data structures:

- **Output queue** — the RPN result
- **Operator stack** — temporary holding for operators

For each token:

1. **Number** → push to output queue
2. **Identifier (function)** → push to operator stack
3. **Identifier (variable)** → push to operator stack (will be resolved later)
4. **Operator** → pop higher-precedence operators from stack to output, then push self
5. **Opening bracket** → push to operator stack + bracket stack
6. **Closing bracket** → pop operators to output until matching opening bracket found

### Bracket Stack

A `stack<char> bracketStack` tracks the **nesting of bracket types** to ensure:

- `(` is closed by `)`, not `}` or `]`
- `{` is closed by `}`, not `)` or `]`
- `[` is closed by `]`, not `)` or `}`

**Error examples:**

- `(3+2]` → "Mismatched brackets: Expected closing for '(' but found ']'"
- `(3+2` → "Mismatched or unclosed brackets at end of expression"
- `3+2)` → "Unmatched closing bracket ')'"

### `processClosingBracket()`

Private helper that handles all three closing bracket types uniformly:

1. Verify bracket stack isn't empty
2. Verify top of bracket stack matches expected type
3. Pop operators from operator stack to output until matching open bracket
4. Pop the opening bracket from operator stack and bracket stack
5. If a math function is on top of operator stack, pop it to output (function call complete)

---

## Evaluator

**Class:** `Evaluator`
**Method:** `evaluate(queue<Token> rpnQueue) → double`

A classic **RPN stack machine**:

### Processing Rules

| Token Type                   | Action                              |
| ---------------------------- | ----------------------------------- |
| Number                       | Push `stod(value)` onto value stack |
| Operator `~`                 | Pop 1, push `-a`                    |
| Operator `+`/`-`/`*`/`/`/`^` | Pop 2, push `a op b`                |
| Math function                | Pop 1, push `func(a)`               |

### Division by Zero

```cpp
if (token.value == "/" && b == 0)
    throw std::runtime_error("Division by zero");
```

### Sqrt Domain Check

```cpp
if (token.value == "sqrt" && arg < 0)
    throw std::runtime_error("Domain error: sqrt of negative number");
```

### Reciprocal Trig Functions

`cot`, `sec`, and `csc` are computed as reciprocals:

- `cot(x)` = `1.0 / tan(x)`
- `sec(x)` = `1.0 / cos(x)`
- `csc(x)` = `1.0 / sin(x)`

No explicit check for division by zero here (e.g., `csc(0)` returns `inf`).

### Final Validation

After processing all tokens, exactly one value must remain on the stack:

```cpp
if (values.size() != 1)
    throw std::runtime_error("Invalid expression: Stack not empty after evaluation");
```

---

## Calculator Class

**Class:** `Calculator`
**Method:** `process(const string& line)`

The top-level orchestrator. It:

1. **Tokenizes** the input line
2. **Detects assignments** — two patterns:
   - `var x = expr` (explicit `var` keyword)
   - `x = expr` (implicit, if first token is Identifier followed by `=`)
3. **Handles declarations** via `handleDeclarations()`
4. **Evaluates remaining tokens** as an expression and prints the result

### Private Members

```cpp
Tokenizer tokenizer;
Parser parser;
Evaluator evaluator;
std::map<std::string, double> variables;
```

The `variables` map persists across calls to `process()`, so variables declared in one line are available in subsequent lines within the same `Calculator` instance.

---

## Variable System

### Declaration Syntax

Variables can be declared in several ways:

```
var a = 10                     # single variable with 'var' keyword
var a = 10, b = 20             # comma-separated
var a = 10. b = 20             # dot-separated
a = 10                         # no 'var' keyword (implicit)
var x = 2(3+1), y = sqrt(x)   # expressions with implicit mult, forward references
```

### `handleDeclarations()`

A loop that processes sequential `name = expression` pairs:

```
While tokens remain:
  1. If current token is NOT an Identifier → return (done with assignments)
  2. Read variable name
  3. Check it's not a reserved function name (e.g., "sin")
  4. Expect '=' follows
  5. Collect expression tokens until Comma, Dot, KeywordVar, or end
  6. Evaluate expression
  7. Store in variables map
  8. Print "Variable name = value"
```

**Delimiter flexibility:** Both `,` and `.` work as separators between assignments. This allows natural mathematical notation like `var a=1. b=2`.

### Variable Substitution

In `evaluateExpression()`, before parsing/evaluating, all Identifier tokens that aren't math functions are looked up in the `variables` map:

```cpp
for (auto& t : tokens) {
    if (t.type == TokenType::Identifier && !is_math_function(t.value)) {
        auto it = variables.find(t.value);
        if (it != variables.end()) {
            t.type = TokenType::Number;              // Replace type
            t.value = std::to_string(it->second);    // Replace value
        } else {
            throw std::runtime_error("Unknown variable: " + t.value);
        }
    }
}
```

**Note:** Variable substitution happens in declaration order, so `var a=1, b=a+1` works because `a` is stored before `b`'s expression is evaluated.

---

## Implicit Multiplication

One of the most interesting features. The tokenizer inserts `*` operators automatically in these cases:

### Rules

| Previous Token            | Current Token             | Inserted? | Example              |
| ------------------------- | ------------------------- | --------- | -------------------- |
| `)`, `}`, `]`             | Number                    | Yes       | `(2+3)4` → `(2+3)*4` |
| `)`, `}`, `]`             | `(`, `{`, `[`             | Yes       | `(2)(3)` → `(2)*(3)` |
| `)`, `}`, `]`             | Identifier (non-function) | Yes       | `(2)x` → `(2)*x`     |
| Number                    | `(`, `{`, `[`             | Yes       | `2(3)` → `2*(3)`     |
| Number                    | Identifier                | Yes       | `2x` → `2*x`         |
| Identifier (non-function) | `(`, `{`, `[`             | Yes       | `x(3)` → `x*(3)`     |

### Non-insertion Cases

| Previous Token        | Current Token | Inserted? | Why                                             |
| --------------------- | ------------- | --------- | ----------------------------------------------- |
| Identifier (function) | `(`           | No        | `sin(x)` is a function call, not multiplication |
| Operator              | Number        | No        | `+3` is addition, not `+*3`                     |
| `=`                   | anything      | No        | `a = 5` is assignment                           |

### Implementation Location

The implicit multiplication logic is scattered across three locations in the tokenizer:

1. **Number parsing block** (line ~121): After `)`, `}`, `]`
2. **Identifier parsing block** (line ~155): After Number, `)`, `}`, `]`
3. **Bracket opening block** (line ~200): After Number, `)`, `}`, `]`, non-function Identifier

---

## Bracket Handling

All three bracket types `()`, `{}`, `[]` are **functionally equivalent** — they all group sub-expressions:

```
(2+3) = {2+3} = [2+3] = 5
```

But they must be **properly nested**:

- ✅ `({2+3})` — valid
- ❌ `({2+3)}` — mismatched
- ❌ `(2+3` — unclosed

The bracket stack in the Parser enforces correct nesting. Each opening bracket pushes its type onto the bracket stack, and each closing bracket verifies the stack top matches.

---

## Error Handling

All errors throw `std::runtime_error` with descriptive messages. The `calculator()` entry point catches them and prints `"Error: ..."`.

### Error Categories

| Category      | Example Message                                               |
| ------------- | ------------------------------------------------------------- |
| **Lexer**     | `Unknown character '@' at position 5`                         |
| **Bracket**   | `Mismatched brackets: Expected closing for '(' but found ']'` |
| **Bracket**   | `Unmatched closing bracket ')' at position 7`                 |
| **Bracket**   | `Mismatched or unclosed brackets at end of expression`        |
| **Parser**    | `Unexpected token '=' at position 3`                          |
| **Evaluator** | `Invalid expression: Missing operand for unary minus`         |
| **Evaluator** | `Invalid expression: Missing operands for operator +`         |
| **Evaluator** | `Division by zero`                                            |
| **Evaluator** | `Domain error: sqrt of negative number`                       |
| **Evaluator** | `Unexpected identifier in evaluator: x`                       |
| **Evaluator** | `Invalid expression: Stack not empty after evaluation`        |
| **Variable**  | `Unknown variable: x`                                         |
| **Variable**  | `Cannot assign to reserved function 'sin'`                    |
| **Variable**  | `Expected expression for variable 'x'`                        |

---

## Entry Point: `calculator()`

```cpp
inline void calculator() {
    Calculator calc;
    // prints banner
    // REPL loop: read line → calc.process(line) → catch & print errors
    // exits on "exit", "quit", or EOF
}
```

This is the **public-facing free function** that creates a `Calculator` instance and runs an interactive REPL. It's called from the main pythonic library and provides a standalone calculator experience.

### REPL Session Example

```
CLI Calculator
Features: + - * / ^ ( ) { } [ ]
Functions: sin, cos, tan, log, sqrt, etc.
Variables: var a = 10, b = a*2
Type 'exit' or 'quit' to stop.
>> 2+3*4
14
>> var x = 10, y = x^2
Variable x = 10
Variable y = 100
>> sqrt(y) + x
20
>> sin(3.14159/2)
1
>> exit
```

---

## Supported Operations Reference

### Arithmetic Operators

| Operator | Meaning                | Precedence | Associativity | Example     |
| -------- | ---------------------- | :--------: | :-----------: | ----------- |
| `+`      | Addition               |     1      |     Left      | `3+2` → `5` |
| `-`      | Subtraction            |     1      |     Left      | `3-2` → `1` |
| `*`      | Multiplication         |     2      |     Left      | `3*2` → `6` |
| `/`      | Division               |     2      |     Left      | `6/2` → `3` |
| `^`      | Power                  |     3      |   **Right**   | `2^3` → `8` |
| `~`      | Unary minus (internal) |     4      |     Right     | `-5` → `-5` |

### Math Functions (Precedence 5)

| Category            | Functions                                                        |
| ------------------- | ---------------------------------------------------------------- |
| **Trigonometric**   | `sin(x)`, `cos(x)`, `tan(x)`                                     |
| **Reciprocal Trig** | `cot(x)`, `sec(x)`, `csc(x)`                                     |
| **Inverse Trig**    | `asin(x)`, `acos(x)`, `atan(x)`, `acot(x)`, `asec(x)`, `acsc(x)` |
| **Logarithmic**     | `log(x)` (natural), `log2(x)`, `log10(x)`                        |
| **Other**           | `sqrt(x)`, `abs(x)`                                              |

### Variable Operations

| Syntax             | Meaning                   |
| ------------------ | ------------------------- |
| `var x = expr`     | Declare and assign        |
| `x = expr`         | Assign (no `var` keyword) |
| `var a = 1, b = 2` | Multiple comma-separated  |
| `var a = 1. b = 2` | Multiple dot-separated    |

---

## Design Decisions & Trade-offs

### 1. Header-Only

The entire calculator is in a single `.hpp` file with `inline` on the entry point. This makes it trivially includable with no separate compilation required — ideal for a library component.

### 2. All Brackets Are Equal

`()`, `{}`, `[]` are interchangeable for grouping. This is unusual (most calculators only support `()`), but it's a deliberate design choice that allows mathematical notation like `{2+[3*4]}`.

### 3. Dot as Delimiter

Using `.` as a delimiter between variable assignments (`var a=1. b=2`) is clever but creates the **dot ambiguity** problem — is `.` a decimal point or a delimiter? The tokenizer resolves this by looking ahead: if the next character is a digit, it's a decimal; otherwise, it's a delimiter.

### 4. `double` Arithmetic Only

All values are `double`. There's no integer mode, no arbitrary precision, and no complex numbers. This keeps the evaluator simple but means:

- Integer division: `7/2` → `3.5` (not `3`)
- Large numbers may lose precision beyond 2^53

### 5. No Operator Overloading with `var` Type

Unlike ScriptIt (which uses `pythonic::vars::var`), the calculator uses raw `double` throughout. This is a standalone math tool, not part of the ScriptIt type system.

### 6. Variables Are Mutable and Session-Scoped

Variables persist across lines within a single `Calculator` instance. They can be reassigned freely. There's no concept of constants or scope — it's a flat global namespace.

### 7. Eager Variable Resolution

Variables are substituted by **text replacement** before parsing: the token's type changes from Identifier to Number and its value becomes `std::to_string(double)`. This is simple but means:

- Variable names are resolved at evaluation time, not parse time
- No forward references within a single expression (but works across comma-separated declarations)
- `std::to_string()` may introduce floating-point representation artifacts

---

_End of pythonicCalculator Internals Documentation_
