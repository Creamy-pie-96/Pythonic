[⬅ Back to Table of Contents](index.md)
[⬅ Back to Contributing](contributing.md)

# Changelog

Will keep release notes and notable changes here.

Will use semantic versioning and short incremental notes for each release.

---

## [Unreleased]

### Added

#### Dispatch Table-Based Operator System

- **Refactored operator dispatch** using function pointer tables for O(1) type-based dispatch
- 18×18 dispatch matrix covering all `TypeTag` combinations for arithmetic/comparison operators
- Separate dispatch tables for: `add`, `sub`, `mul`, `div`, `mod`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `and_op`, `or_op`, `xor_op`
- Stubs generated via `gen_dispatch_stubs.py` for easy extensibility
- Files: `pythonicDispatch.hpp`, `pythonicDispatchForwardDecls.hpp`, `pythonicDispatchStubs.cpp`

#### Mutation Detection During Iteration

- **Python-like safety**: Containers now track a version counter (`version_`)
- Iterators store the version at construction and check on dereference
- Throws `PythonicRuntimeError` if container is mutated during iteration
- Applies to: `list`, `set`, `dict`, `ordered_set`, `ordered_dict`, `string`
- Affected operations: `append()`, `add()`, `remove()`, `clear()`, `pop()`, `extend()`, `update()`
- New methods: `get_version()` returns current version counter

#### Class-Based Loop Helpers

- **`each(container, func)`**: Type-safe iteration with lambda, supports early exit
- **`each_indexed(container, func)`**: Iteration with index and value
- **`indexed(container)`**: Range-based for yielding `(index, value)` pairs
- **`times(n, func)`**: Execute function n times with optional index
- **`until(container, pred)`**: Iterate until predicate returns true
- **`while_each(container, pred)`**: Iterate while predicate returns true

### Changed

#### Type Promotion Logic

- Updated smart type promotion for arithmetic operations
- Better handling of mixed-type operations (int/double, int/long long, etc.)
- Consistent Python-like division semantics (always returns double for `/`)

### Deprecated

- **Loop macros**: `for_each`, `for_index`, `for_enumerate`, `for_range` are now deprecated
- Prefer class-based helpers (`each()`, `indexed()`, `enumerate()`, `range()`) for type safety

### Documentation

- **Math documentation** updated with `Overflow::None_of_them` policy (raw C++ arithmetic)
- **Overflow policy guide** with recommendations for different use cases
- **Type promotion documentation** expanded with strategies and smart fit behavior
- **Type access methods** documented: `get<T>()`, `toInt()`, `toDouble()`, `is<T>()`, etc.
- **Operator overflow behavior** clarified: operators use `None_of_them`, functions use `Throw`

---

## Next check

- [QuickStart](examples/quickstart.md)
