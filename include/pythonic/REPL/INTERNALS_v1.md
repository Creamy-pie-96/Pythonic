# 🔬 How MyLang Works — Interpreter Internals

This document explains how the interpreter transforms your code into output, step by step. Even if you've never built a language before, you'll understand the full pipeline by the end.

---

## The Big Picture

When you write `2 + 3.`, the interpreter runs through **4 stages** to produce `5`:

```
Source Code  →  [Tokenizer]  →  [Parser]  →  [Evaluator]  →  Output
  "2 + 3."     Tokens          AST Tree      Walks tree       "5"
```

Let's walk through each stage.

---

## Stage 1: Tokenizer (Lexer)

**File**: `me_doingIt.cpp` → `class Tokenizer`

The tokenizer reads raw text character by character and breaks it into **tokens** — small meaningful pieces. Think of it like breaking a sentence into words.

### Example

Input: `var x = 10 + 3.`

Tokens produced:

```
[KeywordVar: "var"]  [Identifier: "x"]  [Equals: "="]  [Number: "10"]
[Operator: "+"]  [Number: "3"]  [Dot: "."]  [Eof]
```

### How It Works

The tokenizer uses a `while` loop that walks through the source string one character at a time:

```
Position:  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
Source:    v  a  r     x     =     1  0     +     3  .
```

For each character it asks:

1. **Is it a digit?** → Keep reading digits to form a `Number` token (`10`)
2. **Is it a letter?** → Keep reading letters to form a word, then check:
   - Is it a keyword (`var`, `fn`, `if`, `while`, etc.)? → Keyword token
   - Is it a logical operator (`and`, `or`, `not`)? → Operator token
   - Otherwise? → `Identifier` token (a variable/function name)
3. **Is it a symbol?** → Match single or double-character operators (`+`, `==`, `&&`, `<=`, etc.)
4. **Is it `-->`?** → Skip everything until `<--` (comment)
5. **Whitespace?** → Skip it

### Important Detail: The Dot Ambiguity

The character `.` serves **two purposes**:

- **Statement terminator**: `x.` means "end of statement, print x"
- **Decimal point**: `3.14` is a floating-point number

The tokenizer resolves this by checking: _Is the next character after `.` a digit?_ If yes → it's part of a decimal number. If no → it's a terminator.

```
"3.14."  →  [Number: "3.14"]  [Dot: "."]
"10."    →  [Number: "10"]    [Dot: "."]
```

---

## Stage 2: Parser

**File**: `me_doingIt.cpp` → `class Parser`

The parser reads the flat list of tokens and builds a **tree structure** called an **AST** (Abstract Syntax Tree). This tree represents the logical structure of your program.

### What the Parser Produces

For this code:

```
if x > 10:
    x.
;
```

The parser creates this tree:

```
IfStmt
├── condition: Expression [x > 10]
├── then-block: BlockStmt
│   └── ExprStmt
│       └── Expression [x]
└── else-block: (none)
```

### Statement Types

The parser knows how to recognize these statement patterns:

| Statement            | Pattern                                  | Produced Node                     |
| -------------------- | ---------------------------------------- | --------------------------------- |
| Variable declaration | `var NAME = EXPR.`                       | `AssignStmt` (isDeclaration=true) |
| Assignment           | `NAME = EXPR.`                           | `AssignStmt`                      |
| Function definition  | `fn NAME @(PARAMS): BODY ;`              | `FunctionDefStmt`                 |
| If/elif/else         | `if EXPR: BODY ; [elif...] [else...]`    | `IfStmt`                          |
| While loop           | `while EXPR: BODY ;`                     | `WhileStmt`                       |
| For loop             | `for NAME in range(from X to Y): BODY ;` | `ForStmt`                         |
| Return               | `give(EXPR).`                            | `ReturnStmt`                      |
| Pass                 | `pass.`                                  | `PassStmt`                        |
| Expression           | `EXPR.`                                  | `ExprStmt` (prints the result)    |

### How Expression Parsing Works: The Shunting-Yard Algorithm

This is the most complex part. Expressions like `2 + 3 * 4` need to respect operator precedence (`*` before `+`). The parser uses the **Shunting-Yard Algorithm** (invented by Edsger Dijkstra) to convert infix notation to **RPN** (Reverse Polish Notation).

#### What is RPN?

Normal math (infix): `2 + 3 * 4`
RPN (postfix): `2 3 4 * +`

