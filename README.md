# Pythonic C++ Library

Hey there! So you love Python's clean syntax but need C++'s performance? You're in the right place.

This library brings Python's most beloved features to C++ - dynamic typing, easy containers, slicing, string methods, comprehensions, and tons more. No weird hacks, just modern C++17 that feels surprisingly Pythonic.

## Table of Contents

- [Quick Start](#quick-start)
  - [The Easy Way (Recommended)](#the-easy-way-recommended)
  - [The Manual Way](#the-manual-way)
- [The `var` Type](#the-var-type)
  - [Type Checking & Conversion](#type-checking--conversion)
- [Containers](#containers)
  - [Lists](#lists)
  - [Slicing](#slicing-finally)
  - [Dicts](#dicts)
  - [Sets](#sets)
  - [Truthiness (Python-style Boolean Context)](#truthiness-python-style-boolean-context)
  - [None Values](#none-values)
- [String Methods](#string-methods)
- [Comprehensive Math Library](#comprehensive-math-library)
  - [Basic Math Operations](#basic-math-operations)
  - [Trigonometric Functions](#trigonometric-functions)
  - [Hyperbolic Functions](#hyperbolic-functions)
  - [Angle Conversion](#angle-conversion)
  - [Mathematical Constants](#mathematical-constants)
  - [Random Functions](#random-functions)
  - [Product](#product-multiply-all-elements)
  - [Advanced Math Functions](#advanced-math-functions)
  - [Aggregation Functions](#aggregation-functions-from-pythonicvars)
- [List Comprehensions](#list-comprehensions)
- [Lambda Helpers](#lambda-helpers)
- [Iteration](#iteration)
  - [Range](#range)
  - [Enumerate](#enumerate)
  - [Zip](#zip)
  - [Reversed](#reversed)
  - [Loop Macros](#loop-macros)
- [Functional Programming](#functional-programming)
  - [Map, Filter, Reduce](#map-filter-reduce)
  - [Sorting](#sorting)
  - [Advanced Functional Utilities](#advanced-functional-utilities)
  - [All/Any Predicates](#allany-predicates)
- [Global Variable Table (let macro)](#global-variable-table-let-macro)
- [Printing & Formatting](#printing--formatting)
- [User Input](#user-input)
- [File I/O](#file-io)
- [Operator Overloading](#operator-overloading)
- [What's Under the Hood?](#whats-under-the-hood)
- [Examples](#examples)
- [Tips & Tricks](#tips--tricks)
- [Common Pitfalls](#common-pitfalls)

## Quick Start

### The Easy Way (Recommended)

Just include the main header - it pulls in everything:

```cpp
#include "pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;
using namespace pythonic::file;
```

### The Manual Way

Or pick exactly what you need:

```cpp
#include "pythonicVars.hpp"      // var, list, dict, set
#include "pythonicPrint.hpp"     // print(), pprint()
#include "pythonicLoop.hpp"      // range(), enumerate(), zip() + macros
#include "pythonicFunction.hpp"  // map, filter, comprehensions
#include "pythonicFile.hpp"      // File I/O
#include "pythonicMath.hpp"      // Math functions (trig, logarithms, etc.)

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;
using namespace pythonic::file;
using namespace pythonic::math;
```

**About Namespaces:**

- `pythonic::vars` - Core `var` type, containers, type conversion
- `pythonic::print` - Printing and formatting
- `pythonic::loop` - Iteration helpers (range, enumerate, zip, reversed)
- `pythonic::func` - Functional programming (map, filter, comprehensions)
- `pythonic::file` - File I/O
- `pythonic::math` - Comprehensive math library (trig, logarithms, random, etc.)

You can use them all with `using namespace` or be selective and qualify names like `pythonic::vars::sorted()`.

**Compile with C++17:**

```bash
g++ -std=c++17 your_code.cpp -o your_app
```

## The `var` Type

This is the heart of the library - a dynamic variable that can hold anything.

```cpp
var x = 42;           // int
var y = 3.14;         // double
var name = "Alice";   // string
var flag = true;      // bool

// Changes type whenever you want
x = "now I'm a string";
x = list(1, 2, 3);    // now I'm a list!
```

### Type Checking & Conversion

```cpp
var x = 42;
print(x.type());              // prints: int
print(isinstance(x, "int"));  // prints: True

// Template version
if (isinstance<int>(x)) {
    int n = x.get<int>();
}

// Type conversion helpers
var a = Int("123");      // "123" -> 123
var b = Float("3.14");   // "3.14" -> 3.14
var c = Str(42);         // 42 -> "42"
var d = Bool(0);         // 0 -> false
```

## Containers

### Lists

```cpp
var nums = list(1, 2, 3, 4, 5);
var mixed = list("hello", 42, 3.14, true);  // yep, mixed types
var empty = list();

// Access by index
print(nums[0]);         // 1
nums[0] = 100;          // modify

// Methods you'd expect
nums.append(6);
nums.extend(list(7, 8, 9));
nums.insert(0, -1);
nums.pop();
nums.remove(100);
nums.clear();
print(nums.len());      // or len(nums)
```

### Slicing (finally!)

Works just like Python. Negative indices, steps, the whole deal.

```cpp
var lst = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

var a = lst.slice(2, 5);      // [2, 3, 4]
var b = lst.slice(0, 10, 2);  // [0, 2, 4, 6, 8]  (every 2nd)
var c = lst.slice(-3);        // [7, 8, 9]  (last 3)
var d = lst(1, 4);            // [1, 2, 3]  (operator() works too)

// Reverse with negative step
var rev = lst.slice(pythonic::vars::None, pythonic::vars::None, -1);  // reverse!

// Strings too!
var s = "Hello, World!";
print(s.slice(0, 5));         // Hello
```

### Dicts

```cpp
var person = dict();
person["name"] = "Bob";
person["age"] = 25;
person["city"] = "NYC";

// Check if key exists
if (person.has("name")) {
    print(person["name"]);
}

// Iterate
for_in(key, person.keys()) {
    print(key, ":", person[key]);
}

// Get keys, values, items
var keys = person.keys();      // list of keys
var values = person.values();  // list of values
var items = person.items();    // list of [key, value] pairs
```

### Sets

```cpp
var s = set(1, 2, 3, 4, 5);
s.add(6);
s.remove(1);

// Check if element exists (like "in" in Python)
if (s.has(3)) {
    print("3 is in the set");
}

// Set operations
var a = set(1, 2, 3);
var b = set(3, 4, 5);

var union_set = a | b;          // {1, 2, 3, 4, 5}
var intersect = a & b;          // {3}
var diff = a - b;               // {1, 2}
```

### Truthiness (Python-style Boolean Context)

Just like Python, containers and values have "truthiness":

```cpp
var empty_list = list();
var full_list = list(1, 2, 3);
var zero = 0;
var number = 42;
var empty_str = "";
var text = "hello";

// Works exactly like Python's if statement
if (full_list) {
    print("List has items!");      // This prints
}

if (!empty_list) {
    print("List is empty!");       // This prints
}

if (number) {
    print("Number is non-zero!");  // This prints
}

if (!zero) {
    print("Zero is falsy!");       // This prints
}

if (text) {
    print("String is not empty!"); // This prints
}

// Truthiness rules (just like Python):
// - Empty containers (list, dict, set) are False
// - Zero (0, 0.0) is False
// - Empty string "" is False
// - None is False
// - Everything else is True
```

### None Values

Check for None (undefined/null values):

```cpp
var x;  // Default is None

// Check if None
if (x.type() == "None") {
    print("x is None");
}

// Or use is_none()
if (x.is_none()) {
    print("x is None");
}

// Functions can return None
var result = some_dict.get("nonexistent_key");  // Returns None if key doesn't exist
if (result.is_none()) {
    print("Key not found");
}

// None in truthiness context
if (!x) {
    print("x is None or empty or zero");
}
```

## String Methods

All the string goodness you know and love:

```cpp
var s = "  Hello, World!  ";

// Transformations
print(s.upper());              // "  HELLO, WORLD!  "
print(s.lower());              // "  hello, world!  "
print(s.strip());              // "Hello, World!"
print(s.lstrip());             // "Hello, World!  "
print(s.rstrip());             // "  Hello, World!"
print(s.replace("World", "Python"));  // "  Hello, Python!  "
print(s.capitalize());         // "  hello, world!  "
print(s.title());              // "  Hello, World!  "

// Splitting and joining
var words = s.split(",");      // ["  Hello", " World!  "]
var joined = var("-").join(list("a", "b", "c"));  // "a-b-c"

// Searching
print(s.find("World"));        // 9
print(s.count("l"));           // 3
print(s.startswith("  Hello")); // True
print(s.endswith("!  "));      // True

// Checking character types
var digit_str = "12345";
print(digit_str.isdigit());    // True
print(digit_str.isalpha());    // False
print(digit_str.isalnum());    // True

var alpha_str = "Hello";
print(alpha_str.isalpha());    // True

var mixed = "Hello123";
print(mixed.isalnum());        // True

var space_str = "   ";
print(space_str.isspace());    // True

// Padding
var text = "Hi";
print(text.center(10));        // "    Hi    "
print(text.center(10, "*"));   // "****Hi****"
print(text.zfill(5));          // "000Hi"

// Other useful methods
print(s.reverse());            // "  !dlroW ,olleH  "
```

## Comprehensive Math Library

The pythonic math module provides extensive mathematical functions - from basic operations to advanced trigonometry!

```cpp
using namespace pythonic::math;  // Don't forget this!
```

### Basic Math Operations

```cpp
// Rounding
print(round(var(3.7)));        // 4
print(floor(var(3.7)));        // 3
print(ceil(var(3.2)));         // 4
print(trunc(var(-3.7)));       // -3

// Powers and roots
print(pow(var(2), var(10)));   // 1024
print(sqrt(var(16)));          // 4
print(nthroot(var(27), var(3))); // 3 (cube root)

// Exponential and logarithms
print(exp(var(1)));            // 2.718... (e^1)
print(log(var(2.718)));        // ~1 (natural log)
print(log10(var(100)));        // 2
print(log2(var(8)));           // 3

// Absolute value
var x = abs(var(-5));          // 5 (also in pythonic::vars)
```

### Trigonometric Functions

All angles in radians:

```cpp
var pi_val = pi();             // Get π constant

// Basic trig
print(sin(pi_val / 2));        // 1
print(cos(var(0)));            // 1
print(tan(pi_val / 4));        // 1

// Reciprocal trig functions
print(cot(pi_val / 4));        // 1 (cotangent)
print(sec(var(0)));            // 1 (secant)
print(cosec(pi_val / 2));      // 1 (cosecant, also: csc())

// Inverse trig
print(asin(var(1)));           // π/2
print(acos(var(1)));           // 0
print(atan(var(1)));           // π/4
print(atan2(var(1), var(1)));  // π/4 (two-argument)

// And their reciprocals
print(acot(var(1)));           // Inverse cotangent
print(asec(var(1)));           // Inverse secant
print(acosec(var(1)));         // Inverse cosecant (also: acsc())
```

### Hyperbolic Functions

```cpp
print(sinh(var(0)));           // 0
print(cosh(var(0)));           // 1
print(tanh(var(0)));           // 0

// Inverse hyperbolic
print(asinh(var(0)));          // 0
print(acosh(var(1)));          // 0
print(atanh(var(0)));          // 0
```

### Angle Conversion

```cpp
var deg = degrees(pi());       // 180 (radians to degrees)
var rad = radians(var(180));   // π (degrees to radians)
```

### Mathematical Constants

```cpp
var pi_val = pi();             // 3.14159...
var e_val = e();               // 2.71828...
```

### Random Functions

```cpp
// Random integer in range [min, max]
var dice = random_int(var(1), var(6));

// Random float in range [min, max)
var rand_percent = random_float(var(0.0), var(100.0));

// Random element from list or set
var items = list("apple", "banana", "cherry");
var choice = random_choice(items);

var nums_set = set(10, 20, 30, 40);
var num_choice = random_choice_set(nums_set);

// Fill list with N random integers (like NumPy/PyTorch)
var rand_ints = fill_random(100, var(1), var(100));  // 100 random ints in [1, 100]

// Fill list with N random floats (uniform distribution)
var rand_floats = fill_randomf(100, var(0.0), var(1.0));  // 100 random floats in [0, 1)

// Fill list with N random floats (Gaussian/normal distribution)
var gauss_data = fill_randomn(100, var(0.0), var(1.0));  // mean=0, stddev=1

// Fill set with N unique random integers
var rand_set = fill_random_set(10, var(1), var(50));  // 10 unique ints in [1, 50]

// Fill set with N random floats (uniform distribution)
var rand_set_f = fill_randomf_set(10, var(0.0), var(1.0));

// Fill set with N random floats (Gaussian distribution)
var gauss_set = fill_randomn_set(10, var(0.0), var(1.0));  // mean=0, stddev=1
```

### Product (Multiply All Elements)

```cpp
var factors = list(2, 3, 4);
print(product(factors));        // 24

// With starting value
print(product(factors, var(10))); // 240

// Works with sets too
var s = set(2, 3, 4);
print(product(s));              // 24
```

### Advanced Math Functions

```cpp
// Greatest common divisor
print(gcd(var(48), var(18)));  // 6

// Least common multiple
print(lcm(var(12), var(18)));  // 36

// Factorial
print(factorial(var(5)));      // 120

// Hypotenuse (Pythagorean theorem)
print(hypot(var(3), var(4)));  // 5

// Other useful functions
print(fmod(var(5.5), var(2.0))); // 1.5 (floating-point remainder)
print(copysign(var(5), var(-1))); // -5 (copy sign)
print(fabs(var(-3.14)));       // 3.14 (floating-point absolute)
```

### Aggregation Functions (from pythonic::vars)

```cpp
using namespace pythonic::vars;  // These are in vars namespace

var nums = list(3, 1, 4, 1, 5);
print(min(nums));              // 1
print(max(nums));              // 5
print(sum(nums));              // 14

// **Warning**: min(), max(), and sum() on empty lists will throw runtime_error
// Always check if list is not empty:
var data = list();
if (data) {  // Truthiness check
    print(max(data));
}
```

## List Comprehensions

Yes, we have those too! Three flavors:

### List Comprehensions

```cpp
using namespace pythonic::func;

// Basic: [x*2 for x in range(5)]
auto doubled = list_comp(
    range(5),
    lambda_(x, x * 2)
);
// Result: [0, 2, 4, 6, 8]

// With filter: [x*2 for x in range(10) if x % 2 == 0]
auto even_doubled = list_comp(
    range(10),
    lambda_(x, x * 2),
    lambda_(x, x % 2 == 0)
);
// Result: [0, 4, 8, 12, 16]
```

### Set Comprehensions

```cpp
// {x*x for x in range(5)}
auto squares = set_comp(
    range(5),
    lambda_(x, x * x)
);
// Result: {0, 1, 4, 9, 16}
```

### Dict Comprehensions

```cpp
// {x: x*x for x in range(5)}
auto square_dict = dict_comp(
    range(5),
    lambda_(x, x),        // key function
    lambda_(x, x * x)     // value function
);
// Result: {0:0, 1:1, 2:4, 3:9, 4:16}
```

## Lambda Helpers

Writing lambdas in C++ can be verbose. We've got macros to help:

```cpp
// One parameter
auto double_it = lambda_(x, x * 2);

// Two parameters
auto add = lambda2_(x, y, x + y);

// Three parameters
auto sum3 = lambda3_(x, y, z, x + y + z);

// Capturing lambda (captures by value)
int factor = 10;
auto scale = clambda_(x, x * factor);
```

## Iteration

### Range

```cpp
using namespace pythonic::loop;

// range(n) - 0 to n-1
for_in(i, range(5)) {
    print(i);  // 0, 1, 2, 3, 4
}

// range(start, stop)
for_in(i, range(2, 5)) {
    print(i);  // 2, 3, 4
}

// range(start, stop, step)
for_in(i, range(0, 10, 2)) {
    print(i);  // 0, 2, 4, 6, 8
}

// Negative step (reverse iteration) - very useful!
for_in(i, range(10, 0, -1)) {
    print(i);  // 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
}

// **Note**: step cannot be 0 - will throw invalid_argument
```

### Enumerate

```cpp
var fruits = list("apple", "banana", "cherry");

for_enumerate(i, fruit, fruits) {
    print(i, ":", fruit);
    // 0 : apple
    // 1 : banana
    // 2 : cherry
}
```

### Zip

```cpp
var names = list("Alice", "Bob", "Charlie");
var ages = list(25, 30, 35);

for (auto pair : zip(names, ages)) {
    print(pair[0], "is", pair[1], "years old");
}
```

### Reversed

Iterate in reverse order - super common in Python!

```cpp
var nums = list(1, 2, 3, 4, 5);
var rev = reversed_var(nums);  // [5, 4, 3, 2, 1]

for_in(x, reversed_var(nums)) {
    print(x);  // 5, 4, 3, 2, 1
}

// Works with range() too
for_in(i, reversed_var(range(10))) {
    print(i);  // 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
}
```

### Loop Macros

Tired of typing `for (auto x : ...)`? Use these:

```cpp
var nums = list(1, 2, 3, 4, 5);

// for_in - simple iteration
for_in(x, nums) {
    print(x);
}

// for_index - index-based loop
for_index(i, nums) {
    print(i, nums[i]);
}

// for_enumerate - both index and value
for_enumerate(i, x, nums) {
    print(i, ":", x);
}

// for_range - like Python's range()
for_range(i, 0, 10, 2) {
    print(i);  // 0, 2, 4, 6, 8
}

// while_true - infinite loop
int count = 0;
while_true {
    print(count++);
    if (count > 5) break;
}
```

## Functional Programming

### Map, Filter, Reduce

```cpp
using namespace pythonic::func;

var nums = list(1, 2, 3, 4, 5);

// Map
var doubled = map(nums, lambda_(x, x * 2));
// [2, 4, 6, 8, 10]

// Map with index (hybrid of map and enumerate)
// Perfect for position-dependent transformations!
auto with_idx = map_indexed(nums, lambda2_(idx, x,
    dict().set("index", idx).set("value", x)
));
// [{index:0, value:1}, {index:1, value:2}, ...]

// Practical example: Apply weights based on position
var values = list(10, 20, 30, 40);
var weights = list(1.0, 1.5, 2.0, 2.5);
auto weighted = map_indexed(values, lambda2_(i, x,
    x * weights[i]
));
// [10, 30, 60, 100]

// Filter
var evens = filter(nums, lambda_(x, x % 2 == 0));
// [2, 4]

// Reduce (sum example)
var total = reduce(nums, 0, lambda2_(acc, x, acc + x));
// 15
```

### Sorting

```cpp
var nums = list(3, 1, 4, 1, 5, 9);

// Basic sort
var sorted_nums = sorted(nums);
// [1, 1, 3, 4, 5, 9]

// Reverse sort
var desc = sorted(nums, true);
// [9, 5, 4, 3, 1, 1]

// Sort with custom key
var words = list("apple", "pie", "zoo", "a");
auto by_length = sorted(words, lambda_(x, x.len()));
// ["a", "pie", "zoo", "apple"]
```

### Advanced Functional Utilities

```cpp
using namespace pythonic::func;

var nums = list(1, 2, 3, 4, 5, 6, 7, 8, 9);

// Take first N elements
var first_three = take(nums, 3);  // [1, 2, 3]

// Drop first N elements
var skip_three = drop(nums, 3);   // [4, 5, 6, 7, 8, 9]

// Take while condition is true
auto less_than_5 = take_while(nums, lambda_(x, x < 5));
// [1, 2, 3, 4]

// Drop while condition is true
auto from_5 = drop_while(nums, lambda_(x, x < 5));
// [5, 6, 7, 8, 9]

// Find first matching element
auto found = find_if(nums, lambda_(x, x > 5));
// 6

// Get index of element
var idx = index(nums, 5);  // 4

// Count matching elements
auto count_evens = count_if(nums, lambda_(x, x % 2 == 0));
// 4

// Flatten nested lists
var nested = list(list(1, 2), list(3, 4), list(5));
auto flat = flatten(nested);  // [1, 2, 3, 4, 5]

// Get unique elements
var dups = list(1, 2, 2, 3, 3, 3);
auto uniq = unique(dups);  // [1, 2, 3]

// Group by key
var items = list(1, 2, 3, 4, 5, 6);
auto grouped = group_by(items, lambda_(x, x % 2));
// {0: [2, 4, 6], 1: [1, 3, 5]}

// Cartesian product
auto prod = product(list(1, 2), list("a", "b"));
// [[1, "a"], [1, "b"], [2, "a"], [2, "b"]]

// Partial application
auto add = lambda2_(x, y, x + y);
auto add5 = partial(add, 5);
var result = add5(3);  // 8

// Function composition
auto double_it = lambda_(x, x * 2);
auto add_one = lambda_(x, x + 1);
auto double_then_add = compose(add_one, double_it);
var res = double_then_add(5);  // 11 (5*2 + 1)
```

### All/Any Predicates

```cpp
var all_true = list(true, true, true);
var some_true = list(false, true, false);
var none_true = list(false, false, false);

print(all_var(all_true));    // True
print(all_var(some_true));   // False
print(any_var(some_true));   // True
print(any_var(none_true));   // False
```

## Global Variable Table (let macro)

Need a Python-like global variable table? We've got you covered:

```cpp
using namespace pythonic::vars;

// Store variables by name
let(x) = 42;
let(name) = "Alice";
let(data) = list(1, 2, 3);

// Access them
print(let(x));        // 42
print(let(name));     // Alice

// Modify them
let(x) = let(x) + 10;
print(let(x));        // 52

// Check if exists
if (DynamicVar::has("x")) {
    print("x exists!");
}

// List all variables
auto all_vars = DynamicVar::list_all();
for_in(name, all_vars) {
    print(name, "=", DynamicVar::get_ref(name.template get<std::string>()));
}
```

This is handy for: - Scripting-like code

- Dynamic variable lookups
- Runtime configuration
- DSLs and interpreters

## Printing & Formatting

### Basic Print

Python-style print with space-separated arguments:

```cpp
using namespace pythonic::print;

// Works with var types
print("Hello", "World", 42);
// Output: Hello World 42

// Works with mixed native C++ types and var
var x = 100;
print("The result is:", x, "and the flag is:", true);
// Output: The result is: 100 and the flag is: true

// Automatic spacing between arguments
print(1, 2, 3, 4, 5);
// Output: 1 2 3 4 5
```

**String Formatting Behavior:**
Just like Python, strings behave differently based on context:

- Top-level strings: No quotes (e.g., `print("hello")` → `hello`)
- Strings in containers: Quoted (e.g., `print(list("hello"))` → `[hello]`)

This makes the output feel natural and Pythonic!

### Pretty Print

For complex nested structures with automatic indentation:

```cpp
var data = dict()
    .set("name", "Alice")
    .set("age", 30)
    .set("hobbies", list("reading", "coding", "gaming"))
    .set("scores", dict().set("math", 95).set("english", 88));

pprint(data);
// Output (nicely formatted with 2-space indentation):
// {
//   age: 30
//   hobbies: [reading, coding, gaming]
//   name: Alice
//   scores: {
//     english: 88
//     math: 95
//   }
// }

// Custom indentation (4 spaces instead of 2)
pprint(data, 4);
// {
//     age: 30
//     hobbies: [reading, coding, gaming]
//     ...
// }
```

**Smart Formatting:**

- **Simple/short lists**: Kept on one line for readability
- **Nested/complex structures**: Automatically indented with proper nesting
- **Recursive handling**: Deep nesting is automatically formatted correctly

Example of automatic nesting:

```cpp
var complex = dict()
    .set("users", list(
        dict().set("name", "Alice").set("scores", list(85, 90, 95)),
        dict().set("name", "Bob").set("scores", list(78, 82, 88))
    ))
    .set("meta", dict().set("version", 1).set("updated", "2024-01-01"));

pprint(complex);
// Beautifully formatted with proper indentation at each level!
// {
//   meta: {
//     updated: 2024-01-01
//     version: 1
//   }
//   users: [
//     {
//       name: Alice
//       scores: [85, 90, 95]
//     }
//     {
//       name: Bob
//       scores: [78, 82, 88]
//     }
//   ]
// }
```

## User Input

```cpp
using namespace pythonic::vars;

// Get string input
var name = input("Enter your name: ");
print("Hello,", name);

// Get and convert to int
var age_str = input("Enter your age: ");
var age = Int(age_str);

// Get and convert to float
var price_str = input("Enter price: ");
var price = Float(price_str);
```

## File I/O

### One-Line File Utilities

In standard C++, file I/O requires opening, writing, and closing. We make it simple:

```cpp
using namespace pythonic::file;

// Read entire file in one line
auto content = read_file("data.txt");
if (content) {  // Check if file was read (returns None if missing)
    print(content);
}

// Read lines into a list
auto lines = read_lines("data.txt");
if (lines) {  // Returns None if file doesn't exist
    for_in(line, lines) {
        print(line);
    }
}
```

**Important**: File read operations return `None` if the file doesn't exist or can't be opened. Always check:

```cpp
var data = read_file("config.txt");
if (!data) {
    print("File not found or couldn't be read!");
}
```

### Writing Files

```cpp
// Write string to file (one line!)
write_file("output.txt", "Hello, World!");

// Append to file
append_file("log.txt", "New log entry\n");

// Write list of lines
var lines = list("Line 1", "Line 2", "Line 3");
write_lines("output.txt", lines);
```

### Binary Mode Support

For raw binary data (e.g., neural network weights, images):

```cpp
using namespace pythonic::file;

// Binary write
File bin_file("weights.bin", FileMode::WRITE_BINARY);
bin_file.write("raw binary data");
bin_file.close();

// Binary read
File reader("weights.bin", FileMode::READ_BINARY);
auto binary_data = reader.read();
reader.close();

// Binary append
File appender("data.bin", FileMode::APPEND_BINARY);
appender.write("more binary data");
appender.close();
```

### Python-Style Context Manager (with_open)

The most Pythonic feature - automatic resource management:

```cpp
// Just like Python's "with open(...) as f:"
with_open("data.txt", "r", [](File &f) {
    auto content = f.read();
    print(content);
});  // File automatically closed!

// Write mode
with_open("output.txt", "w", [](File &f) {
    f.write("Hello, World!");
    f.write("Second line\n");
});  // File automatically closed!

// Append mode
with_open("log.txt", "a", [](File &f) {
    f.write("Log entry at ");
    f.write("2024-01-01\n");
});

// No need to remember to close() - it's automatic!
```

**Modes**: "r" (read), "w" (write), "a" (append), "rb" (read binary), "wb" (write binary), "ab" (append binary)

### File Existence Check

```cpp
// Check before reading (pragmatic approach)
if (file_exists("config.txt")) {
    auto config = read_file("config.txt");
    print(config);
} else {
    print("Config file not found, using defaults");
}

// Delete file
if (file_exists("temp.txt")) {
    file_remove("temp.txt");
}
```

## Operator Overloading

The `var` type supports all the operators you'd expect:

```cpp
var a = 10, b = 5;

// Arithmetic
print(a + b);    // 15
print(a - b);    // 5
print(a * b);    // 50
print(a / b);    // 2
print(a % b);    // 0

// Comparison
print(a == b);   // False
print(a != b);   // True
print(a > b);    // True
print(a < b);    // False
print(a >= b);   // True
print(a <= b);   // False

// Logical
var x = true, y = false;
print(x && y);   // False
print(x || y);   // True
print(!x);       // False

// String concatenation
var s1 = "Hello", s2 = "World";
print(s1 + " " + s2);  // "Hello World"

// List concatenation
var l1 = list(1, 2), l2 = list(3, 4);
print(l1 + l2);  // [1, 2, 3, 4]
```

## What's Under the Hood?

Just so you know what you're working with:

- **`var`** - A `std::variant` wrapper that can hold `int`, `double`, `std::string`, `bool`, `List`, `Dict`, `Set`, `None`
- **Containers** - Internally use `std::vector`, `std::map`, `std::set`
- **Performance** - Comparable to standard containers with minimal overhead
- **C++17 Required** - Uses `std::variant`, `std::visit`, `std::optional`, structured bindings
- **No exceptions** - Returns sensible defaults on errors (empty var, None, etc.)
- **Move semantics** - Optimized for modern C++ performance

## Examples

### Quick Script Example

```cpp
#include "pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;

int main() {
    // Get user input
    var name = input("What's your name? ");
    var age_str = input("How old are you? ");
    var age = Int(age_str);

    // Calculate
    var years_to_100 = 100 - age;

    // Print
    print("Hey", name + "!");
    print("You'll be 100 in", years_to_100, "years.");

    // Show some numbers
    var nums = range(1, age + 1);
    var squares = list_comp(nums, lambda_(x, x * x));

    print("Squares from 1 to", age + ":");
    pprint(squares);

    return 0;
}
```

### Data Processing Example

```cpp
#include "pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::func;

int main() {
    // Sample data
    var students = list(
        dict().set("name", "Alice").set("score", 85),
        dict().set("name", "Bob").set("score", 92),
        dict().set("name", "Charlie").set("score", 78),
        dict().set("name", "Diana").set("score", 95)
    );

    // Filter high scorers
    auto high_scorers = filter(students,
        lambda_(s, s["score"] > 80)
    );

    // Get names
    auto names = map(high_scorers,
        lambda_(s, s["name"])
    );

    print("Students with score > 80:");
    pprint(names);

    // Calculate average
    auto scores = map(students, lambda_(s, s["score"]));
    auto total = reduce(scores, 0, lambda2_(a, b, a + b));
    var avg = total / len(scores);

    print("Average score:", avg);

    return 0;
}
```

## Tips & Tricks

1. **Use `using namespace`** - Makes code cleaner, less typing
2. **`pprint()` for debugging** - Much easier to read than `print()` for complex structures
3. **Lambda macros** - `lambda_()`, `lambda2_()` save tons of typing
4. **Loop macros** - `for_in`, `for_enumerate` are your friends
5. **Type conversions** - Use `Int()`, `Float()`, `Str()`, `Bool()` for explicit conversions
6. **Slicing** - Works on both lists and strings, supports negative indices
7. **Comprehensions** - More concise than manual loops for transforming data
8. **The central header** - `#include "pythonic.hpp"` gives you everything

## Common Pitfalls

- **C++17 required** - Won't compile with older standards
- **No automatic conversions** - Use `Int()`, `Float()`, etc. when converting from strings
- **Dictionary keys** - Currently only support string keys
- **Index checking** - Out of bounds returns None instead of throwing
- **Comparison** - Different types compare as not equal (no implicit conversion)

## That's It!

You've got dynamic typing, easy containers, slicing, comprehensions, functional programming, math functions, file I/O, and more. Write Python-style code that compiles to fast C++.

Happy coding! 🚀
