# 📜 ScriptIt v2

A dynamic scripting language powered by [pythonic](https://github.com/your-repo/pythonic)'s `var` type system. Write expressive scripts with strings, lists, sets, math, file I/O, and more — all with a clean dot-terminated syntax.

---

## Quick Start

### Build

```bash
g++ -std=c++20 -I/path/to/pythonic/include -o scriptit ScriptIt.cpp /path/to/pythonic/src/pythonicDispatchStubs.cpp
```

### Run a Script

```bash
./scriptit myprogram.sit
```

### Interactive REPL

```bash
./scriptit
```

Type expressions at the `>> ` prompt. Type `exit` to quit.

### Install System-Wide

```bash
sudo cp scriptit /usr/local/bin/
```

Then run from anywhere:

```bash
scriptit myprogram.sit
scriptit              # opens REPL
```

### Run Tests

```bash
python3 -m unittest test_interpreter
```

---

## Language Guide

### Statements End with `.`

The dot is your statement terminator:

```
1 + 1.
```

Output: `2`

> 💡 **Tip:** The dot can be omitted at the very end of a script or before block terminators (`;`, `elif`, `else`).

### Comments

```
--> This is a comment <--
--> Multi-line
    comments work too <--
```

---

## Data Types

ScriptIt supports rich dynamic types:

| Type    | Example                | Description                 |
| ------- | ---------------------- | --------------------------- |
| Integer | `42`                   | Whole numbers               |
| Float   | `3.14`                 | Decimal numbers             |
| String  | `"hello"` or `'world'` | Text                        |
| Boolean | `True`, `False`        | Logical values              |
| None    | `var x.`               | Absence of value            |
| List    | `[1, 2, 3]`            | Ordered collection          |
| Set     | `{1, 2, 3}`            | Unique unordered collection |

---

## Variables

### Declaration with `var`

```
var x = 10.
var name = "Alice".
var nums = [1, 2, 3].
x.                       --> 10
name.                    --> Alice
```

### Declaration with `let ... be`

```
let x be 42.
let greeting be "Hello".
x.                       --> 42
```

### Uninitialized Variables (None)

```
var x.                   --> x is None
var a, b, c.             --> all are None
```

### Reassignment

```
var x = 10.
x = 20.
x.                       --> 20
```

---

## Strings

### Literals

```
"double quoted".
'single quoted'.
```

### Escape Sequences

| Escape | Result    |
| ------ | --------- |
| `\n`   | newline   |
| `\t`   | tab       |
| `\\`   | backslash |
| `\"`   | quote     |

### Operations

```
"hello" + " " + "world".    --> hello world
"ab" * 3.                   --> ababab
3 * "xy".                   --> xyxyxy
"val=" + 42.                --> val=42

"abc" == "abc".              --> 1
"abc" != "xyz".              --> 1
len("hello").                --> 5
```

---

## Math

### Arithmetic

```
3 + 2.         --> 5
10 - 3.        --> 7
4 * 5.         --> 20
7 / 2.         --> 3.5
10 % 3.        --> 1
2 ^ 10.        --> 1024
-5.            --> -5
```

### Implicit Multiplication

```
3(4 + 1).      --> 15
2(3).          --> 6
```

### Precedence

Standard math precedence: `^` > `* / %` > `+ -`

```
2 + 3 * 4.     --> 14
(2 + 3) * 4.   --> 20
```

### Overflow Safety

Large numbers auto-promote — no silent overflow:

```
1000000 * 1000000.    --> 1000000000000
```

### Built-in Math Functions

```
sqrt(16).      --> 4
abs(-5).       --> 5
min(3, 7).     --> 3
max(3, 7).     --> 7
ceil(3.2).     --> 4
floor(3.8).    --> 3
round(3.5).    --> 4
```

**Trigonometry:** `sin`, `cos`, `tan`, `cot`, `sec`, `csc`
**Inverse trig:** `asin`, `acos`, `atan`
**Logarithms:** `log` (natural), `log2`, `log10`

### Constants

```
PI.            --> 3.14159265
e.             --> 2.7182818
```

---

## Comparisons & Logic

Comparisons return `1` (true) or `0` (false):

```
(5 > 3).             --> 1
(1 == 2).            --> 0
(1 != 2).            --> 1
("abc" == "abc").    --> 1
```

### Logical Operators

Use **words** or **symbols**:

```
(1 == 1) and (2 == 2).     --> 1
(1 == 0) or (2 == 2).      --> 1
not (1 == 0).               --> 1

--> Same with symbols:
(1 == 1) && (2 == 2).      --> 1
(1 == 0) || (2 == 2).      --> 1
!(1 == 0).                  --> 1
```

### Short-Circuit Evaluation

```
(1 == 1) || (10 / 0).    --> 1 (no crash — right side skipped!)
(1 == 0) && (10 / 0).    --> 0 (no crash!)
```

---

## Collections

### Lists

```
var nums = [1, 2, 3].
print(nums).                --> [1, 2, 3]
len(nums).                  --> 3

var more = append(nums, 4).
print(more).                --> [1, 2, 3, 4]

var last = pop(more).
last.                       --> 4

var empty = list().
```

### Sets

```
var s = {1, 2, 3, 2}.
print(s).                   --> {1, 2, 3}

var empty = set().
```

### Mixed Types

```
var mixed = [1, "hello", 3.14].
print(mixed).               --> [1, hello, 3.14]
```

---

## Control Flow

### If / Elif / Else

Blocks start with `:` and end with `;`

```
var x = 15.

if x > 20:
    print("big").
elif x > 10:
    print("medium").
else:
    print("small").
;
```

Output: `medium`

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

### For Loop (Range)

```
for i in range(from 1 to 5):
    print(i).
;
--> prints 1, 2, 3, 4, 5
```

Reverse ranges:

```
for i in range(from 5 to 1):
    print(i).
;
--> prints 5, 4, 3, 2, 1
```

### For-In Loop (New in v2)

Iterate over lists, strings, or sets:

```
for x in [10, 20, 30]:
    print(x).
;
--> prints 10, 20, 30

for ch in "abc":
    print(ch).
;
--> prints a, b, c

var names = ["Alice", "Bob"].
for name in names:
    print("Hello " + name).
;
--> Hello Alice
--> Hello Bob
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

### Strings in Functions

```
fn greet @(name):
    let msg be "Hello " + name.
    give(msg).
;

greet("World").     --> Hello World
```

### Lists in Functions

```
fn sum_list @(items):
    var total = 0.
    for x in items:
        total = total + x.
    ;
    give(total).
;

sum_list([1, 2, 3, 4]).    --> 10
```

### No Return = Returns None

```
fn side_effect @():
    var x = 1.
;
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

---

## Built-in Functions

### I/O

| Function      | Description                  |
| ------------- | ---------------------------- |
| `print(a, b)` | Print space-separated values |
| `pprint(x)`   | Pretty-print with type info  |
| `input()`     | Read a line from stdin       |
| `input("? ")` | Read with prompt             |

### File I/O

```
write("/tmp/data.txt", "Hello!").
var content = read("/tmp/data.txt").
print(content).                        --> Hello!

var lines = readLine("/tmp/data.txt").
print(len(lines)).                     --> 1
```

### Type Functions

| Function   | Description                   | Example            |
| ---------- | ----------------------------- | ------------------ |
| `type(x)`  | Type name as string           | `type(42)` → `int` |
| `str(x)`   | Convert to string             | `str(42)` → `"42"` |
| `int(x)`   | Convert to integer (truncate) | `int(3.7)` → `3`   |
| `float(x)` | Convert to float              | `float(5)` → `5.0` |
| `len(x)`   | Length of string/list/set     | `len("hi")` → `2`  |

### Collection Functions

| Function         | Description                  |
| ---------------- | ---------------------------- |
| `list()`         | Create empty list            |
| `set()`          | Create empty set             |
| `append(lst, x)` | Append item, return new list |
| `pop(lst)`       | Remove and return last item  |

---

## REPL Commands

| Command | Description                                    |
| ------- | ---------------------------------------------- |
| `exit`  | Quit the REPL                                  |
| `clear` | Clear the terminal screen                      |
| `wipe`  | Clear screen AND reset all state (fresh start) |

---

## Script Files (`.sit`)

Save your code in a `.sit` file:

```
--> hello.sit <--
let name be "World".
print("Hello " + name).
```

Run it:

```bash
scriptit hello.sit
```

Output: `Hello World`

---

## Pass

Use `pass` for empty blocks:

```
if 1 == 0:
    pass.
;
```

---

## Error Messages

| What happened            | Message                           |
| ------------------------ | --------------------------------- |
| `1 / 0.`                 | `Error: Division by zero`         |
| `noVar.`                 | `Error: Undefined variable noVar` |
| `fn f @(a): pass. ; f()` | `Error: Arity mismatch`           |
| Missing dot in middle    | `Error: Expected '.' ...`         |
| `"hello" - 1.`           | `Error: ...` (invalid operation)  |

---

## Complete Example

```
--> A complete ScriptIt v2 program <--

let names be ["Alice", "Bob", "Charlie"].
var scores = [95, 87, 92].

fn average @(nums):
    var total = 0.
    for n in nums:
        total = total + n.
    ;
    give(total / len(nums)).
;

print("Students:").
for name in names:
    print("  " + name).
;

print("Average score: " + str(average(scores))).

if average(scores) > 90:
    print("Class is doing great!").
else:
    print("Keep working hard!").
;
```

Output:

```
Students:
  Alice
  Bob
  Charlie
Average score: 91.3333
Class is doing great!
```
