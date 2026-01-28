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

---

## Conversion Methods (Member)

| API              | Description                   | Example            |
| ---------------- | ----------------------------- | ------------------ |
| `toInt()`        | Convert to int                | `v.toInt()`        |
| `toUInt()`       | Convert to unsigned int       | `v.toUInt()`       |
| `toLong()`       | Convert to long               | `v.toLong()`       |
| `toULong()`      | Convert to unsigned long      | `v.toULong()`      |
| `toLongLong()`   | Convert to long long          | `v.toLongLong()`   |
| `toULongLong()`  | Convert to unsigned long long | `v.toULongLong()`  |
| `toFloat()`      | Convert to float              | `v.toFloat()`      |
| `toDouble()`     | Convert to double             | `v.toDouble()`     |
| `toLongDouble()` | Convert to long double        | `v.toLongDouble()` |
| `toString()`     | Convert to string             | `v.toString()`     |

---

## Built-in Conversion Helpers (Free Functions)

| API               | Description                     | Example           |
| ----------------- | ------------------------------- | ----------------- |
| `Bool(v)`         | Convert to bool                 | `Bool(v)`         |
| `Str(v)`          | Convert to string               | `Str(v)`          |
| `String(v)`       | Convert to string               | `String(v)`       |
| `Int(v)`          | Convert to int                  | `Int(v)`          |
| `Long(v)`         | Convert to long                 | `Long(v)`         |
| `LongLong(v)`     | Convert to long long            | `LongLong(v)`     |
| `UInt(v)`         | Convert to unsigned int         | `UInt(v)`         |
| `ULong(v)`        | Convert to unsigned long        | `ULong(v)`        |
| `ULongLong(v)`    | Convert to unsigned long long   | `ULongLong(v)`    |
| `Double(v)`       | Convert to double               | `Double(v)`       |
| `Float(v)`        | Convert to float                | `Float(v)`        |
| `LongDouble(v)`   | Convert to long double          | `LongDouble(v)`   |
| `list(v)`         | Convert to List                 | `list(v)`         |
| `dict(v)`         | Convert to Dict                 | `dict(v)`         |
| `set(v)`          | Convert to Set                  | `set(v)`          |
| `ordered_set(v)`  | Convert to OrderedSet           | `ordered_set(v)`  |
| `ordered_dict(v)` | Convert to OrderedDict          | `ordered_dict(v)` |
| `graph(v)`        | Convert to Graph                | `graph(v)`        |
| `load_graph(v)`   | Load/convert to Graph from file | `load_graph(v)`   |

---

## Examples

```cpp
using namespace pythonic::vars;

var v = 42;
v.is<int>();           // true
isinstance<std::string>(v); // false
isinstance(v, "int"); // true
v.type();              // "int"
v.type_tag() == TypeTag::INT;

var s = var("hello");
s.is_string();         // true
s.as_string();         // "hello"

var l = list(1, 2, 3);
l.is_list();           // true
l.as_list().size();    // 3

var d = dict({{"a", 1}, {"b", 2}});
d.is_dict();           // true
d.as_dict()["a"]      // 1

var f = var(3.14);
f.toInt();             // 3
Double(f);             // 3.14

// Safe pointer access
if (auto *p = l.var_get_if<List>()) {
	// use *p
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