# Pythonic `var` — User-facing API

This document lists the user-facing functions and methods extracted from `include/pythonic/pythonicVars.hpp`. Use this as the basis for a README section describing the `var` dynamic type.

## Numeric / Arithmetic

- arithmetic operators: `operator+`, `operator-`, `operator*`, `operator/`, `operator%` and compound `operator+=`, `operator-=`, `operator*=`, `operator/=`, `operator%=` (exposed as member overloads)
- numeric helpers: `abs`, `min`, `max`, `sum` (overloads available)

## Comparison & Boolean

- comparison operators (`operator==`, `operator!=`, `operator<`, `operator>`, ...) implemented
- `operator bool()` and explicit numeric conversions (`explicit operator int()`, `explicit operator double()`, etc.)

## Iterators, Mapping & Functional Helpers

- `template <typename Func> map`, `filter`, `reduce` helpers
- `template <typename... Args> var list(Args&&...)`, `set`, `ordered_set`, and inline empty constructors `list()`, `set()`, `dict()`, `ordered_set()`, `ordered_dict()`
- `template <typename... Ts> var tuple_to_list(const std::tuple<Ts...>&)` and `unpack` helpers

## Graph Helpers (if using graph type)

- `size_t node_count()` / `size_t edge_count()` / `bool is_connected()` / `bool has_cycle()`
- `bool has_edge(size_t from, size_t to)` / `size_t out_degree(size_t node)` / `size_t in_degree(size_t node)`
- `size_t add_node()` / `size_t add_node(const var &data)` / `void remove_node(size_t node)`
- `bool remove_edge(size_t from, size_t to, bool remove_reverse = true)`
- `void set_edge_weight(size_t from, size_t to, double weight)` / `void set_node_data(size_t node, const var &data)` / `var &get_node_data(size_t node)`

## Global / Convenience Functions

- `inline TypeTag type_name_to_tag(const char *type_name)`
- `inline bool isinstance(const var &v, const char *type_name)` / `template <typename T> inline bool isinstance(const var &v)`
- `inline var repr(const var &v)` / `inline var repr` / `inline var Str(const var &v)`
- helpers: `reversed_var`, `all_var`, `any_var`

## Which functions are user-facing vs internal

- User-facing (include in README): constructors, `is_*` checks, `as_*`/`to*` conversions, container operations (`append`, `add`, `extend`, `update`, `remove`, `items`, `keys`, `values`, iterators), string methods (`upper`, `lower`, `split`, `join`, etc.), numeric ops and operators, graph helpers (if using graph), global helpers (`list`, `dict`, `Bool`, `Int`, `Str`, `isinstance`, `len`, `type`).

- Internal / Not recommended for general users (omit from top-level README): functions with `_unchecked` suffix, low-level heap helpers (`destroy_heap_data`, `copy_heap_data`, `move_heap_data`), functions that appear to be implementation details (e.g., `getTypeRank`, `isUnsignedTag`, `isSignedIntegerTag`, `isFloatingTag`) unless you want a detailed developer API section.

---

If you want, I can now:

- produce a compact README-ready section with one-line descriptions for each user-facing function (short docstrings), or
- group these by class/namespace and produce separate Markdown files per category.

Which format do you prefer for the README content? (compact list, grouped with short descriptions, or full reference with signatures and brief notes)
