# `var` — Core API

What this doc covers:

- Constructors for primitives, strings, containers, and graph
- `type_tag()` / `type()` / `type_cstr()` differences
- `is<T>()` template and `is_*()` helpers
- `as_*()` (checked) vs `as_*_unchecked()` (unchecked) usage
- `get` / `get_if` semantics
- `isNone()` / `is_none()` and truthiness rules
- Memory and ownership summary (heap types, RAII)
- Copy / move / destructor behavior and examples

Usage guidance:

- Prefer `as_*()` in general code
- Use `is_*()` + `as_*_unchecked()` for hot paths

Examples:

- Creating `var` from int/string/list
- Checking type and safely extracting values
