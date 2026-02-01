[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Error](../Errors/errors.md)

# Iteration Helpers

This page documents all user-facing iteration helpers in Pythonic, including range, enumerate, zip, reversed, loop helpers, and utility functions, with concise examples.

---

## Mutation Detection (Python-like Safety)

Starting with the latest version, all `var` container types (`list`, `set`, `dict`, `ordered_set`, `ordered_dict`, `string`) now have **Python-like mutation detection during iteration**. If you modify a container while iterating over it, a `PythonicRuntimeError` will be thrown.

| Behavior                         | Description                                                                | Example                                            |
| -------------------------------- | -------------------------------------------------------------------------- | -------------------------------------------------- |
| Mutation during iteration throws | Modifying a container (append, clear, remove, etc.) while iterating throws | `for (auto x : lst) { lst.append(1); } // THROWS!` |
| Normal iteration is safe         | Non-mutating iteration works as expected                                   | `for (auto x : lst) { print(x); } // OK`           |
| Iteration version tracking       | Each container tracks a version counter that increments on mutation        | `lst.get_version()`                                |

**Note:** This safety applies to all iteration methods: range-based for, `enumerate()`, `indexed()`, `each()`, `each_indexed()`, and the deprecated macros.

---

## Range & Views

| Function / Macro                 | Description                                                            | Example                                                |
| -------------------------------- | ---------------------------------------------------------------------- | ------------------------------------------------------ |
| `range(end)`                     | Range from 0 to end-1                                                  | `for (auto i : range(10)) { ... }`                     |
| `range(start, end)`              | Range from start to end-1                                              | `for (auto i : range(1, 10)) { ... }`                  |
| `range(start, end, step)`        | Range with explicit step                                               | `for (auto i : range(10, 0, -1)) { ... }`              |
| `views::take_n(r, n)`            | View of first n elements                                               | `for (auto x : views::take_n(lst, 5)) { ... }`         |
| `views::drop_n(r, n)`            | View dropping first n elements                                         | `for (auto x : views::drop_n(lst, 2)) { ... }`         |
| `views::filter_view(r, pred)`    | Filtered view by predicate                                             | `for (auto x : views::filter_view(lst, pred)) { ... }` |
| `views::transform_view(r, func)` | Transformed view by function                                           | `for (auto x : views::transform_view(lst, f)) { ... }` |
| `views::reverse_view(r)`         | Reverse view (lazy, no copy, only for std containers with rbegin/rend) | _Not supported for pythonic::vars::var or list_        |
| `views::iota_view(start, end)`   | Integer range view                                                     | `for (auto i : views::iota_view(0, 5)) { ... }`        |

---

## Enumerate & Zip

| Function / Macro                | Description                                          | Example                                                  |
| ------------------------------- | ---------------------------------------------------- | -------------------------------------------------------- |
| `enumerate(container, start=0)` | Index/value pairs (safe with mutation detection)     | `for (auto [i, x] : enumerate(lst)) { ... }`             |
| `indexed(container)`            | Index/value pairs (alias for enumerate, class-based) | `for (auto [i, x] : indexed(lst)) { ... }`               |
| `views::enumerate_view(r)`      | Enumerated view (C++20)                              | `for (auto [i, x] : views::enumerate_view(lst)) { ... }` |
| `zip(a, b, ...)`                | Zip multiple containers                              | `for (auto [x, y] : zip(lst1, lst2)) { ... }`            |

---

## Reversed

| Function / Macro      | Description                                                  | Example                                         |
| --------------------- | ------------------------------------------------------------ | ----------------------------------------------- |
| `reversed(container)` | Reverse iteration (only for std containers with rbegin/rend) | _Not supported for pythonic::vars::var or list_ |

---

## Class-Based Loop Helpers (Recommended)

These are type-safe, template-based alternatives to macros. They provide better IDE support, compile-time error checking, and integrate with mutation detection.

| Function                        | Description                                                       | Example                                                      |
| ------------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------ |
| `each(container, func)`         | Apply function to each element (early exit if func returns false) | `each(lst, [](auto& x) { print(x); });`                      |
| `each_indexed(container, func)` | Apply function with index (early exit if func returns false)      | `each_indexed(lst, [](size_t i, auto& x) { print(i, x); });` |
| `indexed(container)`            | Returns wrapper yielding (index, value) pairs for range-for       | `for (auto [i, x] : indexed(lst)) { ... }`                   |
| `times(n, func)`                | Execute func n times, with optional index parameter               | `times(10, [](size_t i) { print(i); });`                     |
| `until(container, pred)`        | Iterate until predicate returns true                              | `until(lst, [](auto& x) { return x == 5; });`                |
| `while_each(container, pred)`   | Iterate while predicate returns true                              | `while_each(lst, [](auto& x) { return x < 5; });`            |

