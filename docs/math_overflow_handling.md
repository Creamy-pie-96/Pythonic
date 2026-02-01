# Pythonic Math Overflow Handling Documentation

## Overview

The math operations in `pythonicMath.hpp` (e.g., `add`, `sub`, `mul`, `div`, `mod`, etc.) support configurable overflow handling via the `Overflow` enum and a `smallest_fit` flag. This allows you to control how arithmetic overflows are managed for dynamic types.

## Overflow Policy: `pythonic::overflow::Overflow`

| Policy         | Enum Value | Description                                                                   |
| -------------- | ---------- | ----------------------------------------------------------------------------- |
| `Throw`        | 0          | Throws a `PythonicOverflowError` on overflow. **Default for math functions.** |
| `Promote`      | 1          | Automatically promotes the result to a larger type if overflow occurs.        |
| `Wrap`         | 2          | Allows wrapping (C++ default behavior), so overflowed values wrap around.     |
| `None_of_them` | 3          | Raw C++ arithmetic with no checks. **Default for operator overloads.**        |

### Policy Selection Guide

| Use Case                            | Recommended Policy |
| ----------------------------------- | ------------------ |
| Production code (safety first)      | `Throw`            |
| Scientific computing (large values) | `Promote`          |
| Performance-critical inner loops    | `None_of_them`     |
| Embedded/systems programming        | `Wrap`             |

### Default Behavior

- **Math functions** (`add()`, `sub()`, `mul()`, etc.) default to `Overflow::Throw` for safety.
- **Operator overloads** (`+`, `-`, `*`, etc.) use `Overflow::None_of_them` for maximum performance.

```cpp
// Math functions: safe by default (Throw)
add(INT_MAX, 1);     // Throws PythonicOverflowError

// Operators: fast by default (None_of_them)
var a = INT_MAX;
var b = a + 1;       // Raw C++ arithmetic (wraps silently)
```

## Function Signature Example

```cpp
var add_int_long(const var& a, const var& b, pythonic::overflow::Overflow policy = pythonic::overflow::Overflow::Throw, bool smallest_fit = false);
```

- `policy`: Controls overflow handling (see above).
- `smallest_fit`: If true, finds the smallest container that fits the result; if false, does not downgrade below the highest input type rank.

## Promotion Logic

- **Smart Promotion**: Computes in `long double`, then fits the result to the smallest container that can hold it.
- **Integer Promotion**:
  - If both inputs are unsigned and result ≥ 0: tries `uint → ulong → ulong_long → float → ...`
  - If any input is signed or result < 0: tries `int → long → long_long → float → ...`
- **Floating Point Promotion**: If any input is floating point or the operation is division, tries `float → double → long double`.

## Wrap Logic

- If `Overflow::Wrap` is selected, arithmetic operations use C++ default wrapping behavior (e.g., `int` wraps around on overflow).

## Throw Logic

- If `Overflow::Throw` is selected, the operation checks for overflow and throws a `PythonicOverflowError` if detected.

## Type Ranking System

- Used for `smallest_fit` feature.
- Ranking: `bool=0 < uint=1 < int=2 < ulong=3 < long=4 < ulong_long=5 < long_long=6 < float=7 < double=8 < long_double=9`
- Unsigned types have lower rank than signed counterparts.
- If either input is unsigned, result prefers unsigned containers.

## Example: Addition

- If adding two integers and the result fits in `int`, returns `int`.
- If result overflows `int` and `Overflow::Promote` is set, promotes to `long`, then `long_long`, etc.
- If `Overflow::Throw` is set, throws on overflow.
- If `Overflow::Wrap` is set, wraps around.
- If `Overflow::None_of_them` is set, uses raw C++ arithmetic (fastest, but may overflow silently).

## None_of_them Policy Details

The `None_of_them` policy (value `3`) performs raw C++ arithmetic without any overflow detection:

- **No bounds checking** - operates directly on underlying values
- **Maximum performance** - no additional computation overhead
- **Undefined behavior possible** - signed integer overflow is UB in C++
- **Best for**: Performance-critical code where you've pre-validated ranges

```cpp
// When you know overflow won't happen:
for (int i = 0; i < 1000000; ++i) {
    total = add(total, small_value, Overflow::None_of_them);  // Fastest
}

// Operator overloads use None_of_them internally:
var a = 10, b = 20;
var c = a + b;  // Uses None_of_them for speed
```

## Centralized Control

By passing `Overflow` and `smallest_fit` to dispatcher functions, you can centralize overflow and promotion logic, making it easier to control arithmetic behavior across all types.
