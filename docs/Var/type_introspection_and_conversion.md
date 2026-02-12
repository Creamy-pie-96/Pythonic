[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Construction & Lifetime](construction_and_lifetime.md)

# Type Introspection & Conversion

This page documents all type-checking, type-introspection, and conversion APIs for the `var` class and helpers.

---

## Type Tag & Type Name APIs

| API           | Description                          | Example                         |
| ------------- | ------------------------------------ | ------------------------------- |
| `type_tag()`  | Returns the internal type tag enum.  | `v.type_tag() == TypeTag::LIST` |
| `type()`      | Returns the type name as a string.   | `v.type() // "list"`            |
| `type_cstr()` | Returns the type name as a C string. | `v.type_cstr() // "list"`       |

---

## Type Checking APIs

### Template-based

| API                | Description                     | Example                       |
| ------------------ | ------------------------------- | ----------------------------- |
| `is<T>()`          | Checks if the var holds type T. | `v.is<int>()`, `v.is<List>()` |
| `isinstance<T>(v)` | Free function, same as above.   | `isinstance<std::string>(v)`  |

### String-based

| API                                  | Description                 | Example                             |
| ------------------------------------ | --------------------------- | ----------------------------------- |
| `isinstance(v, "int")`               | Checks by type name string. | `isinstance(v, "list")`             |
| `isinstance(v, std::string("dict"))` | Checks by std::string.      | `isinstance(v, std::string("set"))` |

### Fast O(1) Type Checks

| API                 | Description              |
| ------------------- | ------------------------ |
| `is_list()`         | Is a list                |
| `is_dict()`         | Is a dict                |
| `is_set()`          | Is a set                 |
| `is_string()`       | Is a string              |
| `is_int()`          | Is an int                |
| `is_double()`       | Is a double              |
| `is_float()`        | Is a float               |
| `is_bool()`         | Is a bool                |
| `is_none()`         | Is None                  |
| `is_ordered_dict()` | Is an ordered dict       |
| `is_ordered_set()`  | Is an ordered set        |
| `is_long()`         | Is a long                |
| `is_long_long()`    | Is a long long           |
| `is_long_double()`  | Is a long double         |
| `is_uint()`         | Is an unsigned int       |
| `is_ulong()`        | Is an unsigned long      |
| `is_ulong_long()`   | Is an unsigned long long |
| `is_graph()`        | Is a graph               |
| `is_any_integral()` | Is any integral type     |
| `is_any_floating()` | Is any floating type     |
| `is_any_numeric()`  | Is any numeric type      |

---

## Value Accessors

| API               | Description                                        | Example                                   |
| ----------------- | -------------------------------------------------- | ----------------------------------------- |
| `get<T>()`        | Returns value as T (throws if wrong type)          | `v.get<int>()`                            |
| `var_get_if<T>()` | Returns pointer to T if type matches, else nullptr | `if (auto *p = v.var_get_if<List>()) ...` |

### Internal Accessors: var_get Methods

These methods provide direct access to the underlying value or container inside a `var`. They are intended for advanced use and require the user to know the exact type held by the `var` (no type checking is performed).

| Method               | Description                                                                  | Example                                          |
| -------------------- | ---------------------------------------------------------------------------- | ------------------------------------------------ |
| `var_get<T>()`       | Returns a reference to the value of type `T` stored in the `var`.            | `auto &lst = vlist.var_get<List>();`             |
| `var_get<const T>()` | Returns a const reference to the value of type `T` stored in the `var`.      | `const auto &lst = vlist.var_get<const List>();` |
| `var_get_if<T>()`    | Returns pointer to value if type matches, else nullptr (safe checked access) | `if (auto *p = v.var_get_if<Dict>()) ...`        |

**Note:**

- `var_get<T>()` is unsafe if the type does not match; it is for expert/advanced use when you are certain of the type.
- For safe access, prefer `get<T>()` (throws on wrong type) or `var_get_if<T>()` (returns nullptr if wrong type).

---

## Conversion Methods (Member vs Free Function)

There are two main ways to convert types:

- **Member methods** (e.g., `toInt()`): Return the value as a native C++ type (e.g., `int`).
- **Free functions** (e.g., `Int(v)`): Return a new `var` holding the converted value.

**Key difference:**

- `v.toInt()` returns an `int` (native type), does not wrap in `var`.
- `Int(v)` returns a `var` holding an int value.

| API              | Description                                                       | Example                                  |
| ---------------- | ----------------------------------------------------------------- | ---------------------------------------- |
| `toInt()`        | Returns value as native int (does not modify in-place, not a var) | `int x = v.toInt()`                      |
| `toUInt()`       | Returns value as native unsigned int                              | `unsigned int x = v.toUInt()`            |
| `toLong()`       | Returns value as native long                                      | `long x = v.toLong()`                    |
| `toULong()`      | Returns value as native unsigned long                             | `unsigned long x = v.toULong()`          |
| `toLongLong()`   | Returns value as native long long                                 | `long long x = v.toLongLong()`           |
| `toULongLong()`  | Returns value as native unsigned long long                        | `unsigned long long x = v.toULongLong()` |
| `toFloat()`      | Returns value as native float                                     | `float x = v.toFloat()`                  |
| `toDouble()`     | Returns value as native double                                    | `double x = v.toDouble()`                |
| `toLongDouble()` | Returns value as native long double                               | `long double x = v.toLongDouble()`       |
| `toString()`     | Returns value as native std::string                               | `std::string x = v.toString()`           |