---

## Loop Macros (Deprecated - use class-based helpers)

These macros are kept for backward compatibility but are **not type-safe**. Prefer the class-based helpers above.

| Macro                                | Description                              | Recommended Alternative                            |
| ------------------------------------ | ---------------------------------------- | -------------------------------------------------- |
| `for_each(var, container)`           | Range-based for loop                     | `for (auto x : container) { ... }`                 |
| `for_index(idx, container)`          | Loop with index                          | `for (auto [i, x] : indexed(container)) { ... }`   |
| `for_enumerate(idx, val, container)` | Enumerate style loop                     | `for (auto [i, x] : enumerate(container)) { ... }` |
| `for_range(var, ...)`                | Python-like for in range                 | `for (auto i : range(...)) { ... }`                |
| `while_true`                         | Infinite loop (like Python's while True) | `while (true) { ... }`                             |

---

## Utility Functions

| Function            | Description                      | Example             |
| ------------------- | -------------------------------- | ------------------- |
| `len(container)`    | Get length of container or range | `len(lst)`          |
| `to_list(iterable)` | Convert any iterable to a list   | `to_list(range(5))` |
| `sum(iterable)`     | Sum of elements                  | `sum(list(1,2,3))`  |
| `min(iterable)`     | Minimum element                  | `min(list(1,2,3))`  |
| `max(iterable)`     | Maximum element                  | `max(list(1,2,3))`  |
| `any(iterable)`     | True if any element is truthy    | `any(list(0,1,0))`  |
| `all(iterable)`     | True if all elements are truthy  | `all(list(1,1,1))`  |

---

## Examples

```cpp
#include "pythonic/pythonicLoop.hpp"
using namespace pythonic::loop;
using namespace pythonic::vars;

// --- Range ---
for (auto i : range(5)) { print(i); } // 0 1 2 3 4
for (auto i : range(1, 5)) { print(i); } // 1 2 3 4
for (auto i : range(10, 0, -2)) { print(i); } // 10 8 6 4 2

// --- Class-based helpers (recommended) ---
var lst = list(10, 20, 30);

// each() - clean iteration with lambdas
each(lst, [](var& x) { print(x); });

// each() with early exit
each(lst, [](var& x) -> bool {
    if (x.toInt() > 15) return false; // stop
    print(x);
    return true;
});

// each_indexed() - iteration with index
each_indexed(lst, [](size_t i, var& x) {
    print(i, ":", x);
});

// indexed() - range-based for with index
for (auto [i, x] : indexed(lst)) {
    print(i, x);
}

// times() - repeat N times
times(5, [](size_t i) { print("iteration", i); });
times(3, []() { print("hello"); });

// --- Enumerate ---
for (auto [i, x] : enumerate(lst)) { print(i, x); }
for (auto [i, x] : enumerate(lst, 100)) { print(i, x); } // start index at 100

// --- Zip ---
var names = list("Alice", "Bob");
var ages = list(25, 30);
for (auto [name, age] : zip(names, ages)) { print(name, "is", age); }

// --- Mutation Detection ---
var mutable_lst = list(1, 2, 3);
try {
    for (auto x : mutable_lst) {
        mutable_lst.append(4); // THROWS PythonicRuntimeError!
    }
} catch (const PythonicRuntimeError& e) {
    print("Caught:", e.what());
}

// Safe: mutation after iteration is fine
for (auto x : mutable_lst) { print(x); }
mutable_lst.append(4); // OK - iteration is done

// --- Reversed (for std containers) ---
std::vector<int> v = {1, 2, 3};
for (auto x : reversed(v)) { print(x); } // 3 2 1

// For var/list, use slicing with negative step instead:
for (auto x : lst.slice(var(), var(), -1)) { print(x); } // 30 20 10

// --- Utility Functions ---
print(len(list(1,2,3)));       // 3
print(to_list(range(5)));      // [0,1,2,3,4]
print(sum(list(1,2,3)));       // 6
print(min(list(1,2,3)));       // 1
print(max(list(1,2,3)));       // 3
print(any(list(0,1,0)));       // true
print(all(list(1,1,1)));       // true
```

---

## Notes

- **Mutation detection is now enabled** for all `var` container types. Modifying during iteration throws.
- **Prefer class-based helpers** (`each()`, `each_indexed()`, `indexed()`, `times()`) over macros for type safety and IDE support.
- **Negative indices and negative step** are supported in `var`/list slicing for reverse iteration.
- Loop macros are deprecated but kept for backward compatibility.
- Views are lazy and efficient; use them for large data or pipelines.
- Utility functions operate on any iterable, including ranges and views.

---

## Next Check

- [Functional](../Functional/functional.md)
