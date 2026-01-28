[⬅ Back to Table of Contents](index.md)

# Functional Programming

This page documents all user-facing functional programming helpers in Pythonic, including comprehensions, map/filter/reduce, lambda helpers, and utility functions, with concise examples.

---

## Map, Filter, Reduce

| Function                | Description                                 | Example                        |
| ----------------------- | ------------------------------------------- | ------------------------------ |
| `map(func, iterable)`   | Apply function to each element, return list | `map(lambda_(x, x*2), list(1,2,3))` |
| `map_indexed(func, iterable)` | Apply function to each element with index | `map_indexed(lambda2_(i, x, i+x), list(1,2,3))` |
| `filter(func, iterable)`| Keep elements where func(x) is true         | `filter(lambda_(x, x>1), list(1,2,3))` |
| `reduce(func, iterable)`| Reduce elements using binary function       | `reduce(lambda2_(x, y, x+y), list(1,2,3))` |
| `reduce(func, iterable, initial)` | Reduce with initializer                | `reduce(lambda2_(x, y, x+y), list(1,2,3), 10)` |

---

## Comprehensions

| Function                | Description                                 | Example                        |
| ----------------------- | ------------------------------------------- | ------------------------------ |
| `list_comp(expr, iterable)` | List comprehension [expr(x) for x in iterable] | `list_comp(lambda_(x, x*2), list(1,2,3))` |
| `list_comp(expr, iterable, cond)` | List comp with condition [expr(x) for x in iterable if cond(x)] | `list_comp(lambda_(x, x*2), list(1,2,3), lambda_(x, x>1))` |
| `set_comp(expr, iterable)` | Set comprehension {expr(x) for x in iterable} | `set_comp(lambda_(x, x*2), list(1,2,3))` |
| `set_comp(expr, iterable, cond)` | Set comp with condition                | `set_comp(lambda_(x, x*2), list(1,2,3), lambda_(x, x>1))` |
| `dict_comp(key_expr, val_expr, iterable)` | Dict comp {key(x): val(x) for x in iterable} | `dict_comp(lambda_(x, x), lambda_(x, x*2), list(1,2,3))` |
| `dict_comp(key_expr, val_expr, iterable, cond)` | Dict comp with condition | `dict_comp(lambda_(x, x), lambda_(x, x*2), list(1,2,3), lambda_(x, x>1))` |

---

## Sorting & Utility Functions

| Function                | Description                                 | Example                        |
| ----------------------- | ------------------------------------------- | ------------------------------ |
| `sorted(iterable, reverse=false)` | Sort elements, optionally reversed   | `sorted(list(3,1,2))`<br>`sorted(list(3,1,2), true)` |
| `sorted(iterable, key, reverse=false)` | Sort by key function               | `sorted(list(3,1,2), lambda_(x, -x))` |
| `flatten(nested)`       | Flatten one level of nested lists           | `flatten(list(list(1,2), list(3,4)))` |
| `unique(iterable)`      | Unique elements, preserve order             | `unique(list(1,2,2,3))`        |
| `group_by(key_func, iterable)` | Group elements by key function        | `group_by(lambda_(x, x%2), list(1,2,3,4))` |
| `slice(container, start, end, step=1)` | Python-like slicing                | `slice(list(1,2,3,4,5), 1, 4, 2)` |
| `join(separator, iterable)`     | Join elements into string             | `join(",", list("a","b","c"))` |
| `split(str, separator=" ")`   | Split string into list                | `split("a b c")`<br>`split("a,b,c", ",")` |
| `find_if(iterable, pred)`       | Find first element matching predicate | `find_if(list(1,2,3), lambda_(x, x>1))` |
| `index(iterable, value)`        | Find index of value                   | `index(list(1,2,3), 2)`         |
| `count_if(iterable, pred)`      | Count elements matching predicate     | `count_if(list(1,2,3), lambda_(x, x>1))` |
| `count(iterable, value)`        | Count occurrences of value            | `count(list(1,2,2,3), 2)`       |
| `take(n, iterable)`             | Take first n elements                 | `take(2, list(1,2,3))`          |
| `drop(n, iterable)`             | Drop first n elements                 | `drop(2, list(1,2,3))`          |
| `take_while(pred, iterable)`    | Take elements while predicate true    | `take_while(lambda_(x, x<3), list(1,2,3,4))` |
| `drop_while(pred, iterable)`    | Drop elements while predicate true    | `drop_while(lambda_(x, x<3), list(1,2,3,4))` |
| `product(iterable, start=1)`    | Product of all elements               | `product(list(1,2,3))`          |

