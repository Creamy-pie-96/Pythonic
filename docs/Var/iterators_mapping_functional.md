[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Comparison and boolean](comparison_and_boolean.md)

# Iterators, Mapping & Functional Helpers

This page documents all user-facing iterator, mapping, and functional APIs for `var`, including STL-style iteration, dict helpers, slicing, and functional utilities, in a clear tabular format with concise examples.

---

## Mutation Detection (Python-like Safety)

All `var` containers now have **Python-like mutation detection**. Modifying a container while iterating throws `PythonicRuntimeError`.

| Feature          | Description                                                                 | Example                                            |
| ---------------- | --------------------------------------------------------------------------- | -------------------------------------------------- |
| Version tracking | Each container has `version_` counter, incremented on mutation              | `lst.get_version()`                                |
| Safe iterators   | Iterators check version on dereference                                      | `for (auto x : lst) { lst.append(1); } // THROWS!` |
| Affected methods | `append()`, `add()`, `remove()`, `clear()`, `pop()`, `extend()`, `update()` | All these increment version                        |

---

## Iteration Methods

| Method(s)                                | Description                                                                                                                       | Example                                 |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- |
| `begin()`, `end()`, `cbegin()`, `cend()` | STL-style and range-based for loop support for `var` containers (`list`, `set`, `dict`, `string`, `ordered_set`, `ordered_dict`). | `for (auto &v : list(1,2,3)) print(v);` |
| `get_version()`                          | Returns the current mutation version counter                                                                                      | `uint32_t v = lst.get_version();`       |

---

## Dict Iteration Helpers

| Method     | Description                              | Example                                        |
| ---------- | ---------------------------------------- | ---------------------------------------------- |
| `items()`  | List of `[key, value]` pairs from a dict | `for (auto &p : d.items()) print(p[0], p[1]);` |
| `keys()`   | List of keys from a dict                 | `for (auto &k : d.keys()) print(k);`           |
| `values()` | List of values from a dict               | `for (auto &v : d.values()) print(v);`         |

---

## Slicing

| Method                    | Description                                                           | Example                                                                                |
| ------------------------- | --------------------------------------------------------------------- | -------------------------------------------------------------------------------------- |
| `slice(start, end, step)` | Python-like slicing for lists and strings (supports negative indices) | `list(10,20,30,40,50).slice(1,4) // [20,30,40]`<br>`var("abcdef").slice(2,5) // "cde"` |

---

## Functional Helpers

| Function                        | Description                                     | Example                                            |
| ------------------------------- | ----------------------------------------------- | -------------------------------------------------- |
| `map(func, list)`               | Apply function to each element, return new list | `map(lambda x: x+1, list(1,2,3)) // [2,3,4]`       |
| `filter(pred, list)`            | Filter elements by predicate, return new list   | `filter(lambda x: x>1, list(1,2,3)) // [2,3]`      |
| `reduce(func, list[, initial])` | Reduce list with binary function                | `reduce(lambda a,b: a+b, list(1,2,3)) // 6`        |
| `tuple_to_list(tuple)`          | Convert C++ tuple to var list                   | `tuple_to_list(std::make_tuple(1,2,3)) // [1,2,3]` |
| `unpack(tuple)`                 | Unpack tuple to var list                        | `unpack(std::make_tuple(1,2,3)) // [1,2,3]`        |

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
    // Iteration
    var l = list(1,2,3);
    for (auto it = l.begin(); it != l.end(); ++it) print(*it);
    for (const auto &v : l) print(v);

    // Mutation detection - this will THROW!
    try {
        for (auto v : l) {
            l.append(4); // THROWS PythonicRuntimeError!
        }
    } catch (const PythonicRuntimeError& e) {
        print("Caught:", e.what());
    }

    // Safe: mutate after iteration
    for (auto v : l) { print(v); }
    l.append(4); // OK - iteration is done

    // Dict helpers
    var d = dict();
    d["a"] = 1;
    d["b"] = 2;
    for (const auto &pair : d.items()) print(pair.at(0), pair.at(1));
    for (const auto &k : d.keys()) print(k);
    for (const auto &v : d.values()) print(v);

    // Slicing
    var s = var("abcdef");
    print(s.slice(2, 5)); // "cde"
    var l2 = list(10, 20, 30, 40, 50);
    print(l2.slice(1, 4)); // [20, 30, 40]

    // Functional helpers
    auto plus1 = [](const var &x) { return x + 1; };
    print(map(plus1, list(1,2,3))); // [2,3,4]
    auto gt1 = [](const var &x) { return x > 1; };
    print(filter(gt1, list(1,2,3))); // [2,3]
    auto add = [](const var &a, const var &b) { return a + b; };
    print(reduce(add, list(1,2,3))); // 6
    print(tuple_to_list(std::make_tuple(1,2,3))); // [1,2,3]
    print(unpack(std::make_tuple(1,2,3))); // [1,2,3]
    return 0;
}
```

---

## Notes

- **Mutation detection is enabled** - modifying a container during iteration throws `PythonicRuntimeError`.
- All major iterator, mapping, and functional helpers are available for `var`.
- Dict helpers (`items`, `keys`, `values`) only work for dict types.
- Slicing supports negative indices and works for lists and strings.

## Next check

- [Graph Helpers (if using graph type)](graph_helpers.md)
