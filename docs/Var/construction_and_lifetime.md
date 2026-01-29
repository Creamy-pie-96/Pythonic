[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var](var.md)
[⬅ Back to Data types](dtypes.md)

# Construction & Lifetime

This page documents all ways to construct and manage the lifetime of a `var` object, using a clear tabular format with concise examples.

---

## Constructors

| Constructor               | Description              | Example                             |
| ------------------------- | ------------------------ | ----------------------------------- |
| `var()`                   | Default (int 0)          | `var a; // 0`                       |
| `var(NoneType{})`         | Pythonic None            | `var n(NoneType{}); // None`        |
| `var(bool)`               | Boolean                  | `var b(true); // true`              |
| `var(int)`                | Integer                  | `var i = 42; // 42`                 |
| `var(double)`             | Floating-point           | `var d = 3.14; // 3.14`             |
| `var(const char*)`        | String                   | `var s("hello"); // "hello"`        |
| `var(const std::string&)` | String                   | `var s(std::string("hi")); // "hi"` |
| `var(List/Set/Dict/...)`  | Container types          | `var vlist(list(1,2,3));`           |
| `graph(n)`                | Graph (VarGraph wrapper) | `var vg = graph(5);`                |

---

## Converter Helpers

| Helper                | Description                   | Example                   |
| --------------------- | ----------------------------- | ------------------------- |
| `Bool(v)`             | Convert to bool               | `Bool("abc") // true`     |
| `Str(v)`              | Convert to string             | `Str(123)`                |
| `String(v)`           | Convert to string             | `String(123)`             |
| `Int(v)`              | Convert to int                | `Int("42")`               |
| `Long(v)`             | Convert to long               | `Long("42")`              |
| `LongLong(v)`         | Convert to long long          | `LongLong("42")`          |
| `UInt(v)`             | Convert to unsigned int       | `UInt("42")`              |
| `ULong(v)`            | Convert to unsigned long      | `ULong("42")`             |
| `ULongLong(v)`        | Convert to unsigned long long | `ULongLong("42")`         |
| `Double(v)`           | Convert to double             | `Double("3.14")`          |
| `Float(v)`            | Convert to float              | `Float("3.14")`           |
| `LongDouble(v)`       | Convert to long double        | `LongDouble("3.14")`      |
| `list(...)`           | Create List                   | `list(1,2,3)`             |
| `dict({...})`         | Create Dict                   | `dict({{"a",1}})`         |
| `set(...)`            | Create Set                    | `set(1,2,3)`              |
| `ordered_set(...)`    | Create OrderedSet             | `ordered_set(3,1,2)`      |
| `ordered_dict({...})` | Create OrderedDict            | `ordered_dict({{"k",1}})` |
| `graph(n)`            | Create Graph                  | `graph(5)`                |
| `load_graph(path)`    | Load Graph from file          | `load_graph("file")`      |

---

## Input

| Function           | Description                                                     | Example                  |
| ------------------ | --------------------------------------------------------------- | ------------------------ |
| `input(prompt="")` | Read a line from standard input, optionally displaying a prompt | `input("Enter value: ")` |

---

## Copy / Move Semantics

| Operation        | Description                   | Example                 |
| ---------------- | ----------------------------- | ----------------------- |
| Copy constructor | Deep copy for heap types      | `var b = a;`            |
| Move constructor | Steals heap data if possible  | `var c = std::move(a);` |
| Destructor       | Frees heap-allocated contents | `~var()` (automatic)    |

---

## Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace py;

// --- Direct Constructors ---
var a;                        // default -> 0
var n(NoneType{});            // None
var b(true);                  // bool
var i = 42;                   // int
var ui = static_cast<unsigned int>(123u); // unsigned int
var l = 123456789L;           // long
var ul = static_cast<unsigned long>(123456789UL); // unsigned long
var ll = 123456789012345LL;   // long long
var ull = static_cast<unsigned long long>(123456789012345ULL); // unsigned long long
var f = 3.14f;                // float
var d = 3.1415;               // double
var ld = (long double)1.23456789L; // long double
var s1("hello");             // const char*
var s2(std::string("world")); // std::string

// --- Container Constructors ---
List lst = { var(1), var(2), var(3) };
var vlist(lst);
Set st = { var(1), var(2), var(3) };
var vset(st);
Dict dct = {{"a", var(1)}, {"b", var(2)}};
var vdict(dct);
OrderedSet os = { var(3), var(1), var(2) };
var vos(os);
OrderedDict od = { {"k1", var(1)}, {"k2", var(2)} };
var vod(od);

// --- Graph Constructor ---
var vg = graph(5); // recommended
vg.add_edge(0, 1);
vg.save_graph("mygraph.txt"); // save the graph to a file

// --- Converter Helpers ---
var vstr = "123";
var vbool = Bool(vstr);         // true
var vint = Int(vstr);           // 123
var vlong = Long(vstr);         // 123L
var vll = LongLong(vstr);       // 123LL
var vuint = UInt(vstr);         // 123u
var vul = ULong(vstr);          // 123ul
var vull = ULongLong(vstr);     // 123ull
var vfloat = Float(vstr);       // 123.0f
var vdouble = Double(vstr);     // 123.0
var vld = LongDouble(vstr);     // 123.0L
var vstr2 = Str(vint);          // "123"
var vstr3 = String(vint);       // "123"

// --- Factory Helpers ---
var flist = list(1, 2, 3);
var fset = set(1, 2, 3);
var fodset = ordered_set(3, 1, 2);
var fdict = dict({{"a", 1}, {"b", 2}});
var fodict = ordered_dict({{"k1", 1}, {"k2", 2}});
var fgraph = graph(5);
var loaded = load_graph("mygraph.txt"); // load the graph from the file
//auto loaded = load_graph("/path/to/graph.file");


// --- input ---
 var user_input = input("Enter value: ");

// --- Copy / Move / Destructor ---
var copy = flist;               // copy
var moved = std::move(flist);   // move

// --- Output ---
print(a, n, b, i, ui, l, ul, ll, ull, f, d, ld, s1, s2);
print(vlist); print(vset); print(vdict); print(vos); print(vod); print(vg);
print(vbool, vint, vlong, vll, vuint, vul, vull, vfloat, vdouble, vld, vstr2, vstr3);
print(flist, fset, fodset, fdict, fodict, fgraph, loaded);
print(copy, moved);
```

---

## Notes

- Use constructors for direct value storage.
- Use converter helpers for explicit conversion or factory creation.
- Prefer move for large containers to avoid unnecessary copies.
- All containers and graphs are stored as heap-backed types inside `var`.

## Next check

- [Type Introspection & Conversion](type_introspection_and_conversion.md)