In RPN, operators come **after** their operands. The beauty: **no parentheses needed** and evaluation is trivially simple with a stack.

#### The Algorithm

Uses two data structures: an **output queue** and an **operator stack**.

```
Input tokens:  2  +  3  *  4

Step 1: "2" is a number → push to output
        Output: [2]   Stack: []

Step 2: "+" is an operator → push to stack
        Output: [2]   Stack: [+]

Step 3: "3" is a number → push to output
        Output: [2, 3]   Stack: [+]

Step 4: "*" is an operator → precedence of * (6) > + (5)
        So * goes on top, + stays
        Output: [2, 3]   Stack: [+, *]

Step 5: "4" is a number → push to output
        Output: [2, 3, 4]   Stack: [+, *]

Step 6: End of input → pop all operators to output
        Output: [2, 3, 4, *, +]   Stack: []
```

Result RPN: `2 3 4 * +` ✓

#### How Parentheses Work

`(2 + 3) * 4`:

- `(` → pushed to stack as marker
- `2 + 3` processed normally
- `)` → pop operators until `(` is found, removing the marker
- `*` → normal processing

Result: `2 3 + 4 *` ✓ (addition happens first)

#### Unary Operators

`-5` is tricky because `-` could be subtraction or negation. The parser checks: was the **previous token** an operator, opening paren, or nothing? If so, it's unary.

Unary `-` is renamed to `~` internally so the evaluator can distinguish:

- `-` with two operands = subtraction
- `~` with one operand = negation

Unary `!` stays as `!`.

### Short-Circuit Evaluation (the Tricky Part)

`&&` and `||` need **lazy evaluation** — the right side shouldn't run if the left side already determines the result. But RPN evaluates everything eagerly!

**Solution**: The parser has **three layers**:

```
parseExpression()     → calls parseLogicalOr()
parseLogicalOr()      → calls parseLogicalAnd(), handles ||
parseLogicalAnd()     → calls parsePrimaryExpr(), handles &&
parsePrimaryExpr()    → Shunting-Yard for everything else
```

When `||` or `&&` appears **at the top level** (not inside parentheses), the parser **doesn't** put them in the RPN. Instead, it creates a tree node:

```
Expression
├── logicalOp: "||"
├── lhs: Expression [left side - RPN]
└── rhs: Expression [right side - RPN]
```

The evaluator then checks the LHS first, and **only evaluates RHS if needed**:

```cpp
if (logicalOp == "&&") {
    double leftVal = lhs->evaluate(scope);
    if (leftVal == 0) return 0.0;     // Short-circuit: skip RHS!
    return rhs->evaluate(scope);       // Only evaluate if LHS was true
}
```

---

## Stage 3: Evaluator

**File**: `me_doingIt.cpp` → `Expression::evaluate()` and `*.execute()` methods

### Expression Evaluation (RPN Stack Machine)

Evaluating RPN is beautifully simple. Use a **stack**:

```
RPN: 2 3 4 * +

Step 1: "2" → push          Stack: [2]
Step 2: "3" → push          Stack: [2, 3]
Step 3: "4" → push          Stack: [2, 3, 4]
Step 4: "*" → pop 4 and 3,
              push 3*4=12   Stack: [2, 12]
Step 5: "+" → pop 12 and 2,
              push 2+12=14  Stack: [14]

Result: 14 ✓
```

### Statement Execution

Each AST node has an `execute()` method:

- **ExprStmt**: Evaluates the expression and **prints** the result
- **AssignStmt**: Evaluates the expression, stores the result in the scope
- **IfStmt**: Evaluates condition → if non-zero, executes the matching branch's block
- **WhileStmt**: Evaluates condition → while non-zero, executes body, re-evaluates condition
- **ForStmt**: Determines range → iterates, setting loop variable in scope for each iteration
- **FunctionDefStmt**: Stores the function definition in the scope (does not run it yet)
- **ReturnStmt**: Evaluates expression, throws `ReturnException` with the value

### How `give` (Return) Works

`give(value)` throws a C++ exception (`ReturnException`). This exception **unwinds** through any nested loops, if-blocks, etc., until it's caught by the function call code in the evaluator. This is why `give` correctly exits from inside while loops:

```
fn find @():
    var i = 0.
    while i < 100:        ← loop running
        if i == 42:
            give(i).      ← throws ReturnException(42)
        ;                    ← exception flies through if-block
        i = i + 1.
    ;                        ← exception flies through while-loop
;                            ← caught here by function call handler
```

