# 📜 ScriptIt v0.3.0 — Language Guide

A dynamic scripting language powered by [pythonic](https://github.com/BenCaunt/pythonic)'s `var` type system. Write expressive scripts with strings, lists, sets, math, file I/O, functions, and more — all with a clean dot-terminated syntax.

**File extension:** `.sit`

---

## Table of Contents

- [Quick Start](#quick-start)
- [Syntax Rules](#syntax-rules)
- [Data Types](#data-types)
- [Variables](#variables)
- [Operators](#operators)
- [Strings](#strings)
- [Lists](#lists)
- [Sets](#sets)
- [Printing](#printing)
- [Functions](#functions)
- [Control Flow](#control-flow)
- [The `of` Keyword](#the-of-keyword)
- [Math Functions](#math-functions)
- [Builtin Functions](#builtin-functions)
- [Type Conversion](#type-conversion)
- [Type Methods](#type-methods-on-any-value)
- [File I/O](#file-io)
- [Overflow Promotion](#overflow-promotion)
- [Notebook Mode](#notebook-mode)
- [REPL Commands](#repl-commands)
- [Error Messages](#error-messages)
- [Complete Example](#complete-example)

---

## Quick Start

### Build

```bash
g++ -std=c++20 -I/path/to/pythonic/include -o scriptit ScriptIt.cpp /path/to/pythonic/src/pythonicDispatchStubs.cpp -O2
```

### Run a Script

```bash
./scriptit myprogram.sit
```

### Interactive REPL

```bash
./scriptit
```

Type expressions at the `>> ` prompt. Type `exit` to quit, `clear` to clear the screen, or `wipe` for a fresh start (clears screen **and** resets all variables/functions).

### Install System-Wide

```bash
sudo cp scriptit /usr/local/bin/
```

Then run from anywhere:

```bash
scriptit myprogram.sit    # run a file
scriptit                  # open REPL
```

### Run Tests

```bash
python3 -m unittest test_interpreter
```

---

## Syntax Rules

### Statements End with `.` (dot)

The dot is your statement terminator — think of it as the period at the end of a sentence:

```
var x = 10.
print(x).
```

> 💡 The dot is **optional** at the end of a file, or right before a block terminator (`;`, `elif`, `else`).

### Newlines as Terminators

Newlines can also act as statement terminators, so this works too:

```
var x = 10
print(x)
```

### Blocks: `:` and `;`

Code blocks (if, for, while, functions) start with `:` and end with `;`

```
if x > 5:
    print("big")
;
```

### Comments

Use `-->` to open a comment and `<--` to close it. Comments can span multiple lines:

```
--> This is a single-line comment <--

--> This is a
    multi-line comment
    that spans several lines <--
```

### Line Continuation

Use a backtick (`` ` ``) before a newline to join it with the next line:

```
var result = 1 + 2 + 3 `
           + 4 + 5 + 6.
print(result).          --> 21
```

---

## Data Types

ScriptIt is dynamically typed. Every value has one of these types:

| Type    | Examples                      | Description                   |
| ------- | ----------------------------- | ----------------------------- |
| Integer | `42`, `-7`, `0`               | Whole numbers                 |
| Float   | `3.14`, `.5`, `-0.001`        | Decimal numbers               |
| String  | `"hello"`, `'world'`          | Text (single or double quote) |
| Boolean | `True`, `False`               | Logical values                |
| None    | _(default for uninitialized)_ | Absence of value              |
| List    | `[1, 2, 3]`, `["a", "b"]`     | Ordered collection            |
| Set     | `{1, 2, 3}`                   | Unique unordered collection   |

---

## Variables

### Declaration with `var`

```
var x = 10.
var name = "Alice".
var nums = [1, 2, 3].
```

### Uninitialized Variables

Declaring without a value sets the variable to `None`:

```
var x.                   --> x is None
```

### Multi-Variable Declaration

Declare multiple variables in a single statement:

```
var a = 1 b = 2 c = 3.
```

### Declaration with `let ... be`

An alternative, more readable way to declare variables:

```
let x be 42.
let greeting be "Hello".
```

### Assignment

```
var x = 10.
x = 20.
x.                       --> 20
```

### Compound Assignment

```
x += 1.                  --> add 1
x -= 2.                  --> subtract 2
x *= 3.                  --> multiply by 3
x /= 4.                  --> divide by 4
x %= 5.                  --> remainder after dividing by 5
```

### Increment and Decrement

Both prefix and postfix forms are supported:

```
var x = 5.
x++.                     --> x is now 6
x--.                     --> x is now 5 again
++x.                     --> x is now 6 (prefix)
--x.                     --> x is now 5 (prefix)
```

---

## Operators

### Arithmetic

| Operator | Meaning        | Example           |
| -------- | -------------- | ----------------- |
| `+`      | Addition       | `3 + 2` → `5`     |
| `-`      | Subtraction    | `10 - 3` → `7`    |
| `*`      | Multiplication | `4 * 5` → `20`    |
| `/`      | Division       | `7 / 2` → `3.5`   |
| `%`      | Modulo         | `10 % 3` → `1`    |
| `^`      | Exponent       | `2 ^ 10` → `1024` |

### Comparison

| Operator | Meaning               |
| -------- | --------------------- |
| `==`     | Equal                 |
| `!=`     | Not equal             |
| `<`      | Less than             |
| `>`      | Greater than          |
| `<=`     | Less than or equal    |
| `>=`     | Greater than or equal |

Comparisons return `1` (true) or `0` (false):

```
(5 > 3).                 --> 1
(1 == 2).                --> 0
("abc" == "abc").        --> 1
```

### Identity Operators: `is`, `is not`

Value equality — same as `==` and `!=` in ScriptIt:

```
var x = 5.
(x is 5).                --> 1
(x is not 10).           --> 1
```

### Reference Operators: `points`, `not points`

Strict type **and** value identity — no tolerance, no type coercion:

```
var x = 5.
(x points 5).            --> 1
(x not points 5.0).      --> 1  (different types!)
```

### Logical Operators

Use **words** or **symbols** — both work identically:

| Word  | Symbol | Meaning     |
| ----- | ------ | ----------- |
| `and` | `&&`   | Logical AND |
| `or`  | `\|\|` | Logical OR  |
| `not` | `!`    | Logical NOT |

```
(1 == 1) and (2 == 2).   --> 1
(1 == 0) or (2 == 2).    --> 1
not (1 == 0).             --> 1

--> Same with symbols:
(1 == 1) && (2 == 2).    --> 1
(1 == 0) || (2 == 2).    --> 1
!(1 == 0).                --> 1
```

**Short-circuit evaluation** — the right side is skipped when unnecessary:

```
(1 == 1) || (10 / 0).    --> 1 (no crash!)
(1 == 0) && (10 / 0).    --> 0 (no crash!)
```

### Implicit Multiplication

ScriptIt supports implicit multiplication when two values are adjacent without an operator:

```
var x = 5.
3x.                      --> 15      (3 * x)
2(3 + 4).                --> 14      (2 * (3 + 4))
x y.                     --> x * y
(2 + 1)(3 + 1).          --> 12      ((2+1) * (3+1))
```

---

## Strings

### Literals

Both single and double quotes work:

```
"double quoted".
'single quoted'.
```

### Escape Sequences

| Escape | Result       |
| ------ | ------------ |
| `\n`   | Newline      |
| `\t`   | Tab          |
| `\\`   | Backslash    |
| `\"`   | Double quote |
| `\'`   | Single quote |

### Concatenation

```
"hello" + " " + "world".      --> hello world
"value=" + 42.                 --> value=42
```

### Repetition

```
"ha" * 3.                      --> hahaha
3 * "xy".                      --> xyxyxy
```

### String Methods

Strings have a rich set of dot-methods:

**Zero-argument methods:**

| Method          | Description                        | Example                                   |
| --------------- | ---------------------------------- | ----------------------------------------- |
| `.upper()`      | Convert to uppercase               | `"hello".upper()` → `"HELLO"`             |
| `.lower()`      | Convert to lowercase               | `"HELLO".lower()` → `"hello"`             |
| `.strip()`      | Remove leading/trailing whitespace | `"  hi  ".strip()` → `"hi"`               |
| `.lstrip()`     | Remove leading whitespace          | `"  hi".lstrip()` → `"hi"`                |
| `.rstrip()`     | Remove trailing whitespace         | `"hi  ".rstrip()` → `"hi"`                |
| `.title()`      | Title Case                         | `"hello world".title()` → `"Hello World"` |
| `.capitalize()` | Capitalize first letter            | `"hello".capitalize()` → `"Hello"`        |
| `.reverse()`    | Reverse the string                 | `"abc".reverse()` → `"cba"`               |
| `.split()`      | Split on whitespace                | `"a b c".split()` → `["a","b","c"]`       |
| `.isdigit()`    | All characters are digits?         | `"123".isdigit()` → `True`                |
| `.isalpha()`    | All characters are letters?        | `"abc".isalpha()` → `True`                |
| `.isalnum()`    | All characters are alphanumeric?   | `"abc1".isalnum()` → `True`               |
| `.isspace()`    | All characters are whitespace?     | `"  ".isspace()` → `True`                 |
| `.len()`        | Length of the string               | `"hello".len()` → `5`                     |
| `.type()`       | Type name                          | `"hi".type()` → `"string"`                |
| `.str()`        | String representation              | `"hi".str()` → `"hi"`                     |
| `.empty()`      | Is the string empty?               | `"".empty()` → `True`                     |

**One-argument methods:**

| Method           | Description                              | Example                                |
| ---------------- | ---------------------------------------- | -------------------------------------- |
| `.find(sub)`     | Index of first occurrence (-1 if absent) | `"hello".find("ll")` → `2`             |
| `.count(sub)`    | Count occurrences                        | `"banana".count("a")` → `3`            |
| `.contains(sub)` | Does it contain the substring?           | `"hello".contains("ell")` → `True`     |
| `.startswith(s)` | Starts with prefix?                      | `"hello".startswith("he")` → `True`    |
| `.endswith(s)`   | Ends with suffix?                        | `"hello".endswith("lo")` → `True`      |
| `.split(sep)`    | Split on separator                       | `"a-b-c".split("-")` → `["a","b","c"]` |
| `.join(list)`    | Join list elements                       | `"-".join(["a","b","c"])` → `"a-b-c"`  |
| `.zfill(width)`  | Pad with zeros                           | `"42".zfill(5)` → `"00042"`            |

**Two-argument methods:**

| Method                 | Description             | Example                                 |
| ---------------------- | ----------------------- | --------------------------------------- |
| `.replace(old, new)`   | Replace all occurrences | `"hello".replace("l", "r")` → `"herro"` |
| `.slice(start, end)`   | Substring by index      | `"hello".slice(1, 4)` → `"ell"`         |
| `.center(width, fill)` | Center with fill char   | `"hi".center(10, "-")` → `"----hi----"` |

---

## Lists

### Creating Lists

```
var nums = [1, 2, 3].
var mixed = [1, "hello", 3.14, True].
var empty = list().
```

### Concatenation and Repetition

```
[1, 2] + [3, 4].              --> [1, 2, 3, 4]
[0] * 5.                      --> [0, 0, 0, 0, 0]
```

### Indexing

```
var fruits = ["apple", "banana", "cherry"].
fruits[0].                     --> apple
fruits[2].                     --> cherry
```

### List Methods

**Zero-argument methods:**

| Method       | Description                    | Example                         |
| ------------ | ------------------------------ | ------------------------------- |
| `.pop()`     | Remove and return last element | `[1,2,3].pop()` → `3`           |
| `.clear()`   | Remove all elements            | `nums.clear()`                  |
| `.sort()`    | Sort in place                  | `[3,1,2].sort()` → `[1,2,3]`    |
| `.reverse()` | Reverse in place               | `[1,2,3].reverse()` → `[3,2,1]` |
| `.front()`   | First element                  | `[1,2,3].front()` → `1`         |
| `.back()`    | Last element                   | `[1,2,3].back()` → `3`          |
| `.empty()`   | Is the list empty?             | `[].empty()` → `True`           |
| `.size()`    | Number of elements             | `[1,2,3].size()` → `3`          |

**One-argument methods:**

| Method           | Description                              | Example                        |
| ---------------- | ---------------------------------------- | ------------------------------ |
| `.append(x)`     | Add element to end                       | `nums.append(4)`               |
| `.extend(other)` | Add all elements from another list       | `nums.extend([4, 5])`          |
| `.remove(val)`   | Remove first occurrence of value         | `nums.remove(2)`               |
| `.contains(val)` | Does the list contain the value?         | `[1,2,3].contains(2)` → `True` |
| `.index(val)`    | Index of first occurrence (-1 if absent) | `[1,2,3].index(2)` → `1`       |
| `.count(val)`    | Count occurrences of value               | `[1,2,2,3].count(2)` → `2`     |

**Two-argument methods:**

| Method                | Description        | Example                           |
| --------------------- | ------------------ | --------------------------------- |
| `.insert(index, val)` | Insert at position | `nums.insert(0, 99)`              |
| `.slice(start, end)`  | Sub-list by index  | `[1,2,3,4].slice(1, 3)` → `[2,3]` |

---

## Sets

### Creating Sets

Sets contain only unique values. Duplicates are automatically removed:

```
var s = {1, 2, 3, 2}.
print(s).                      --> {1, 2, 3}
var empty = set().
```

### Set Methods

| Method           | Description                         | Example                        |
| ---------------- | ----------------------------------- | ------------------------------ |
| `.add(x)`        | Add an element                      | `s.add(4)`                     |
| `.remove(x)`     | Remove an element (error if absent) | `s.remove(2)`                  |
| `.contains(x)`   | Check membership                    | `{1,2,3}.contains(2)` → `True` |
| `.clear()`       | Remove all elements                 | `s.clear()`                    |
| `.empty()`       | Is the set empty?                   | `{}.empty()` → `True`          |
| `.size()`        | Number of elements                  | `{1,2,3}.size()` → `3`         |
| `.update(other)` | Add all elements from another set   | `s.update({4, 5})`             |

---

## Printing

### `print()`

Prints values followed by a newline. Multiple arguments are space-separated:

```
print("hello").                --> hello
print("x =", 42).             --> x = 42
print(1, 2, 3).               --> 1 2 3
```

### `pprint()`

Pretty-print with type information:

```
pprint(42).                    --> 42 (int)
pprint([1, 2, 3]).             --> [1, 2, 3] (list)
```

### Auto-Print for Expressions

Any expression statement that evaluates to a non-None value is automatically printed. This is especially useful in the REPL:

```
1 + 1.                         --> 2
"hello".upper().               --> HELLO
sqrt(16).                      --> 4
```

---

## Functions

### Define and Call

Use `fn` to define a function. The body starts with `:` and ends with `;`:

```
fn greet(name):
    print("Hello " + name)
;

greet("Alice").                --> Hello Alice
```

### Return Values with `give`

`give` is the return keyword. It does **not** require parentheses:

```
fn add(a, b):
    give a + b
;

print(add(3, 4)).              --> 7
```

Parentheses are optional — both forms work:

```
give a + b.       --> OK
give(a + b).      --> also OK
```

### Functions Without `give` Return None

```
fn doStuff():
    var x = 5
;

--> doStuff() returns None
```

### Forward Declaration

Declare a function signature without a body — define it later:

```
fn myFunc(a, b).               --> forward declaration (no body)

--> ... later in the code ...

fn myFunc(a, b):
    give a + b
;
```

### Overloading by Arity

You can define multiple functions with the same name but different numbers of parameters:

```
fn add(a, b):
    give a + b
;

fn add(a, b, c):
    give a + b + c
;

print(add(1, 2)).              --> 3
print(add(1, 2, 3)).           --> 6
```

### Pass-by-Reference with `@`

Prefix a parameter with `@` to pass it by reference — changes inside the function modify the caller's variable:

```
fn increment(@x):
    x = x + 1
;

var n = 10.
increment(n).
print(n).                      --> 11
```

Without `@`, the original variable is unchanged:

```
fn tryChange(x):
    x = x + 1
;

var n = 10.
tryChange(n).
print(n).                      --> 10 (unchanged)
```

### Recursion

```
fn factorial(n):
    if n <= 1:
        give 1
    ;
    give n * factorial(n - 1)
;

print(factorial(5)).           --> 120
```

### Nested Functions

```
fn outer(x):
    fn inner(y):
        give x + y
    ;
    give inner(10)
;

print(outer(5)).               --> 15
```

---

## Control Flow

### If / Elif / Else

```
var score = 85.

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

Output: `B`

The dot is optional before `elif`, `else`, and `;`.

### For Loops

**Simple range (0 to n):**

```
for i in range(5):
    print(i)
;
--> prints 0, 1, 2, 3, 4, 5
```

**Named range (from ... to):**

```
for i in range(from 1 to 10):
    print(i)
;
--> prints 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
```

**Range with step:**

```
for i in range(from 0 to 20 step 2):
    print(i)
;
--> prints 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20
```

**Reverse range:**

```
for i in range(from 5 to 1):
    print(i)
;
--> prints 5, 4, 3, 2, 1
```

**Iterate over a list:**

```
var names = ["Alice", "Bob", "Charlie"].

for name in names:
    print("Hello " + name)
;
--> Hello Alice
--> Hello Bob
--> Hello Charlie
```

**Iterate over a string:**

```
for ch in "abc":
    print(ch)
;
--> a
--> b
--> c
```

### While Loops

```
var i = 0.
var sum = 0.

while i < 10:
    sum += i
    i++
;

print(sum).                    --> 45
```

### Pass (empty blocks)

Use `pass` for a block that does nothing:

```
if 1 == 0:
    pass
;
```

---

## The `of` Keyword

The `of` keyword lets you call methods in a **"verb first"** style. Instead of `object.method()`, write `method() of object`:

```
var name = "hello".

--> These are equivalent:
print(name.upper()).
print(upper() of name).
```

Works with arguments too:

```
var text = "hello world".
print(replace("world", "ScriptIt") of text).
--> hello ScriptIt
```

Works with builtins:

```
print(type() of 42).          --> int
print(type() of True).        --> bool
print(len() of [1, 2, 3]).    --> 3
```

---

## Math Functions

ScriptIt includes a full suite of math functions:

### Basic Math

| Function    | Description      | Example            |
| ----------- | ---------------- | ------------------ |
| `abs(x)`    | Absolute value   | `abs(-5)` → `5`    |
| `sqrt(x)`   | Square root      | `sqrt(16)` → `4`   |
| `min(a, b)` | Minimum          | `min(3, 7)` → `3`  |
| `max(a, b)` | Maximum          | `max(3, 7)` → `7`  |
| `ceil(x)`   | Round up         | `ceil(3.2)` → `4`  |
| `floor(x)`  | Round down       | `floor(3.8)` → `3` |
| `round(x)`  | Round to nearest | `round(3.5)` → `4` |

### Trigonometry (radians)

| Function | Inverse   |
| -------- | --------- |
| `sin(x)` | `asin(x)` |
| `cos(x)` | `acos(x)` |
| `tan(x)` | `atan(x)` |
| `cot(x)` | `acot(x)` |
| `sec(x)` | `asec(x)` |
| `csc(x)` | `acsc(x)` |

### Logarithms

| Function   | Description       | Example            |
| ---------- | ----------------- | ------------------ |
| `log(x)`   | Natural logarithm | `log(e)` → `1`     |
| `log2(x)`  | Base-2 logarithm  | `log2(8)` → `3`    |
| `log10(x)` | Base-10 logarithm | `log10(100)` → `2` |

### Constants

```
PI.                            --> 3.14159265358979...
e.                             --> 2.71828182845904...
```

### Operator Precedence

Standard math precedence applies: `^` > `* / %` > `+ -`

```
2 + 3 * 4.                    --> 14
(2 + 3) * 4.                  --> 20
2 ^ 3 ^ 2.                    --> 512
```

---

## Builtin Functions

### I/O Functions

| Function           | Description                 | Example                   |
| ------------------ | --------------------------- | ------------------------- |
| `print(a, b, ...)` | Print values with newline   | `print("hi", 42)`         |
| `pprint(x)`        | Pretty-print with type info | `pprint([1,2])`           |
| `input()`          | Read a line from stdin      | `var s = input()`         |
| `input("? ")`      | Read with a prompt          | `var s = input("Name: ")` |
| `read(f)`          | Read file contents          | `var text = read(f)`      |
| `write(f, text)`   | Write text to file          | `write(f, "Hello\n")`     |
| `readLine(f)`      | Read file as list of lines  | `var lines = readLine(f)` |

### Type Functions

| Function           | Description               | Example                          |
| ------------------ | ------------------------- | -------------------------------- |
| `type(x)`          | Type name as string       | `type(42)` → `"int"`             |
| `len(x)`           | Length of string/list/set | `len("hi")` → `2`                |
| `str(x)`           | Convert to string         | `str(42)` → `"42"`               |
| `int(x)`           | Convert to integer        | `int("42")` → `42`               |
| `float(x)`         | Convert to float          | `float(42)` → `42.0`             |
| `double(x)`        | Convert to double         | `double(42)` → `42.0`            |
| `bool(x)`          | Convert to boolean        | `bool(0)` → `False`              |
| `repr(x)`          | Representation string     | `repr("hi")` → `'"hi"'`          |
| `isinstance(x, t)` | Check type by name        | `isinstance(42, "int")` → `True` |

### Extended Numeric Conversions

| Function          | Description                   |
| ----------------- | ----------------------------- |
| `long(x)`         | Convert to long               |
| `long_long(x)`    | Convert to long long          |
| `long_double(x)`  | Convert to long double        |
| `uint(x)`         | Convert to unsigned int       |
| `ulong(x)`        | Convert to unsigned long      |
| `ulong_long(x)`   | Convert to unsigned long long |
| `auto_numeric(x)` | Auto-detect best numeric type |

### Container Functions

| Function         | Description                     |
| ---------------- | ------------------------------- |
| `list()`         | Create an empty list            |
| `set()`          | Create an empty set             |
| `dict()`         | Create an empty dictionary      |
| `append(lst, x)` | Append to list, return new list |
| `pop(lst)`       | Remove and return last item     |
| `range_list(n)`  | Create list `[0, 1, ..., n-1]`  |

### Functional Functions

| Function            | Description                     |
| ------------------- | ------------------------------- |
| `sum(list)`         | Sum of all elements             |
| `sorted(list)`      | Return a sorted copy            |
| `reversed(list)`    | Return a reversed copy          |
| `all(list)`         | True if all elements are truthy |
| `any(list)`         | True if any element is truthy   |
| `enumerate(list)`   | List of `[index, value]` pairs  |
| `zip(list1, list2)` | Pair up elements from two lists |
| `map(fn, list)`     | Apply function to each element  |

### File Functions

| Function           | Description         |
| ------------------ | ------------------- |
| `open(path, mode)` | Open a file handle  |
| `close(f)`         | Close a file handle |

---

## Type Conversion

Convert between types using builtin functions:

```
int("42").                     --> 42
int(3.7).                      --> 3
float(42).                     --> 42.0
str(42).                       --> "42"
bool(0).                       --> False
bool(1).                       --> True
bool("").                      --> False
bool("hello").                 --> True
```

Extended numeric conversions:

```
long(42).
long_long(42).
long_double(3.14).
uint(42).
ulong(42).
ulong_long(42).
```

---

## Type Methods (on any value)

Every value in ScriptIt supports these dot-methods:

### Type Introspection

```
var x = 42.
x.type().                      --> "int"
x.str().                       --> "42"
x.len().                       --> (length, for strings/lists/sets)
x.hash().                      --> (hash value)
```

### Type Checks

| Method               | Checks for         |
| -------------------- | ------------------ |
| `.is_none()`         | None               |
| `.is_int()`          | Integer            |
| `.is_uint()`         | Unsigned integer   |
| `.is_long()`         | Long               |
| `.is_ulong()`        | Unsigned long      |
| `.is_long_long()`    | Long long          |
| `.is_ulong_long()`   | Unsigned long long |
| `.is_float()`        | Float              |
| `.is_double()`       | Double             |
| `.is_long_double()`  | Long double        |
| `.is_bool()`         | Boolean            |
| `.is_string()`       | String             |
| `.is_list()`         | List               |
| `.is_set()`          | Set                |
| `.is_dict()`         | Dictionary         |
| `.is_any_integral()` | Any integer type   |
| `.is_any_floating()` | Any floating type  |
| `.is_any_numeric()`  | Any numeric type   |

### Type Conversion Methods

| Method            | Converts to |
| ----------------- | ----------- |
| `.toInt()`        | Integer     |
| `.toDouble()`     | Double      |
| `.toFloat()`      | Float       |
| `.toLong()`       | Long        |
| `.toLongLong()`   | Long long   |
| `.toLongDouble()` | Long double |
| `.toBool()`       | Boolean     |
| `.toString()`     | String      |

```
var s = "42".
print(s.toInt() + 8).         --> 50

var n = 3.14.
print(n.toInt()).              --> 3
```

---

## File I/O

### Using the Context Manager

The `let ... be ... :` syntax opens a file and auto-closes it when the block ends:

```
--> Write to a file
let f be open("test.txt", "w"):
    write(f, "Hello World\n")
    write(f, "Second line\n")
;

--> Read from a file
let f be open("test.txt", "r"):
    var content = read(f)
    print(content)
;
```

Output:

```
Hello World
Second line
```

### Quick Read/Write

For simpler cases, use the global `read` and `write` with file paths:

```
write("/tmp/data.txt", "Hello!").
var content = read("/tmp/data.txt").
print(content).                --> Hello!
```

### Read Lines

```
var lines = readLine("/tmp/data.txt").
print(len(lines)).             --> 1
```

---

## Overflow Promotion

ScriptIt automatically promotes numeric types when an operation would overflow. You never get silent integer overflow:

**Promotion chain:** `int → long → long_long → float → double → long_double`

```
var big = 1000000 * 1000000.
print(big).                    --> 1000000000000
print(type() of big).          --> long_long

var huge = 9223372036854775807 + 1.
print(type() of huge).         --> (promoted automatically)
```

This means math in ScriptIt is **safe by default** — no surprise wrap-around.

---

## Notebook Mode

ScriptIt has a browser-based notebook interface (like Jupyter) for interactive development:

```bash
python3 notebook_server.py
```

Then open **http://localhost:8888** in your browser. Notebooks use the `.nsit` file extension.

You can also use the launcher script:

```bash
./notebook.sh                          # start with a new notebook
./notebook.sh my_notebook.nsit         # open an existing notebook
./notebook.sh my.nsit --port 9999      # specify a port
```

---

## Scope Rules

| Context         | Can read outer variables? | Can modify outer variables? |
| --------------- | ------------------------- | --------------------------- |
| Functions       | ✅ Yes                    | ❌ No (unless `@` ref)      |
| If/Elif/Else    | ✅ Yes                    | ✅ Yes                      |
| For/While loops | ✅ Yes                    | ✅ Yes                      |

---

## REPL Commands

| Command | Description                                            |
| ------- | ------------------------------------------------------ |
| `exit`  | Quit the REPL                                          |
| `clear` | Clear the terminal screen                              |
| `wipe`  | Clear the screen **and** reset all state (fresh start) |

---

## Error Messages

ScriptIt provides clear error messages:

| Situation                          | Error                                                        |
| ---------------------------------- | ------------------------------------------------------------ |
| `1 / 0.`                           | `Error: Division by zero`                                    |
| `noVar.`                           | `Error: Undefined variable noVar`                            |
| `f()` with wrong arg count         | `Error: Arity mismatch`                                      |
| Missing dot mid-script             | `Error: Expected '.' ...`                                    |
| `"hello" - 1.`                     | `Error: ...` (invalid operation)                             |
| Forward-declared but never defined | `Error: Function 'x' was forward-declared but never defined` |
| Duplicate parameter names          | `Error: Duplicate parameter name 'x' in function 'f'`        |
| Empty function body                | `Error: Empty function body not allowed, use 'pass'`         |

---

## Complete Example

Here's a full ScriptIt program that puts it all together:

```
--> students.sit — Grade calculator <--

var names = ["Alice", "Bob", "Charlie", "Diana"].
var scores = [95, 87, 92, 78].

fn average(nums):
    var total = 0
    for n in nums:
        total += n
    ;
    give total / len(nums)
;

fn grade(score):
    if score >= 90:
        give "A"
    elif score >= 80:
        give "B"
    elif score >= 70:
        give "C"
    else:
        give "F"
    ;
;

print("=== Student Report ===")
print("")

for i in range(from 0 to 3):
    var name = names[i]
    var score = scores[i]
    print(name + ": " + str(score) + " (" + grade(score) + ")")
;

print("")
print("Class average: " + str(average(scores)))

if average(scores) >= 90:
    print("Outstanding class performance!")
elif average(scores) >= 80:
    print("Good work, class!")
else:
    print("Keep studying!")
;
```

Output:

```
=== Student Report ===

Alice: 95 (A)
Bob: 87 (B)
Charlie: 92 (A)
Diana: 78 (C)

Class average: 88
Good work, class!
```

---

## More Examples

### FizzBuzz

```
for i in range(from 1 to 100):
    if i % 15 == 0:
        print("FizzBuzz")
    elif i % 3 == 0:
        print("Fizz")
    elif i % 5 == 0:
        print("Buzz")
    else:
        print(i)
    ;
;
```

### String Processing

```
var sentence = "  the quick BROWN fox  ".

--> Clean up and transform
var cleaned = sentence.strip().lower().title()
print(cleaned).                --> The Quick Brown Fox

--> Split into words
var words = cleaned.split(" ")
print(words).                  --> [The, Quick, Brown, Fox]
print(len(words)).             --> 4

--> Check contents
print(cleaned.contains("Quick")).   --> 1
print(cleaned.startswith("The")).   --> 1

--> Using 'of' syntax
print(upper() of cleaned).    --> THE QUICK BROWN FOX
print(reverse() of cleaned).  --> xoF nworB kciuQ ehT
```

### Fibonacci with Memoization

```
fn fib(n):
    if n <= 1:
        give n
    ;
    give fib(n - 1) + fib(n - 2)
;

for i in range(from 0 to 10):
    print("fib(" + str(i) + ") = " + str(fib(i)))
;
```

### Pass-by-Reference Swap

```
fn swap(@a, @b):
    var temp = a.
    a = b.
    b = temp.
;

var x = 10.
var y = 20.
swap(x, y).
print("x=" + str(x) + " y=" + str(y)).   --> x=20 y=10
```

---

## Tips & Tricks

1. **REPL auto-print:** In the REPL, just type an expression — no `print()` needed:

   ```
   >> 2 ^ 10
   1024
   >> "hello".upper()
   HELLO
   ```

2. **Implicit multiplication** makes math feel natural:

   ```
   var r = 5.
   var area = PI r^2.
   ```

3. **The dot is forgiving** — you can often omit it at end of lines or before block terminators.

4. **Use `wipe`** in the REPL when you want a completely fresh environment.

5. **Pass-by-reference** with `@` is powerful for functions that modify arguments in-place.

6. **Chain methods** for expressive one-liners:
   ```
   "  Hello World  ".strip().lower().split(" ").
   --> [hello, world]
   ```

---

_ScriptIt v2 — Write scripts, not boilerplate._ ✨
