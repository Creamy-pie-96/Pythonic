[⬅ Back to Table of Contents](../index.md)
# `var`: A Pythonic Dynamic Type for C++

`var` is a dynamic, type-erased value type for C++ inspired by Python's flexible variables. It allows you to store, manipulate, and pass around values of many types (integers, floats, strings, lists, sets, dicts, and more) with a unified, high-level API. The `var` class brings Python-like expressiveness and convenience to C++ code, supporting:

- Dynamic typing: assign and reassign values of different types to the same variable
- Rich built-in operations: arithmetic, comparison, string and container methods, slicing, and more
- Pythonic containers: list, set, dict, ordered variants, and graph types
- Type introspection and safe conversion
- Functional helpers: map, filter, reduce, etc.
- Seamless integration with C++ types and idioms

This documentation is an orchestrator for all `var`-related API and usage guides. Use the links below to explore construction, operations, and advanced features.

Table of Contents
- [DataTypes](dtypes.md)
- [Construction & Lifetime](construction_and_lifetime.md)
- [Type Introspection & Conversion](type_introspection_and_conversion.md)
- [Container & Sequence Operations](container_and_sequence_operations.md)
- [String-like Methods (on string `var`)](string_like_methods.md)
- [Numeric / Arithmetic](numeric_arithmetic.md)
- [Comparison & Boolean](comparison_and_boolean.md)
- [Iterators, Mapping & Functional Helpers](iterators_mapping_functional.md)
- [Graph Helpers (if using graph type)](graph_helpers.md)

# Next Check

- [Math](../Math/math.md)