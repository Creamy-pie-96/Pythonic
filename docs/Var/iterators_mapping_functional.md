[⬅ Back to Table of Contents](var.md)

# Iterators, Mapping & Functional Helpers

Functional and iterator helpers for `var`.

- `template <typename Func> map`, `filter`, `reduce`
- `template <typename... Args> var list(Args&&...)`, `set`, `ordered_set`, and inline empty constructors `list()`, `set()`, `dict()`, `ordered_set()`, `ordered_dict()`
- `template <typename... Ts> var tuple_to_list(const std::tuple<Ts...>&)` and `unpack` helpers
