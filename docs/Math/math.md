[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to graph](../Var/graph_helpers.md)

# Math Functions

This page documents all user-facing math functions in Pythonic, using a clear tabular format with concise examples.

---

## Overflow Policy

Many arithmetic and aggregation functions accept an optional `policy` parameter of type `Overflow`, which controls how overflows are handled:

| Policy                   | Enum Value | Description                                                                                  |
| ------------------------ | ---------- | -------------------------------------------------------------------------------------------- |
| `Overflow::Throw`        | 0          | **Default for functions and operators.** Throw an exception on overflow (safe, Python-like). |
| `Overflow::Promote`      | 1          | Promote to a larger type on overflow (never throws, but may use bigger type).                |
| `Overflow::Wrap`         | 2          | Wrap around on overflow (C++-like, may lose data, never throws).                             |
| `Overflow::None_of_them` | 3          | Raw C++ arithmetic (no checks, maximum performance).                                         |

**Policy Selection Guide:**

| Use Case                            | Recommended Policy       |
| ----------------------------------- | ------------------------ |
| Production code (safety first)      | `Overflow::Throw`        |
| Scientific computing (large values) | `Overflow::Promote`      |
| Performance-critical inner loops    | `Overflow::None_of_them` |
| Embedded/systems programming        | `Overflow::Wrap`         |

**Usage Example:**

```cpp
add(1, 2);                        // Uses Overflow::Throw by default
add(1, 2, Overflow::Promote);     // Promotes type on overflow
add(1, 2, Overflow::Wrap);        // Wraps on overflow
add(1, 2, Overflow::None_of_them);// Raw C++ arithmetic (fastest)

// Operator overloads use None_of_them by default for performance
var a = 1000000, b = 1000000;
var c = a * b;  // Uses Overflow::None_of_them (raw C++ multiplication)
```

---

## Type Promotion Sequence

When using `Overflow::Promote`, types are promoted in the following order:

| Rank | Type               | Description                       |
| ---- | ------------------ | --------------------------------- |
| 0    | bool               | Boolean (never promoted TO)       |
| 1    | unsigned int       | Smallest unsigned integer         |
| 2    | int                | Smallest signed integer           |
| 3    | unsigned long      | Medium unsigned integer           |
| 4    | long               | Medium signed integer             |
| 5    | unsigned long long | Largest unsigned integer          |
| 6    | long long          | Largest signed integer            |
| 7    | float              | Single-precision floating-point   |
| 8    | double             | Double-precision floating-point   |
| 9    | long double        | Extended-precision floating-point |

### Promotion Strategy

The promotion system uses different strategies based on input types:

| Input Types         | Promotion Strategy                                                 |
| ------------------- | ------------------------------------------------------------------ |
| Both unsigned       | Try unsigned containers (uint → ulong → ulong_long), then floating |
| At least one signed | Try signed containers (int → long → long_long), then floating      |
| Any floating-point  | Use floating containers only (float → double → long double)        |

### Smart Fit Behavior

The `smart_promote()` function finds the **smallest** container that can hold the result:

```cpp
// Both unsigned → result fits in smallest unsigned type
var a = 100u, b = 200u;
var result = add(a, b, Overflow::Promote);  // Returns unsigned int (300)

// Large result → promoted to larger type
var big = std::numeric_limits<int>::max();
var result = add(big, 1, Overflow::Promote);  // Returns long long

// Division always uses floating-point for precision
var x = 5, y = 2;
var result = div(x, y, Overflow::Promote);  // Returns float (2.5)
```

---

## Arithmetic Operations (Overflow-Aware)

| Function            | Description                                                        | Example                                       |
| ------------------- | ------------------------------------------------------------------ | --------------------------------------------- |
| `add(a, b, policy)` | Addition with overflow policy. Default is `Overflow::Throw`.       | `add(1, 2)`<br>`add(1, 2, Overflow::Promote)` |
| `sub(a, b, policy)` | Subtraction with overflow policy. Default is `Overflow::Throw`.    | `sub(5, 3)`<br>`sub(5, 3, Overflow::Wrap)`    |
| `mul(a, b, policy)` | Multiplication with overflow policy. Default is `Overflow::Throw`. | `mul(2, 3)`<br>`mul(2, 3, Overflow::Promote)` |
| `div(a, b, policy)` | Division with overflow policy. Default is `Overflow::Throw`.       | `div(6, 2)`<br>`div(6, 2, Overflow::Wrap)`    |
| `mod(a, b, policy)` | Modulo with overflow policy. Default is `Overflow::Throw`.         | `mod(7, 3)`<br>`mod(7, 3, Overflow::Promote)` |

