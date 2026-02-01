[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Numeric and arithmetic operators](numeric_arithmetic.md)

# Comparison & Boolean

This page documents all comparison and boolean operations for `var`, using a clear tabular format with concise examples.

---

## Comparison Operators

Comparison operators use the same **dispatch table system** as arithmetic operators for O(1) type-based dispatch.

### Supported Type Combinations

| Comparison | Numeric × Numeric | String × String | List × List     | Set × Set  | Dict × Dict | OrderedSet × OrderedSet | OrderedDict × OrderedDict |
| ---------- | ----------------- | --------------- | --------------- | ---------- | ----------- | ----------------------- | ------------------------- |
| `==`, `!=` |                   |                 |                 |            |             |                         |                           |
| `<`, `<=`  |                   |                 | (lexicographic) | (subset)   | —           | (lexicographic)         | (lexicographic)           |
| `>`, `>=`  |                   |                 | (lexicographic) | (superset) | —           | (lexicographic)         | (lexicographic)           |

### Container Comparison Semantics

#### List Comparison (Lexicographic)

```cpp
var l1 = list(1, 2, 3);
var l2 = list(1, 2, 4);
var l3 = list(1, 2, 3);

print(l1 == l3);    // true  - same elements in same order
print(l1 != l2);    // true  - different elements
print(l1 < l2);     // true  - [1,2,3] < [1,2,4] lexicographically
print(l1 <= l3);    // true  - equal means <=
print(l2 > l1);     // true  - [1,2,4] > [1,2,3]
print(l1 >= l3);    // true  - equal means >=
```

#### Set Comparison (Subset/Superset)

```cpp
var s1 = set(1, 2);
var s2 = set(1, 2, 3);
var s3 = set(1, 2);

print(s1 == s3);    // true  - same elements (order doesn't matter)
print(s1 != s2);    // true  - different elements
print(s1 < s2);     // true  - s1 is proper subset of s2
print(s1 <= s2);    // true  - s1 is subset of s2
print(s2 > s1);     // true  - s2 is proper superset of s1
print(s2 >= s1);    // true  - s2 is superset of s1
print(s1 <= s3);    // true  - equal sets are subsets of each other
```

#### Dict Comparison (Key-Value Equality)

```cpp
var d1 = dict({{"x", 1}, {"y", 2}});
var d2 = dict({{"y", 2}, {"x", 1}});  // Same keys/values, different order
var d3 = dict({{"x", 1}});

print(d1 == d2);    // true  - same key-value pairs (order doesn't matter for dict)
print(d1 != d3);    // true  - different keys/values
// Note: <, <=, >, >= are NOT supported for dict - will throw PythonicTypeError
```

#### OrderedSet Comparison (Lexicographic)

```cpp
var os1 = orderedset(1, 2, 3);
var os2 = orderedset(1, 2, 4);
var os3 = orderedset(1, 2, 3);

print(os1 == os3);  // true  - same elements in same order
print(os1 < os2);   // true  - lexicographic comparison
print(os2 > os1);   // true
```

#### OrderedDict Comparison (Lexicographic on pairs)

```cpp
var od1 = ordereddict({{1, "a"}, {2, "b"}});
var od2 = ordereddict({{1, "a"}, {2, "b"}});
var od3 = ordereddict({{2, "b"}, {1, "a"}});  // Different order!

print(od1 == od2);  // true  - same pairs in same order
print(od1 == od3);  // false - order matters for ordereddict
print(od1 < od3);   // Lexicographic comparison on pairs
```

### Operator Reference

| Operator                | Description                                    | Example                     |
| ----------------------- | ---------------------------------------------- | --------------------------- |
| `operator==`            | Pythonic equality (returns `var` bool)         | `a == b`                    |
| `operator!=`            | Pythonic inequality (returns `var` bool)       | `a != b`                    |
| `operator<`             | Pythonic less-than (returns `var` bool)        | `a < b`                     |
| `operator>`             | Pythonic greater-than (returns `var` bool)     | `a > b`                     |
| `operator<=`            | Pythonic less-or-equal (returns `var` bool)    | `a <= b`                    |
| `operator>=`            | Pythonic greater-or-equal (returns `var` bool) | `a >= b`                    |
| `operator<(const var&)` | Fast bool ordering for containers              | `std::set<var> s = {a, b};` |

---

## Logical & Boolean Operators

| Operator                           | Description                         | Example               |
| ---------------------------------- | ----------------------------------- | --------------------- | ------------------------------- | --- | --- | --- |
| `operator&&`                       | Logical AND (returns `var` bool)    | `a && b`              |
| `operator                          |                                     | `                     | Logical OR (returns `var` bool) | `a  |     | b`  |
| `operator!`                        | Logical NOT (returns `var` bool)    | `!a`                  |
| `operator bool()`                  | Truthiness for `if`/boolean context | `if (a) { ... }`      |
| `explicit operator int()/double()` | Numeric conversion                  | `int(a)`, `double(a)` |

---

## Comparison Semantics

- Same-type: direct value comparison (fast via `TypeTag`).
- Numeric types: promoted to `double` for comparison (e.g., `int` vs `double`).
- Containers: element-wise (lists), key/value (dicts), membership (sets/ordered sets).
- Different non-numeric types: not equal.
- Some comparisons throw `PythonicTypeError` if unsupported (e.g., `dict > list`).

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
    var a = 10;
var b = 10.0;
print(a == b);       // true (numeric promotion)
print(a != b);       // false
if(a) // any non-zero number is true
{
    print("any non-zero number is true");
}

if(!(b - 10))
{
    print("Any var with value 0 is false");
}

var s1 = "abc";
var s2 = "abc";
print(s1 == s2);     // true

var l1 = list(1,2,3);
var l2 = list(1,2,3);
var l3 = list(1,2,4);
print(l1 == l2);     // true
print(l1 == l3);     // false

var d1 = dict({{"x", 1}, {"y", 2}});
var d2 = dict({{"y", 2}, {"x", 1}});
print(d1 == d2);     // true

var empty = list();
if (empty) {
    print("non-empty");
} else {
    print("empty");   // empty
}

print(list(1) && list()); // false
print(list(1) || list()); // true

std::set<var> st = { var(2), var(1) };
for (auto &v : st) print(v); // 1 2

// Btw a lot of you might try do do this! And you will get an error! So be careful who you compare

try {
    print(dict({{"a",1}}) > list(1));
} catch (const pythonic::PythonicTypeError &e) {
    print("comparison error:", e.what());
}
    return 0;
}

```

---

## Notes

- Comparison operators return `var` for Python-like chaining and high-level APIs.
- Use `==`/`!=` for structural equality. Use `operator<(const var&)` for container ordering only.
- Truthiness is determined by `operator bool()`; use in `if` or cast to `bool` as needed.

## Next check

- [Iterators, Mapping & Functional Helpers](iterators_mapping_functional.md)
