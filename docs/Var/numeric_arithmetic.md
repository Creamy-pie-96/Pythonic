[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to String methods](string_like_methods.md)

# Numeric / Arithmetic

This page documents all user-facing numeric and arithmetic APIs and operators for `var`, including overloads for containers (`list`, `set`, etc.), in a clear tabular format with concise examples.

---

## Dispatch Table Architecture (Internal)

Arithmetic and comparison operators use a **dispatch table system** for O(1) type-based dispatch:

| Component       | Description                                                                                      |
| --------------- | ------------------------------------------------------------------------------------------------ |
| `TypeTag` enum  | 18 type tags: `NONE`, `BOOL`, `INT`, `LONGLONG`, `DOUBLE`, `STRING`, `LIST`, `SET`, `DICT`, etc. |
| Dispatch tables | 18×18 function pointer matrices for each operator (`add`, `sub`, `mul`, `div`, etc.)             |
| Lookup          | `dispatch::add_table[lhs_tag][rhs_tag](lhs, rhs)` - constant time dispatch                       |
| Extensibility   | New type combinations added via `pythonicDispatchStubs.cpp`                                      |

This replaces the previous if/else chain with a much faster table lookup approach.

---

## Arithmetic Operators (Scalars & Containers)

### Supported Type Combinations

The dispatch table supports **324 type combinations** (18×18) for arithmetic operators. Here are the key supported operations:

| Operator | Numeric × Numeric | String × String | String × Int | List × List | List × Int | Set × Set | Dict × Dict |
| -------- | ----------------- | --------------- | ------------ | ----------- | ---------- | --------- | ----------- |
| `+`      | ✅ addition       | ✅ concatenate  | —            | ✅ concat   | —          | —         | —           |
| `-`      | ✅ subtraction    | —               | —            | ✅ diff     | —          | ✅ diff   | ✅ diff     |
| `*`      | ✅ multiplication | —               | ✅ repeat    | —           | ✅ repeat  | —         | —           |
| `/`      | ✅ division       | —               | —            | —           | —          | —         | —           |
| `%`      | ✅ modulo         | —               | —            | —           | —          | —         | —           |

### Operator Reference

| Operator     | Description              | Example (Scalars)        | Example (Containers)                                                        |
| ------------ | ------------------------ | ------------------------ | --------------------------------------------------------------------------- |
| `+`          | Addition / concatenation | `var(2) + var(3) // 5`   | `list(1,2) + list(3,4) // [1,2,3,4]`<br>`"hello" + "world" // "helloworld"` |
| `-`          | Subtraction / difference | `var(5) - var(2) // 3`   | `set(1,2,3) - set(2) // {1,3}`<br>`dict(...) - dict(...) // key diff`       |
| `*`          | Multiplication / repeat  | `var(2) * var(3) // 6`   | `list(1,2) * 2 // [1,2,1,2]`<br>`"ab" * 3 // "ababab"`                      |
| `/`          | Division                 | `var(6) / var(2) // 3.0` | (numeric only)                                                              |
| `%`          | Modulo                   | `var(7) % var(3) // 1`   | (numeric only)                                                              |
| `-` (unary)  | Negation                 | `-var(5) // -5`          | —                                                                           |
| `+` (unary)  | Unary plus               | `+var(5) // 5`           | —                                                                           |
| `abs()`      | Absolute value           | `abs(var(-3)) // 3`      | —                                                                           |
| `operator+=` | In-place addition        | `v += 2`                 | `l += list(3,4)`                                                            |
| `operator-=` | In-place subtraction     | `v -= 2`                 | `s -= set(2)`                                                               |
| `operator*=` | In-place multiplication  | `v *= 2`                 | `l *= 2`                                                                    |
| `operator/=` | In-place division        | `v /= 2`                 | —                                                                           |
| `operator%=` | In-place modulo          | `v %= 2`                 | —                                                                           |

### Special Type Operations

#### String Operations

```cpp
var s = "hello";
print(s + " world");     // "hello world" (concatenation)
print(s * 3);            // "hellohellohello" (repeat)
print(3 * s);            // "hellohellohello" (repeat, commutative)
```

#### List Operations

```cpp
var l1 = list(1, 2);
var l2 = list(3, 4);
print(l1 + l2);          // [1, 2, 3, 4] (concatenation)
print(l1 * 2);           // [1, 2, 1, 2] (repeat)
print(l1 - l2);          // [] (difference - elements in l1 not in l2)
```

#### Set/Dict Operations

```cpp
var s1 = set(1, 2, 3);
var s2 = set(2, 3, 4);
print(s1 - s2);          // {1} (difference)

var d1 = dict({{1, "a"}, {2, "b"}});
var d2 = dict({{2, "c"}});
print(d1 - d2);          // {1: "a"} (keys in d1 not in d2)
```

---

## Bitwise Operators (Scalars & Containers)

### Supported Type Combinations