- Use the main function for full control; helper variants like `add_throw`, `add_promote`, `add_wrap` are shortcuts.

---

## Power & Roots

| Function                 | Description                                               | Example                                       |
| ------------------------ | --------------------------------------------------------- | --------------------------------------------- |
| `pow(base, exp, policy)` | Power with overflow policy. Default is `Overflow::Throw`. | `pow(2, 8)`<br>`pow(2, 8, Overflow::Promote)` |
| `sqrt(v)`                | Square root                                               | `sqrt(9)`                                     |
| `nthroot(v, n)`          | N-th root                                                 | `nthroot(27, 3)`                              |

---

## Rounding & Floating Point

| Function         | Description                    | Example           |
| ---------------- | ------------------------------ | ----------------- |
| `round(v)`       | Round to nearest integer       | `round(3.6)`      |
| `floor(v)`       | Floor (round down)             | `floor(3.6)`      |
| `ceil(v)`        | Ceil (round up)                | `ceil(3.1)`       |
| `trunc(v)`       | Truncate towards zero          | `trunc(-3.7)`     |
| `fmod(x, y)`     | Floating-point remainder       | `fmod(7.5, 2.0)`  |
| `copysign(x, y)` | Copy sign of y to x            | `copysign(-3, 2)` |
| `fabs(v)`        | Absolute value                 | `fabs(-5)`        |
| `hypot(x, y)`    | Euclidean norm (sqrt(x² + y²)) | `hypot(3, 4)`     |

---

## Exponential & Logarithmic

| Function   | Description       | Example      |
| ---------- | ----------------- | ------------ |
| `exp(v)`   | Exponential (e^v) | `exp(1)`     |
| `log(v)`   | Natural logarithm | `log(2.718)` |
| `log10(v)` | Base-10 logarithm | `log10(100)` |
| `log2(v)`  | Base-2 logarithm  | `log2(8)`    |

---

## Trigonometric

| Function            | Description | Example       |
| ------------------- | ----------- | ------------- |
| `sin(v)`            | Sine        | `sin(pi())`   |
| `cos(v)`            | Cosine      | `cos(0)`      |
| `tan(v)`            | Tangent     | `tan(pi()/4)` |
| `cot(v)`            | Cotangent   | `cot(1)`      |
| `sec(v)`            | Secant      | `sec(0)`      |
| `cosec(v)`/`csc(v)` | Cosecant    | `cosec(1)`    |

---

## Inverse Trigonometric

| Function              | Description       | Example       |
| --------------------- | ----------------- | ------------- |
| `asin(v)`             | Arcsine           | `asin(1)`     |
| `acos(v)`             | Arccosine         | `acos(1)`     |
| `atan(v)`             | Arctangent        | `atan(1)`     |
| `atan2(y, x)`         | Arctangent of y/x | `atan2(1, 1)` |
| `acot(v)`             | Arccotangent      | `acot(1)`     |
| `asec(v)`             | Arcsecant         | `asec(2)`     |
| `acosec(v)`/`acsc(v)` | Arccosecant       | `acosec(2)`   |

---

## Hyperbolic

| Function   | Description                | Example      |
| ---------- | -------------------------- | ------------ |
| `sinh(v)`  | Hyperbolic sine            | `sinh(1)`    |
| `cosh(v)`  | Hyperbolic cosine          | `cosh(1)`    |
| `tanh(v)`  | Hyperbolic tangent         | `tanh(1)`    |
| `asinh(v)` | Inverse hyperbolic sine    | `asinh(1)`   |
| `acosh(v)` | Inverse hyperbolic cosine  | `acosh(2)`   |
| `atanh(v)` | Inverse hyperbolic tangent | `atanh(0.5)` |

---

## Aggregation & Product

| Function                           | Description                                        | Example                                                                |
| ---------------------------------- | -------------------------------------------------- | ---------------------------------------------------------------------- |
| `product(iterable, start, policy)` | Product of elements. Default is `Overflow::Throw`. | `product(list(1,2,3))`<br>`product(list(1,2,3), 1, Overflow::Promote)` |

---

## Degree/Radian Conversion

| Function           | Description        | Example         |
| ------------------ | ------------------ | --------------- |
| `radians(degrees)` | Degrees to radians | `radians(180)`  |
| `degrees(radians)` | Radians to degrees | `degrees(pi())` |

---

## Advanced Math

