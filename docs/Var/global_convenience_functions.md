[⬅ Back to Table of Contents](var.md)

# Global / Convenience Functions

Global helpers and convenience functions for `var`.

- `inline TypeTag type_name_to_tag(const char *type_name)`
- `inline bool isinstance(const var &v, const char *type_name)` / `template <typename T> inline bool isinstance(const var &v)`
- Converters: `Bool()`, `Str()`, `Int()`, `Long()`, `LongLong()`, `UInt()`, `ULong()`, `ULongLong()`, `Double()`, `Float()`, `LongDouble()`, `String()`
- `inline var repr(const var &v)` / `inline var Str(const var &v)`
- helpers: `reversed_var`, `all_var`, `any_var`