---

## Lambda Helpers

| Macro                     | Description                                 | Example                        |
| ------------------------- | ------------------------------------------- | ------------------------------ |
| `lambda_(param, body)`    | Create lambda with 1 param                  | `lambda_(x, x*2)`              |
| `lambda2_(p1, p2, body)`  | Create lambda with 2 params                 | `lambda2_(x, y, x+y)`          |
| `lambda3_(p1, p2, p3, body)` | Create lambda with 3 params               | `lambda3_(x, y, z, x+y+z)`     |
| `clambda_(param, body)`   | Lambda with capture                        | `clambda_(x, x+external)`      |
| `clambda2_(p1, p2, body)` | Lambda with 2 params and capture            | `clambda2_(x, y, x+y+external)`|
| `clambda3_(p1, p2, p3, body)` | Lambda with 3 params and capture         | `clambda3_(x, y, z, x+y+z+external)` |

---

## Function Utilities

| Function                | Description                                 | Example                        |
| ----------------------- | ------------------------------------------- | ------------------------------ |
| `apply(func, args)`     | Apply function to arguments from list/tuple | `apply(lambda2_(x, y, x+y), list(1,2))` |
| `partial(func, ...args)`| Partial function application                | `auto add5 = partial(lambda2_(x, y, x+y), 5); add5(3);` |
| `compose(f, g)`         | Function composition f(g(x))                | `auto f = compose(lambda_(x, x+1), lambda_(x, x*2)); f(3);` |

---

## Examples

```cpp
#include "pythonic/pythonicFunction.hpp"
using namespace pythonic::func;

// --- Map, Filter, Reduce ---
print(map(lambda_(x, x*2), list(1,2,3))); // [2,4,6]
print(filter(lambda_(x, x>1), list(1,2,3))); // [2,3]
print(reduce(lambda2_(x, y, x+y), list(1,2,3))); // 6

// --- Comprehensions ---
print(list_comp(lambda_(x, x*2), list(1,2,3))); // [2,4,6]
print(set_comp(lambda_(x, x*2), list(1,2,3))); // {2,4,6}
print(dict_comp(lambda_(x, x), lambda_(x, x*2), list(1,2,3))); // {1:2,2:4,3:6}

// --- Sorting & Utility ---
print(sorted(list(3,1,2))); // [1,2,3]
print(flatten(list(list(1,2), list(3,4)))); // [1,2,3,4]
print(unique(list(1,2,2,3))); // [1,2,3]
print(group_by(lambda_(x, x%2), list(1,2,3,4))); // {0:[2,4],1:[1,3]}
print(slice(list(1,2,3,4,5), 1, 4, 2)); // [2,4]
print(join(",", list("a","b","c"))); // "a,b,c"
print(split("a b c")); // ["a","b","c"]

// --- Lambda Helpers ---
auto square = lambda_(x, x*x);
auto add = lambda2_(x, y, x+y);
print(square(5)); // 25
print(add(2,3)); // 5

// --- Function Utilities ---
print(apply(lambda2_(x, y, x+y), list(1,2))); // 3
auto add5 = partial(lambda2_(x, y, x+y), 5);
print(add5(3)); // 8
auto f = compose(lambda_(x, x+1), lambda_(x, x*2));
print(f(3)); // 7

```

---

## Notes

- All functional helpers work with standard containers, Pythonic containers, and C++20 ranges.
- Lambda macros simplify function creation and support captures.
- Comprehensions provide Python-like syntax for list/set/dict creation.
- Utility functions like flatten, unique, group_by, and slice make data manipulation easy.
- Use partial and compose for advanced functional patterns.

---

## See Also

- [Iteration Helpers](iteration.md)
- [Math Functions](math.md)
- [Containers](containers.md)
