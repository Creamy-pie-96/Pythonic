[⬅ Back to Table of Contents](index.md)
[⬅ Back to Error](../Errors/errors.md)

# Iteration Helpers

This page documents all user-facing iteration helpers in Pythonic, including range, enumerate, zip, reversed, loop macros, and utility functions, with concise examples.

---

## Range & Views

| Function / Macro                 | Description                                                            | Example                                                |
| -------------------------------- | ---------------------------------------------------------------------- | ------------------------------------------------------ |
| `range(end)`                     | Range from 0 to end-1                                                  | `for_range(i, 10) { ... }`                             |
| `range(start, end)`              | Range from start to end-1                                              | `for_range(i, 1, 10) { ... }`                          |
| `range(start, end, step)`        | Range with explicit step                                               | `for_range(i, 10, 0, -1) { ... }`                      |
| `views::take_n(r, n)`            | View of first n elements                                               | `for (auto x : views::take_n(lst, 5)) { ... }`         |
| `views::drop_n(r, n)`            | View dropping first n elements                                         | `for (auto x : views::drop_n(lst, 2)) { ... }`         |
| `views::filter_view(r, pred)`    | Filtered view by predicate                                             | `for (auto x : views::filter_view(lst, pred)) { ... }` |
| `views::transform_view(r, func)` | Transformed view by function                                           | `for (auto x : views::transform_view(lst, f)) { ... }` |
| `views::reverse_view(r)`         | Reverse view (lazy, no copy, only for std containers with rbegin/rend) | _Not supported for pythonic::vars::var or list_        |
| `views::iota_view(start, end)`   | Integer range view                                                     | `for (auto i : views::iota_view(0, 5)) { ... }`        |

---

## Enumerate & Zip

| Function / Macro                | Description                                                              | Example                                                                          |
| ------------------------------- | ------------------------------------------------------------------------ | -------------------------------------------------------------------------------- |
| `enumerate(container, start=0)` | Index/value pairs (not for pythonic::vars::var or dynamic list)          | `for_enumerate(i, x, lst) { ... }` (not for pythonic::vars::var or dynamic list) |
| `views::enumerate_view(r)`      | Enumerated view (C++20, works with pythonic::vars::var and dynamic list) | `for (auto [i, x] : views::enumerate_view(lst)) { ... }`                         |
| `zip(a, b, ...)`                | Zip multiple containers                                                  | `for (auto [x, y] : zip(lst1, lst2)) { ... }`                                    |

---

## Reversed

| Function / Macro      | Description                                                  | Example                                         |
| --------------------- | ------------------------------------------------------------ | ----------------------------------------------- |
| `reversed(container)` | Reverse iteration (only for std containers with rbegin/rend) | _Not supported for pythonic::vars::var or list_ |

---

## Loop Macros

| Macro                                | Description                              | Example                            |
| ------------------------------------ | ---------------------------------------- | ---------------------------------- |
| `for_each(var, container)`           | Range-based for loop                     | `for_each(x, lst) { ... }`         |
| `for_index(idx, container)`          | Loop with index                          | `for_index(i, lst) { ... }`        |
| `for_enumerate(idx, val, container)` | Enumerate style loop                     | `for_enumerate(i, x, lst) { ... }` |
| `for_range(var, ...)`                | Python-like for in range                 | `for_range(i, 10) { ... }`         |
| `while_true`                         | Infinite loop (like Python's while True) | `while_true { ... }`               |

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

// --- Range ---
for_range(i, 5) { print(i); } // 0 1 2 3 4
for_range(i, 1, 5) { print(i); } // 1 2 3 4
for_range(i, 10, 0, -2) { print(i); } // 10 8 6 4 2

// --- Enumerate ---
// NOTE: for_enumerate macro is not compatible with pythonic::vars::var or dynamic list. Use enumerate_view for those.
for_enumerate(i, x, std::vector<int>{10, 20, 30}) { print(i, x); }
for (auto [i, x] : views::enumerate_view(list(10, 20, 30))) { print(i, x); }

// --- Zip ---
for (auto [x, y] : zip(list(1,2,3), list("a","b","c"))) { print(x, y); }

// --- Reversed ---
// Not supported for pythonic::vars::var or list. Use slicing with negative step for reverse iteration:
for (auto x : list(1,2,3).slice(var(), var(), -1)) { print(x); } // 3 2 1

// --- Loop Macros ---
for_each(x, list(1,2,3)) { print(x); }
for_index(i, list(1,2,3)) { print(i, list(1,2,3)[i]); }
while_true { break; }

// --- Utility Functions ---
print(len(list(1,2,3)));
print(to_list(range(5)));
print(sum(list(1,2,3)));
print(min(list(1,2,3)));
print(max(list(1,2,3)));
print(any(list(0,1,0)));
print(all(list(1,1,1)));
```

---

## Notes

- All iteration helpers work with standard containers, Pythonic containers, and C++20 ranges, except reverse iteration for pythonic::vars::var/list (use .slice with negative step instead). -**Negative indices and negative step are supported in pythonic::vars::var/list slicing:** - `lst[-1]` gives the last element, `lst.slice(var(), var(), -1)` reverses the list, etc.
- Loop macros provide Python-like syntax for common iteration patterns.
- Views are lazy and efficient; use them for large data or pipelines.
- Utility functions operate on any iterable, including ranges and views.

---

## Next Check

- [Functional](../Functional/functional.md)
