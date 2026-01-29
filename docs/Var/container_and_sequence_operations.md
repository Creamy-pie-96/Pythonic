[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Type Introspection & Conversion](type_introspection_and_conversion.md)

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
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
    var vlist = list(1, 2, 3);
    vlist.append(4);           // [1,2,3,4]
    vlist.extend(list(5,6));   // [1,2,3,4,5,6]
    vlist.remove(2);           // [1,3,4,5,6]
    print(vlist.len());       // 5
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
    for (auto it = vdict.get<Dict>().begin(); it != vdict.get<Dict>().end(); ++it) 
    {
        print("key: ", it->first);  // key (std::string)
        print("value: ", it->second); // value (var)
    }
    for (auto [key,value] : vdict.get<Dict>())
    {
        print("key: ", key);  // key (std::string)
        print("value: ", value); // value (var)
    }
    for (auto it = vdict.begin(); it != vdict.end(); ++it) 
    {
        print(1);
        // but you can't use it->first and it->second here as the vdict is not a std::map. also you can not do auto[key,value] like iteration directly on vdict, you have to use vdict.get<Dict>() to get the underlying std::map
    }
    return 0;
}

```

---

## Functional Helpers

| Function                      | Description                                                             | Example(s)                                                        |
| ----------------------------- | ----------------------------------------------------------------------- | ----------------------------------------------------------------- |
| `reversed_var(list)`          | Return a new list with elements in reverse order                        | `reversed_var(list(1,2,3)) // [3,2,1]`                            |
| `reversed_var(str)`           | Return a new string with characters reversed                            | `reversed_var("abc") // "cba"`                                    |
| `all_var(list)`               | True if all elements are truthy                                         | `all_var(list(1,2,3)) // true`<br>`all_var(list(1,0,3)) // false` |
| `any_var(list)`               | True if any element is truthy                                           | `any_var(list(0,0,3)) // true`<br>`any_var(list(0,0,0)) // false` |
| `map(func, list)`             | Apply a function to each element, return new list                       | <pre>map(lambda x: x+1, list(1,2,3)) // [2,3,4]</pre>             |
| `filter(pred, list)`          | Filter elements by predicate, return new list                           | <pre>filter(lambda x: x>1, list(1,2,3)) // [2,3]</pre>            |
| `reduce(func, list, initial)` | Reduce list to single value with binary function, starting from initial | <pre>reduce(lambda a,b: a+b, list(1,2,3), 10) // 16</pre>         |
| `reduce(func, list)`          | Reduce list to single value with binary function                        | <pre>reduce(lambda a,b: a+b, list(1,2,3)) // 6</pre>              |

---

## Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace py;

// --- reversed_var ---
var vlist = list(1, 2, 3);
var vstr = "hello";
var revlist = reversed_var(vlist);    // [3, 2, 1]
var revstr = reversed_var(vstr);      // "olleh"

// --- all_var / any_var ---
var alltrue = all_var(list(1, 2, 3)); // true
var somefalse = all_var(list(1, 0, 3)); // false
var anytrue = any_var(list(0, 0, 3)); // true
var allfalse = any_var(list(0, 0, 0)); // false

// --- map ---
auto add1 = [](const var &x) { return x + 1; };
var mapped = map(add1, list(1, 2, 3)); // [2, 3, 4]

// --- filter ---
auto greater1 = [](const var &x) { return x > 1; };
var filtered = filter(greater1, list(1, 2, 3)); // [2, 3]

// --- reduce ---
auto sum2 = [](const var &a, const var &b) { return a + b; };
var reduced = reduce(sum2, list(1, 2, 3)); // 6
var reduced_init = reduce(sum2, list(1, 2, 3), 10); // 16

```

---
## Notes

- Use `len(v)` or `v.size()` for container length.
- Use `append`, `add`, `extend`, `update`, `remove` for mutating containers.
- Use `contains`/`has` to check membership.
- Use `slice` or call syntax for sublists.
- Use `items`, `keys`, `values` for dicts.
- Iterators allow C++-style for loops over containers.

## Next check

- [String-like Methods (on string `var`)](string_like_methods.md)