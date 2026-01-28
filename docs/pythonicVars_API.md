# Pythonic `var` — User-facing API

This document lists the user-facing functions and methods extracted from `include/pythonic/pythonicVars.hpp`. Use this as the basis for a README section describing the `var` dynamic type.

## Type Introspection & Conversion

- `bool isNone()`
- `template <typename T> bool is()` — type predicate
- `bool isNumeric()` / `bool isIntegral()`
- `is_list()`, `is_dict()`, `is_set()`, `is_string()`, `is_int()`, `is_double()`, `is_float()`, `is_bool()`, `is_none()`, `is_ordered_dict()`, `is_ordered_set()`, `is_long()`, `is_long_long()`, `is_long_double()`, `is_uint()`, `is_ulong()`, `is_ulong_long()`, `is_graph()` — convenient type checks
- `std::string type()` / `const char *type_cstr()` / `TypeTag type_tag()`
- `int toInt()` / `unsigned int toUInt()` / `long toLong()` / `unsigned long toULong()` / `long long toLongLong()` / `unsigned long long toULongLong()` / `float toFloat()` / `double toDouble()` / `long double toLongDouble()` / `std::string toString()`
- `template <typename T> T &get()` / `template <typename T> const T &get() const` — typed accessors
- `as_*()` family (e.g., `as_int()`, `as_string()`, `as_list()` etc.) for converting or retrieving contained data

## Container & Sequence Operations

- `len()` / `size()` / `var len(const var &v)` (helpers)
- `void append(const var &v)` and templated `append(T&&)`
- `void add(const var &v)` and templated `add(T&&)`
- `void extend(const var &other)`
- `void update(const var &other)`
- `void remove(const var &v)`
- `var contains(const var &v)` / `var has(const var &v)`
- `iterator begin()` / `iterator end()` / `const_iterator ...` / `cbegin()` / `cend()`
- `var items()` / `var keys()` / `var values()`
- `var slice(long long start=0, long long end=LLONG_MAX, long long step=1)` and `operator()(...)` overload

## String-like Methods (on string `var`)

- `var upper()` / `var lower()`
- `var strip()` / `var lstrip()` / `var rstrip()`
- `var replace(const var &old_str, const var &new_str)`
- `var find(const var &substr)` / `var startswith(const var &prefix)` / `var endswith(const var &suffix)`
- `var isdigit()` / `var isalpha()` / `var isalnum()` / `var isspace()`
- `var capitalize()` / `var sentence_case()` / `var title()`
- `var count(const var &substr)` / `var reverse()` / `var split(const var &delim = var(" "))` / `var join(const var &lst)` / `var center(int width, const var &fillchar = var(" "))` / `var zfill(int width)`

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