## Built-in Conversion Helpers (Free Functions)

| API            | Description                                   | Example                |
| -------------- | --------------------------------------------- | ---------------------- |
| `Int(v)`       | Returns a `var` holding an int                | `var x = Int(v)`       |
| `Long(v)`      | Returns a `var` holding a long                | `var x = Long(v)`      |
| `LongLong(v)`  | Returns a `var` holding a long long           | `var x = LongLong(v)`  |
| `UInt(v)`      | Returns a `var` holding an unsigned int       | `var x = UInt(v)`      |
| `ULong(v)`     | Returns a `var` holding an unsigned long      | `var x = ULong(v)`     |
| `ULongLong(v)` | Returns a `var` holding an unsigned long long | `var x = ULongLong(v)` |
| ...            | ...                                           | ...                    |

**Summary Table:**

| Usage       | Returns     | Example              |
| ----------- | ----------- | -------------------- |
| `v.toInt()` | `int`       | `int x = v.toInt();` |
| `Int(v)`    | `var` (int) | `var x = Int(v);`    |

This distinction applies to all similar conversions (Long, Double, etc.).

---

## Built-in Conversion Helpers (Free Functions)

| API               | Description                                                                                       | Example                                     |
| ----------------- | ------------------------------------------------------------------------------------------------- | ------------------------------------------- |
| `Bool(v)`         | Convert to bool                                                                                   | `Bool(v)`                                   |
| `Str(v)`          | Convert to string                                                                                 | `Str(v)`                                    |
| `String(v)`       | Convert to string                                                                                 | `String(v)`                                 |
| `Int(v)`          | Convert to int                                                                                    | `Int(v)`                                    |
| `Long(v)`         | Convert to long                                                                                   | `Long(v)`                                   |
| `LongLong(v)`     | Convert to long long                                                                              | `LongLong(v)`                               |
| `UInt(v)`         | Convert to unsigned int                                                                           | `UInt(v)`                                   |
| `ULong(v)`        | Convert to unsigned long                                                                          | `ULong(v)`                                  |
| `ULongLong(v)`    | Convert to unsigned long long                                                                     | `ULongLong(v)`                              |
| `Double(v)`       | Convert to double                                                                                 | `Double(v)`                                 |
| `Float(v)`        | Convert to float                                                                                  | `Float(v)`                                  |
| `LongDouble(v)`   | Convert to long double                                                                            | `LongDouble(v)`                             |
| `AutoNumeric(v)`  | Convert to the most suitable numeric type (int, long long, double, or long double) automatically. | `AutoNumeric(var("123.45")) // var(double)` |
| `list(v)`         | Convert to List                                                                                   | `list(v)`                                   |
| `dict(v)`         | Convert to Dict                                                                                   | `dict(v)`                                   |
| `set(v)`          | Convert to Set                                                                                    | `set(v)`                                    |
| `ordered_set(v)`  | Convert to OrderedSet                                                                             | `ordered_set(v)`                            |
| `ordered_dict(v)` | Convert to OrderedDict                                                                            | `ordered_dict(v)`                           |
| `graph(v)`        | Convert to Graph                                                                                  | `graph(v)`                                  |
| `load_graph(v)`   | Load/convert to Graph from file                                                                   | `load_graph(v)`                             |

---

## Examples

```cpp
#include <pythonic/pythonic.hpp>
using namespace py;

int main()
{
var v = 42;
print(v.is<int>());           // true
print(isinstance<std::string>(v)); // false
print(isinstance(v, "int")); // true
print(v.type());              // "int"
print(v.type_tag() == TypeTag::INT); // true

var s = var("hello");
print(s.is_string());         // true
print(s);         // "hello"

var l = list(1, 2, 3);
print(l.is_list());           // true
print(l.len());    // 3

var d = dict({{"a", 1}, {"b", 2}});
d["a"] = 42;           // set value
var x = d["a"];        // get value (x is var)
print(x,x.type()); // 42 int

var f = 3.1415;
var y = Double(x);
print(y,y.type()); // 3.0 double


// Safe pointer access
if (auto *p = l.var_get_if<List>()) {
	// use *p
}

return 0;
}
```

---

## Notes

- Prefer `is<T>()` or `isinstance<T>(v)` for compile-time type checks.
- Use `is_*()` helpers for fast runtime checks.
- Use `get<T>()` or `var_get_if<T>()` for value access. These are the only user-facing accessors.
- Use `to*()` or free functions for conversion.
- All conversions throw on invalid types.

## Next check

- [Container & Sequence Operations](container_and_sequence_operations.md)
