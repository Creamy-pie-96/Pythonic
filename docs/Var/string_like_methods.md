[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Containers and Sequence operations](container_and_sequence_operations.md)

# String-like Methods (on string `var`)

This page documents all user-facing string methods available on `var` objects holding strings, in a clear tabular format with concise examples.

---

## String Methods

| Method                          | Description                         | Example                                   |
| ------------------------------- | ----------------------------------- | ----------------------------------------- |
| `upper()`                       | Uppercase                           | `var("abc").upper() // "ABC"`             |
| `lower()`                       | Lowercase                           | `var("ABC").lower() // "abc"`             |
| `strip()`                       | Strip whitespace                    | `var("  hi ").strip() // "hi"`            |
| `lstrip()`                      | Strip left                          | `var("  hi").lstrip() // "hi"`            |
| `rstrip()`                      | Strip right                         | `var("hi  ").rstrip() // "hi"`            |
| `replace(old, new)`             | Replace substring                   | `var("aabb").replace("a","b") // "bbbb"`  |
| `find(substr)`                  | Find substring index                | `var("abc").find("b") // 1`               |
| `startswith(prefix)`            | Starts with prefix                  | `var("abc").startswith("a") // true`      |
| `endswith(suffix)`              | Ends with suffix                    | `var("abc").endswith("c") // true`        |
| `isdigit()`                     | Is all digits                       | `var("123").isdigit() // true`            |
| `isalpha()`                     | Is all alpha                        | `var("abc").isalpha() // true`            |
| `isalnum()`                     | Is alphanumeric                     | `var("a1b2").isalnum() // true`           |
| `isspace()`                     | Is all whitespace                   | `var("   ").isspace() // true`            |
| `capitalize()`                  | Capitalize first letter             | `var("abc").capitalize() // "Abc"`        |
| `sentence_case()`               | Capitalize first letter, rest lower | `var("hELLO").sentence_case() // "Hello"` |
| `title()`                       | Title case                          | `var("hi there").title() // "Hi There"`   |
| `count(substr)`                 | Count substring occurrences         | `var("aaba").count("a") // 3`             |
| `reverse()`                     | Reverse string                      | `var("abc").reverse() // "cba"`           |
| `split(delim = " ")`            | Split by delimiter                  | `var("a b").split() // ["a","b"]`         |
| `join(list)`                    | Join list with string as separator  | `var(",").join(list("a","b")) // "a,b"`   |
| `center(width, fillchar = " ")` | Center string                       | `var("hi").center(5,"-") // "-hi--"`      |
| `zfill(width)`                  | Pad with zeros on left              | `var("7").zfill(3) // "007"`              |

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
var s = "  hello pythonic  ";
print(s);                   // "  hello pythonic  "
print(s.strip());           // "hello pythonic"
print(s.lstrip());          // "hello pythonic  "
print(s.rstrip());          // "  hello pythonic"
print(s.upper());           // "  HELLO pythonic  "
print(s.lower());           // "  hello pythonic  "
print(s.replace(" ", "_")); // "__hello_pythonic__"
print(s.find("pythonic"));    // 8
print(s.startswith("  h")); // true
print(s.endswith("d  "));   // true
print(var("123").isdigit()); // true
print(var("abc").isalpha()); // true
print(var("a1b2").isalnum()); // true
print(var("   ").isspace()); // true
print(var("abc").capitalize()); // "Abc"
print(var("hELLO.i aM PrItHu.").sentence_case()); // "Hello. I am Prithu."
print(var("hi there").title()); // "Hi There"
print(var("aaba").count("a")); // 3
print(var("abc").reverse()); // "cba"
print(var("a b c").split()); // ["a","b","c"]
print(var(",").join(list("a","b","c"))); // "a,b,c"
print(var("hi").center(5,"-")); // "-hi--"
print(var("7").zfill(3)); // "007"
    return 0;
}

```

---

## Notes

- All methods return a new `var` (string or list) and do not modify the original.
- Use `split`/`join` for conversion between strings and lists.
- Most methods match Python string semantics.

## Next check

- [Numeric / Arithmetic](numeric_arithmetic.md)