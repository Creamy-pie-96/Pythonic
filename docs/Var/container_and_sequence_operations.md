[⬅ Back to Table of Contents](var.md)

# Container & Sequence Operations

Working with `var` as a container or sequence.

- `len()` / `size()` / `var len(const var &v)`
- `void append(const var &v)` and templated `append(T&&)`
- `void add(const var &v)` and templated `add(T&&)`
- `void extend(const var &other)`
- `void update(const var &other)`
- `void remove(const var &v)`
- `var contains(const var &v)` / `var has(const var &v)`
- `iterator begin()` / `iterator end()` / `const_iterator ...` / `cbegin()` / `cend()`
- `var items()` / `var keys()` / `var values()`
- `var slice(long long start=0, long long end=LLONG_MAX, long long step=1)` and `operator()(...)` overload
