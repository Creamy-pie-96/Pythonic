# 📜 ScriptIt

A custom scripting language. Here's how to use it.

---

## Getting Started

### Build

```bash
g++ -std=c++17 -o scriptit ScriptIt.cpp
```

### Interactive Mode (REPL)

```bash
./scriptit
```

Type expressions at the `>> ` prompt. Type `exit` to quit.

### Run a Script File

```bash
cat my_program.txt | ./scriptit --script
```

### Run Tests

```bash
python3 test_interpreter.py
```

---

## Language Guide

### Statements End with `.`

The dot is your semicolon:

```
1 + 1.
```

Output: `2`

### Comments

```
--> This is a comment <--
--> Multi-line
    comments work too <--
```

---

## Math

```
3 + 2.         --> 5
10 - 3.        --> 7
4 * 5.         --> 20
7 / 2.         --> 3.5
10 % 3.        --> 1
2 ^ 10.        --> 1024
-5.            --> -5
```

**Precedence** works like regular math: `*` before `+`, use `()` to override:

```
2 + 3 * 4.     --> 14
(2 + 3) * 4.   --> 20
```

**Built-in functions**: `sin`, `cos`, `tan`, `sqrt`, `abs`, `log`, `min`, `max`, `ceil`, `floor`, `round`

```
sqrt(16).      --> 4
min(3, 7).     --> 3
abs(-5).       --> 5
```

**Constants**: `PI` (3.14159265), `e` (2.7182818)

---

## Variables

```
var x = 10.          --> declare a variable
x.                   --> prints 10
x = 20.              --> reassign
x.                   --> prints 20
```

---

## Comparisons & Logic

Comparisons return `1` (true) or `0` (false):

```
(5 > 3).             --> 1
(1 == 2).            --> 0
(1 != 2).            --> 1
```

Logical operators — use **words** or **symbols**:

```
(1 == 1) and (2 == 2).     --> 1
(1 == 0) or (2 == 2).      --> 1
not (1 == 0).               --> 1

--> Same thing with symbols:
(1 == 1) && (2 == 2).      --> 1
(1 == 0) || (2 == 2).      --> 1
!(1 == 0).                  --> 1
```

**Short-circuit**: the right side is skipped if the left side already determines the result:

```
(1 == 1) || (10 / 0).    --> 1 (no crash!)
(1 == 0) && (10 / 0).    --> 0 (no crash!)
```

---

## If / Elif / Else

Blocks start with `:` and end with `;`

```
var x = 15.

if x > 20:
    1.
elif x > 10:
    2.
else:
    3.
;
```

Output: `2`

---

## Loops

### While Loop

```
var n = 5.
var sum = 0.

while n > 0:
    sum = sum + n.
    n = n - 1.
;

sum.               --> 15
```

### For Loop

```
for i in range(from 1 to 5):
    i.
;
--> prints 1, 2, 3, 4, 5
```

Reverse ranges work:

```
for i in range(from 5 to 1):
    i.
;
--> prints 5, 4, 3, 2, 1
```

---

## Functions

### Define and Call

```
fn add @(a, b):
    give(a + b).
;

add(3, 4).         --> 7
```

- `fn` — starts a function
- `@(params)` — parameters
- `give(value)` — returns a value
- `:` starts the body, `;` ends it

### No Return = Returns 0

```
fn nothing @():
    var x = 1.
;
nothing().         --> 0
```

### Recursion

```
fn factorial @(n):
    if n <= 1:
        give(1).
    ;
    give(n * factorial(n - 1)).
;

factorial(5).      --> 120
```

### Nested Functions

```
fn outer @(x):
    fn inner @(y):
        give(x + y).
    ;
    give(inner(10)).
;

outer(5).          --> 15
```

---

## Scope Rules

| Context         | Can read outer vars? | Can change outer vars? |
| --------------- | -------------------- | ---------------------- |
| Functions       | ✅ Yes               | ❌ No                  |
| If/Else blocks  | ✅ Yes               | ✅ Yes                 |
| For/While loops | ✅ Yes               | ✅ Yes                 |

```
var x = 10.

fn noMutate @():
    x = 99.          --> Error! Functions can't change outer vars
;

if 1 == 1:
    x = 99.          --> OK! If-blocks can
;
x.                   --> 99
```

---

## Pass

Use `pass` for an empty block:

```
if 1 == 0:
    pass.
;
```

---

## Errors

| What happened             | Message                           |
| ------------------------- | --------------------------------- |
| `1 / 0.`                  | `Error: Div by 0`                 |
| `noVar.`                  | `Error: Undefined variable noVar` |
| `fn f @(a): pass. ; f().` | `Error: Arity mismatch`           |
| Missing dot terminator    | `Error: Expected . ...`           |
