[⬅ Back to Table of Contents](var.md)

# Container & Sequence Operations

This page documents all user-facing container and sequence APIs for `var`, in a clear tabular format with concise examples.

---

## Core Container Operations

| API / Method                    | Description                            | Example                                                      |
| ------------------------------- | -------------------------------------- | ------------------------------------------------------------ |
| `len(v)` / `size(v)`            | Get length/size of container           | `len(list(1,2,3)) // 3`                                      |
| `v.size()`                      | Member: get size                       | `vlist.size()`                                               |
| `append(const var &v)`          | Append element to list                 | `vlist.append(4)`                                            |
| `add(const var &v)`             | Add element to set/ordered_set         | `vset.add(5)`                                                |
| `extend(const var &other)`      | Extend list/set with another container | `vlist.extend(list(4,5))`                                    |
| `update(const var &other)`      | Update set/dict with another container | `vset.update(set(6,7))`                                      |
| `remove(const var &v)`          | Remove element from container          | `vlist.remove(2)`                                            |
| `contains(const var &v)`        | Check if element is in container       | `vlist.contains(2)`                                          |
| `has(const var &v)`             | Alias for contains                     | `vset.has(3)`                                                |
| `clear()`                       | Remove all elements                    | `vlist.clear()`                                              |
| `empty()`                       | Check if container is empty            | `vlist.empty()`                                              |
| `front()` / `back()`            | Get first/last element (list)          | `vlist.front()`, `vlist.back()`                              |
| `pop()`                         | Remove and return last element (list)  | `vlist.pop()`                                                |
| `at(index)`                     | Access element by index (list)         | `vlist.at(1)`                                                |
| `operator[](index)`             | Indexing (list/dict)                   | `vlist[1]`, `vdict["a"]`                                     |
| `items()`                       | Dict: get key-value pairs              | `vdict.items()`                                              |
| `keys()`                        | Dict: get keys                         | `vdict.keys()`                                               |
| `values()`                      | Dict: get values                       | `vdict.values()`                                             |
| `slice(start, end, step)`       | List: get sublist                      | `vlist.slice(1,3)`                                           |
| `operator()(...)`               | List: slicing via call syntax          | `vlist(1,3)`                                                 |
| `begin()/end()/cbegin()/cend()` | Iterators for C++ loops                | `for (auto it = vlist.begin(); it != vlist.end(); ++it) ...` |

---

## Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace py;

var vlist = list(1, 2, 3);
vlist.append(4);           // [1,2,3,4]
vlist.extend(list(5,6));   // [1,2,3,4,5,6]
vlist.remove(2);           // [1,3,4,5,6]
print(vlist.size());       // 5
print(vlist.front());      // 1
print(vlist.back());       // 6
print(vlist.at(2));        // 4
print(vlist[1]);           // 3
print(vlist.slice(1,4));   // [3,4,5]
print(vlist(1,4));         // [3,4,5]
print(vlist.empty());      // false
vlist.clear();
print(vlist.empty());      // true

var vset = set(1,2,3);
vset.add(4);
vset.update(set(5,6));
print(vset.contains(3));    // true
print(vset.has(5));        // true
vset.remove(1);

var vdict = dict({{"a", 1}, {"b", 2}});
print(vdict["a"]);        // 1
print(vdict.keys());       // ["a", "b"]
print(vdict.values());     // [1,2]
print(vdict.items());      // [("a",1), ("b",2)]

// Iteration
for (auto it = vdict.begin(); it != vdict.end(); ++it) {
    // ...
}
```

---

## Notes

- Use `len(v)` or `v.size()` for container length.
- Use `append`, `add`, `extend`, `update`, `remove` for mutating containers.
- Use `contains`/`has` to check membership.
- Use `slice` or call syntax for sublists.
- Use `items`, `keys`, `values` for dicts.
- Iterators allow C++-style for loops over containers.