| Operator | Integral × Integral | Bool × Bool | Set × Set       | OrderedSet × OrderedSet | List × List     | Dict × Dict    |
| -------- | ------------------- | ----------- | --------------- | ----------------------- | --------------- | -------------- |
| `&`      | ✅ bitwise AND      | ✅ AND      | ✅ intersection | ✅ intersection         | ✅ intersection | ✅ common keys |
| `\|`     | ✅ bitwise OR       | ✅ OR       | ✅ union        | ✅ union                | ✅ concat       | ✅ merge       |
| `^`      | ✅ bitwise XOR      | ✅ XOR      | ✅ sym diff     | ✅ sym diff             | ✅ sym diff     | —              |
| `<<`     | ✅ left shift       | —           | —               | —                       | —               | —              |
| `>>`     | ✅ right shift      | —           | —               | —                       | —               | —              |

### Container Bitwise Semantics

#### Set Bitwise Operations

```cpp
var s1 = set(1, 2, 3);
var s2 = set(2, 3, 4);

print(s1 & s2);    // {2, 3}    - intersection (elements in both)
print(s1 | s2);    // {1, 2, 3, 4} - union (elements in either)
print(s1 ^ s2);    // {1, 4}    - symmetric difference (elements in one but not both)
```

#### OrderedSet Bitwise Operations (Order-Preserving)

```cpp
var os1 = orderedset(1, 2, 3);
var os2 = orderedset(2, 3, 4);

// Uses merge-like algorithm preserving sorted order
print(os1 & os2);  // {2, 3}    - intersection (sorted)
print(os1 | os2);  // {1, 2, 3, 4} - union (sorted)
print(os1 ^ os2);  // {1, 4}    - symmetric difference (sorted)
```

#### List Bitwise Operations

```cpp
var l1 = list(1, 2, 3, 2);
var l2 = list(2, 3, 4);

// Note: List operations preserve order and duplicates from first operand
print(l1 & l2);    // [2, 3, 2] - elements in l1 that are also in l2
print(l1 | l2);    // [1, 2, 3, 2, 2, 3, 4] - concatenation (like Python's +)
print(l1 ^ l2);    // [1, 4]    - elements in one list but not the other
```

#### Dict Bitwise Operations

```cpp
var d1 = dict({{1, "a"}, {2, "b"}, {3, "c"}});
var d2 = dict({{2, "x"}, {3, "y"}, {4, "z"}});

print(d1 & d2);    // {2: "b", 3: "c"} - keys in both (values from d1)
print(d1 | d2);    // {1: "a", 2: "x", 3: "y", 4: "z"} - merge (d2 overwrites d1)
// Note: ^ (XOR) is NOT supported for dict - will throw PythonicTypeError
```

#### OrderedDict Bitwise Operations (Order-Preserving)

```cpp
var od1 = ordereddict({{1, "a"}, {2, "b"}});
var od2 = ordereddict({{2, "x"}, {3, "z"}});

print(od1 & od2);  // {2: "b"} - intersection preserving order from od1
print(od1 | od2);  // {1: "a", 2: "x", 3: "z"} - merge, od2 overwrites, preserves order
// Note: ^ (XOR) is NOT supported for ordereddict
```

#### Bool Bitwise Operations

```cpp
var t = true;
var f = false;

// Bool bitwise returns bool (not int like in C++)
print(t & f);      // false
print(t | f);      // true
print(t ^ f);      // true
print(t ^ t);      // false
```

### Operator Reference

| Operator      | Description                  | Example (Scalars)        | Example (Containers)               |
| ------------- | ---------------------------- | ------------------------ | ---------------------------------- |
| `&`           | Bitwise AND / intersection   | `var(6) & var(3) // 2`   | `set(1,2,3) & set(2,3,4) // {2,3}` |
| `\|`          | Bitwise OR / union           | `var(6) \| var(3) // 7`  | `set(1,2) \| set(2,3) // {1,2,3}`  |
| `^`           | Bitwise XOR / symmetric diff | `var(6) ^ var(3) // 5`   | `set(1,2,3) ^ set(2,3,4) // {1,4}` |
| `~`           | Bitwise NOT (unary)          | `~var(6) // -7`          | —                                  |
| `<<`          | Left shift                   | `var(2) << var(3) // 16` | —                                  |
| `>>`          | Right shift                  | `var(8) >> var(2) // 2`  | —                                  |
| `operator&=`  | In-place AND                 | `v &= 2`                 | `s &= set(2,3)`                    |
| `operator\|=` | In-place OR                  | `v \|= 2`                | `s \|= set(4)`                     |
| `operator^=`  | In-place XOR                 | `v ^= 2`                 | `s ^= set(1)`                      |
| `operator<<=` | In-place left shift          | `v <<= 2`                | —                                  |
| `operator>>=` | In-place right shift         | `v >>= 2`                | —                                  |

---

## Numeric Helpers