### Function Calls

When the evaluator encounters a function call in an expression:

1. **Pop arguments** from the stack
2. **Create a new scope** (child of caller's scope, with barrier)
3. **Define parameters** as local variables in the new scope
4. **Execute** the function body
5. **Catch** any `ReturnException` → push the return value onto the stack
6. If no `give` was used → push `0` (implicit return)

---

## Stage 4: Scope System

**File**: `me_doingIt.cpp` → `struct Scope`

The scope system controls **which variables are visible** and **which can be modified**. It's implemented as a **linked list** of scope frames.

### Scope Chain

```
Global Scope       ← defines: x=10, PI=3.14
  │
  ├── Function Scope (barrier=true)  ← defines: a=5 (parameter)
  │     │
  │     └── If-Block Scope (barrier=false)  ← defines: temp=1
  │
  └── For-Loop Scope (barrier=false)  ← defines: i=3 (loop var)
```

### The Barrier Mechanism

Each scope has a `barrier` flag:

- **`barrier = false`** (if/else, for, while blocks): The `set()` method **propagates** writes to the parent scope. So `x = 99` inside an if-block modifies the outer `x`.

- **`barrier = true`** (function scopes): The `set()` method **stops** at the barrier. So `x = 99` inside a function throws an error — it can't reach the outer `x`.

### Variable Lookup (`get`)

When reading variable `x`, the scope walks **up** the chain:

```
Current scope → has x? → Yes → return it
                       → No  → check parent → has x? → Yes → return it
                                             → No    → check parent → ...
                                                                    → Error!
```

There's **no barrier for reading** — functions can always read outer variables. Only writing is blocked.

### Variable Assignment (`set`)

When writing `x = value`:

```
Current scope → has x? → Yes → update it
                       → No  → barrier? → Yes → ERROR ("cannot mutate outer scope")
                                         → No  → try parent.set(x, value)
```

---

## Putting It All Together

Here's the full journey of this program:

```
var x = 5.
fn double @(n): give(n * 2). ;
double(x).
```

### 1. Tokenizer

```
[var] [x] [=] [5] [.] [fn] [double] [@] [(] [n] [)] [:] [give] [(] [n] [*] [2] [)] [.] [;] [double] [(] [x] [)] [.]
```

### 2. Parser

```
Program (BlockStmt)
├── AssignStmt { name="x", expr=RPN[5], isDeclaration=true }
├── FunctionDefStmt { name="double", params=["n"],
│       body=BlockStmt [
│           ReturnStmt { expr=RPN[n, 2, *] }
│       ]
│   }
└── ExprStmt { expr=RPN[x, double CALL(1)] }
```

### 3. Evaluator

```
1. AssignStmt: evaluate RPN[5] → 5, store x=5 in global scope
2. FunctionDefStmt: store "double" function definition in scope
3. ExprStmt: evaluate RPN[x, double CALL(1)]
   a. Push x → stack: [5]
   b. CALL double with 1 arg
      - Pop 5 from stack
      - Create new scope with n=5
      - Execute body: evaluate RPN[n, 2, *]
        - Push n=5, push 2 → stack: [5, 2]
        - Pop 2, pop 5, push 10 → stack: [10]
        - ReturnStmt throws ReturnException(10)
      - Catch → push 10 to stack
   c. Stack: [10]
   d. Print: 10
```

**Output**: `10`

---

## Summary of Key Design Decisions

| Decision                  | Choice                                  | Why                                                                                            |
| ------------------------- | --------------------------------------- | ---------------------------------------------------------------------------------------------- |
| Expression representation | RPN (Reverse Polish Notation)           | Simple stack-based evaluation, no recursion needed                                             |
| Short-circuit `&&`/`\|\|` | Tree nodes wrapping RPN sub-expressions | Can't lazily evaluate inside flat RPN, so logical ops are lifted to tree layer                 |
| Scope model               | Dynamic scope with barriers             | Simple, satisfies "inner functions can read outer vars" while preventing mutation              |
| Return mechanism          | C++ exceptions (`ReturnException`)      | Cleanly unwinds through nested loops and blocks without adding return-checking code everywhere |
| Statement terminator      | `.` (dot)                               | Chosen by language designer as a visual alternative to `;`                                     |
| Function syntax           | `fn NAME @(PARAMS): BODY ;`             | `@` is a visual separator, `:` and `;` delimit the body                                        |