| Function               | Description                                          | Example                                             |
| ---------------------- | ---------------------------------------------------- | --------------------------------------------------- |
| `gcd(a, b)`            | Greatest common divisor                              | `gcd(12, 18)`                                       |
| `lcm(a, b, policy)`    | Least common multiple. Default is `Overflow::Throw`. | `lcm(6, 8)`<br>`lcm(6, 8, Overflow::Wrap)`          |
| `factorial(n, policy)` | Factorial. Default is `Overflow::Throw`.             | `factorial(5)`<br>`factorial(5, Overflow::Promote)` |

---

## Constants

| Constant | Description          | Example |
| -------- | -------------------- | ------- |
| `pi()`   | Archimedes' constant | `pi()`  |
| `e()`    | Euler's number       | `e()`   |

---

## Random Functions

| Function                                | Description                  | Example                         |
| --------------------------------------- | ---------------------------- | ------------------------------- |
| `random_int(min, max)`                  | Random integer in [min, max] | `random_int(1, 10)`             |
| `random_float(min, max)`                | Random float in [min, max)   | `random_float(0, 1)`            |
| `random_choice(list)`                   | Random element from list     | `random_choice(list(1,2,3))`    |
| `random_choice_set(set)`                | Random element from set      | `random_choice_set(set(1,2,3))` |
| `fill_random(count, min, max)`          | List of random ints          | `fill_random(5, 1, 10)`         |
| `fill_randomf(count, min, max)`         | List of random floats        | `fill_randomf(5, 0, 1)`         |
| `fill_randomn(count, mean, stddev)`     | List of normal floats        | `fill_randomn(5, 0, 1)`         |
| `fill_random_set(count, min, max)`      | Set of random ints           | `fill_random_set(5, 1, 10)`     |
| `fill_randomf_set(count, min, max)`     | Set of random floats         | `fill_randomf_set(5, 0, 1)`     |
| `fill_randomn_set(count, mean, stddev)` | Set of normal floats         | `fill_randomn_set(5, 0, 1)`     |

## Calculator API

| Function        | Description                            | Example                                  |
| --------------- | -------------------------------------- | ---------------------------------------- |
| `calculator()`  | Starts interactive CLI calculator      | `pythonic::calculator::calculator();`    |
| `Calculator`    | Calculator class for programmatic use  | `pythonic::calculator::Calculator calc;` |
| `process(line)` | Process a line (expression/assignment) | `calc.process("var a=10. a+5");`         |

---

## Notes

- For arithmetic, power, lcm, factorial, and product, the `policy` parameter controls overflow: `Overflow::Throw` (default), `Overflow::Promote`, or `Overflow::Wrap`.
- Helper functions like `lcm_throw`, `lcm_promote`, `lcm_wrap`, etc., are shortcuts for common overflow policies.
- All functions operate on `var` objects and support mixed numeric types.
- Constants like `pi()` and `e()` return high-precision values as `var`.
- Random functions use a thread-local engine for thread safety.

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{

    var a = 10, b = 3, c = std::numeric_limits<int>::max();
    print(add(a, b));           // 13
    try
    {
        print(add(a, c)); // overflow
    }
    catch (const pythonic::error::PythonicOverflowError& e)
    {
        print("Overflow error: ", e.what());
    }
    print(a,"+",c,"=",add(a,c,Overflow::Wrap)); // wrap -2147483639
    print(a,"+",c,"=",add(a,c,Overflow::Promote)); // promote 2147483657
    print(sub(a, b));           // 7
    print(mul(a, b));           // 30
    print(div(a, b));           // 3
    print(mod(a, b));           // 1
    print(pow(a, b));           // 1000
    print(sqrt(16));            // 4
    print(round(3.1415));       // 3
    print(floor(3.7));          // 3
    print(ceil(3.1));           // 4
    print(trunc(-3.7));         // -3
    print(fabs(-5));            // 5
    print(hypot(3, 4));         // 5
    print(exp(1));              // 2.718...
    print(log(e()));            // 1
    print(log10(100));          // 2
    print(log2(8));             // 3
    print(sin(pi()));           // 0
    print(cos(0));              // 1
    print(tan(pi()/4));         // 1
    print(gcd(12, 18));         // 6
    print(lcm(6, 8));           // 24
    print(factorial(5));        // 120
    print(random_int(1, 6));    // random int 1-6
    print(random_float(0, 1));  // random float 0-1
    print(random_choice(list(1,2,3)));
    print(product(list(1,2,3,4)));// 24
    print(radians(180));        // 3.1415...
    print(degrees(pi()));       // 180
    return 0;
}

```

---

## Next Check

- [Error](../Errors/errors.md)
