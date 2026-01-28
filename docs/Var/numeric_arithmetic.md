[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to String methods](string_like_methods.md)

# Numeric / Arithmetic

This page documents all user-facing numeric and arithmetic APIs and operators for `var`, including overloads for containers (`list`, `set`, etc.), in a clear tabular format with concise examples.

---

## Arithmetic Operators (Scalars & Containers)

| Operator         | Description                        | Example (Scalars)              | Example (Containers)                |
|------------------|------------------------------------|--------------------------------|-------------------------------------|
| `+`              | Addition                           | `var(2) + var(3) // 5`         | `list(1,2) + list(3,4) // [1,2,3,4]`<br>`set(1,2) + set(2,3) // {1,2,3}` |
| `-`              | Subtraction                        | `var(5) - var(2) // 3`         | `set(1,2,3) - set(2) // {1,3}`      |
| `*`              | Multiplication                     | `var(2) * var(3) // 6`         | `list(1,2) * 2 // [1,2,1,2]`        |
| `/`              | Division                           | `var(6) / var(2) // 3`         |                                     |
| `%`              | Modulo                             | `var(7) % var(3) // 1`         |                                     |
| `-` (unary)      | Negation                           | `-var(5) // -5`                |                                     |
| `+` (unary)      | Unary plus                         | `+var(5) // 5`                 |                                     |
| `abs()`          | Absolute value                     | `abs(var(-3)) // 3`            | `abs(list(-1,2,-3)) // [1,2,3]`     |
| `operator+=`     | In-place addition                  | `v += 2`                       | `l += list(3,4)`                    |
| `operator-=`     | In-place subtraction               | `v -= 2`                       | `s -= set(2)`                       |
| `operator*=`     | In-place multiplication            | `v *= 2`                       | `l *= 2`                            |
| `operator/=`     | In-place division                  | `v /= 2`                       |                                     |
| `operator%=`     | In-place modulo                    | `v %= 2`                       |                                     |
---

## Bitwise Operators (Scalars & Containers)

| Operator         | Description                        | Example (Scalars)              | Example (Containers)                |
|------------------|------------------------------------|--------------------------------|-------------------------------------|
| `&`              | Bitwise AND                        | `var(6) & var(3) // 2`         | `set(1,2,3) & set(2,3,4) // {2,3}`  |
| `\|`              | Bitwise OR                         | `var(6) \| var(3) // 7`         | `set(1,2) \| set(2,3) // {1,2,3}`    |
| `^`              | Bitwise XOR                        | `var(6) ^ var(3) // 5`         | `set(1,2,3) ^ set(2,3,4) // {1,4}`  |
| `~`              | Bitwise NOT (unary)                | `~var(6) // -7`                |                                     |
| `<<`             | Left shift                         | `var(2) << var(3) // 16`       |                                     |
| `>>`             | Right shift                        | `var(8) >> var(2) // 2`        |                                     |
| `operator&=`     | In-place AND                       | `v &= 2`                       | `s &= set(2,3)`                     |
| `operator\|=`     | In-place OR                        | `v \|= 2`                       | `s \|= set(4)`                       |
| `operator^=`     | In-place XOR                       | `v ^= 2`                       | `s ^= set(1)`                       |
| `operator<<=`    | In-place left shift                | `v <<= 2`                      |                                     |
| `operator>>=`    | In-place right shift               | `v >>= 2`                      |                                     |

---

## Numeric Helpers

| Function         | Description                        | Example (Scalar)               | Example (Container)                 |
|------------------|------------------------------------|--------------------------------|-------------------------------------|
| `abs(v)`         | Absolute value                     | `abs(var(-3)) // 3`            | `abs(list(-1,2,-3)) // [1,2,3]`     |
| `min(a, b, ...)` | Minimum of arguments               | `min(var(1), var(2)) // 1`     | `min(list(3,1,2)) // 1`             |
| `max(a, b, ...)` | Maximum of arguments               | `max(var(1), var(2)) // 2`     | `max(list(3,1,2)) // 3`             |
| `sum(list)`      | Sum of list elements               |                                | `sum(list(1,2,3)) // 6`             |

---

## Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace py;

// Scalar arithmetic
var a = 5, b = 2;
print(a + b);         // 7
print(a - b);         // 3
print(a * b);         // 10
print(a / b);         // 2
print(a % b);         // 1
print(-a);            // -5
print(+a);            // 5
print(abs(-a));       // 5

// Container arithmetic
var l = list(1,2);
print(l + list(3,4)); // [1,2,3,4]
l *= 2; print(l);     // [1,2,1,2]

var s = set(1,2,3);
print(s + set(2,4));  // {1,2,3,4}
print(s - set(2));    // {1,3}
s -= set(3); print(s);// {1,2}

// Bitwise set operations
var sa = set(1,2,3), sb = set(2,3,4);
print(sa & sb);       // {2,3}
print(sa | sb);       // {1,2,3,4}
print(sa ^ sb);       // {1,4}
sa &= set(2); print(sa); // {2}

// Numeric helpers
print(abs(var(-7)));              // 7
print(abs(list(-1,2,-3)));        // [1,2,3]
print(min(var(1), var(2), var(3))); // 1
print(max(var(1), var(2), var(3))); // 3
print(min(list(3,1,2)));            // 1
print(max(list(3,1,2)));            // 3
print(sum(list(1,2,3)));            // 6
```

---

## Notes

- All arithmetic and bitwise operators are overloaded for `var` and work with numeric types and containers where appropriate.
- Compound assignment operators (`+=`, `-=`, etc.) are supported.
- Use `abs`, `min`, `max`, `sum` for numeric helpers.

## Next check

- [Comparison & Boolean](comparison_and_boolean.md)