| Function         | Description          | Example (Scalar)           | Example (Container)     |
| ---------------- | -------------------- | -------------------------- | ----------------------- |
| `abs(v)`         | Absolute value       | `abs(var(-3)) // 3`        | —                       |
| `min(a, b, ...)` | Minimum of arguments | `min(var(1), var(2)) // 1` | `min(list(3,1,2)) // 1` |
| `max(a, b, ...)` | Maximum of arguments | `max(var(1), var(2)) // 2` | `max(list(3,1,2)) // 3` |
| `sum(list)`      | Sum of list elements | —                          | `sum(list(1,2,3)) // 6` |

---

## Type Access & Conversion Methods

The `var` class provides methods to access the underlying value and convert between types:

### Direct Type Access (`get<T>()`)

| Method         | Description                                         | Returns          |
| -------------- | --------------------------------------------------- | ---------------- |
| `get<T>()`     | Returns a reference to the stored value as type `T` | `T&` (reference) |
| `var_get<T>()` | Internal method, same as `get<T>()`                 | `T&` (reference) |

```cpp
var v = 42;
int& ref = v.get<int>();   // Get reference to stored int
ref = 100;                   // Modifies v directly
print(v);                    // 100
```

### Type Conversion Methods

| Method           | Description                   | Returns              |
| ---------------- | ----------------------------- | -------------------- |
| `toInt()`        | Convert to int                | `int`                |
| `toUInt()`       | Convert to unsigned int       | `unsigned int`       |
| `toLong()`       | Convert to long               | `long`               |
| `toULong()`      | Convert to unsigned long      | `unsigned long`      |
| `toLongLong()`   | Convert to long long          | `long long`          |
| `toULongLong()`  | Convert to unsigned long long | `unsigned long long` |
| `toFloat()`      | Convert to float              | `float`              |
| `toDouble()`     | Convert to double             | `double`             |
| `toLongDouble()` | Convert to long double        | `long double`        |
| `toBool()`       | Convert to bool               | `bool`               |
| `toStr()`        | Convert to string             | `std::string`        |

```cpp
var v = 3.14159;
int i = v.toInt();           // 3 (truncated)
double d = v.toDouble();     // 3.14159
std::string s = v.toStr();   // "3.14159"

var str = "42";
int num = str.toInt();       // 42 (parsed from string)
```

### Type Checking Methods

| Method      | Description                 | Returns       |
| ----------- | --------------------------- | ------------- |
| `is<T>()`   | Check if var holds type `T` | `bool`        |
| `type()`    | Get type name as string     | `std::string` |
| `typeTag()` | Get internal type tag       | `TypeTag`     |

```cpp
var v = 42;
print(v.is<int>());          // true
print(v.is<double>());       // false
print(v.type());             // "int"
print(v.typeTag());          // TypeTag::INT
```

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
    var a = 5, b = 2;
print(a + b);         // 7
print(a - b);         // 3
print(a * b);         // 10
print(a / b);         // 2.5
//Btw it will automatically promote types and will not do int division. Like check this out:
var c = a/b;
print("a : ","(",a,",",a.type(),")","b : ","(",b,",",b.type(),")","c : ","(",c,",",c.type(),")");             // a :  (5,int) b :  (2,int) c :  (2.5,double)
print(a % b);         // 1
print(-a);            // -5
print(+a);            // 5
print(abs(-a));       // 5

// Container arithmetic
var l = list(1,2);
print(l + list(3,4)); // [1,2,3,4]
l *= 2; print(l);     // [1,2,1,2]

var s = set(1,2,3);
//print(s + set(2,4));  // {1,2,3,4}
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
//print(abs(list(-1,2,-3)));        // [1,2,3] is this what you thought will happen? What am I supposed to abs anyway? Don't be silly or I will crash you program
print(min(var(1), var(2), var(3),3.13)); // 1
print(max(var(1), var(2), var(3),3.13)); // 3.13
//print(max(var(1), var(2), var(3),3.13,"hello"));
/* Do not try this at home, at office or at any place or the ghost of error will haunt you : terminate called after throwing an instance of 'pythonic::error::PythonicTypeError'
  what():  pythonic: TypeError: operator> not supported for these types. Cannot perform 'str > double'.
Aborted (core dumped)*/
print(min(list(3,1,2)));            // 1
print(max(list(3,1,2)));            // 3
print(sum(list(1,2,3)));            // 6
    return 0;
}


```

---

## Notes

- All arithmetic and bitwise operators are overloaded for `var` and work with numeric types and containers where appropriate.
- Compound assignment operators (`+=`, `-=`, etc.) are supported.
- Use `abs`, `min`, `max`, `sum` for numeric helpers.
- **Operator overloads use `Overflow::Throw`** by default for safety.
- **Math functions use `Overflow::Throw`** by default for safety.
- Use `get<T>()` to get a reference to the stored value (for modification).
- Use `toInt()`, `toDouble()`, etc. for type conversion (returns a copy).
- Use `is<T>()` to check the type before accessing.

## Next check

- [Comparison & Boolean](comparison_and_boolean.md)
