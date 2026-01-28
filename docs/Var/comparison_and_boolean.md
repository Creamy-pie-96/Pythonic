[⬅ Back to Table of Contents](var.md)

# Comparison & Boolean

This page documents all comparison and boolean operations for `var`, using a clear tabular format with concise examples.

---

## Comparison Operators

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
#include "pythonic/pythonicVars.hpp"
#include "pythonic/pythonicPrint.hpp"
using namespace pythonic::vars;
using pythonic::print::print;

var a = 10;
var b = 10.0;
print(a == b);       // true (numeric promotion)
print(a != b);       // false

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

try {
    print(dict({{"a",1}}) > list(1));
} catch (const pythonic::PythonicTypeError &e) {
    print("comparison error:", e.what());
}
```

---

## Notes

- Comparison operators return `var` for Python-like chaining and high-level APIs.
- Use `==`/`!=` for structural equality. Use `operator<(const var&)` for container ordering only.
- Truthiness is determined by `operator bool()`; use in `if` or cast to `bool` as needed.

## Next check

- [Iterators, Mapping & Functional Helpers](iterators_mapping_functional.md)