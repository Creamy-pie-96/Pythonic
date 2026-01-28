[⬅ Back to Var Table of Contents](var.md)

# Containers and Data Types

This page lists all the data types and containers supported by `var` in Pythonic. For construction, lifetime, and API details, see the dedicated documentation pages.

---

## Supported Data Types

| TypeTag Enum  | C++ Type / Alias                            | Description                       |
| ------------- | ------------------------------------------- | --------------------------------- |
| `NONE`        | NoneType                                    | Pythonic None/null                |
| `BOOL`        | bool                                        | Boolean                           |
| `INT`         | int                                         | Integer (32-bit)                  |
| `UINT`        | unsigned int                                | Unsigned integer (32-bit)         |
| `LONG`        | long                                        | Long integer (platform-dependent) |
| `ULONG`       | unsigned long                               | Unsigned long                     |
| `LONG_LONG`   | long long                                   | Long long integer (64-bit)        |
| `ULONG_LONG`  | unsigned long long                          | Unsigned long long (64-bit)       |
| `FLOAT`       | float                                       | Single-precision float            |
| `DOUBLE`      | double                                      | Double-precision float            |
| `LONG_DOUBLE` | long double                                 | Extended-precision float          |
| `STRING`      | std::string                                 | String                            |
| `LIST`        | List (std::vector<var>)                     | Pythonic list (dynamic array)     |
| `SET`         | Set (std::unordered_set<var>)               | Pythonic set (hash set)           |
| `ORDEREDSET`  | OrderedSet (std::set<var>)                  | Ordered set (sorted, unique)      |
| `DICT`        | Dict (std::unordered_map<std::string, var>) | Pythonic dict (hash map)          |
| `ORDEREDDICT` | OrderedDict (std::map<std::string, var>)    | Ordered dict (sorted keys)        |
| `GRAPH`       | GraphPtr (VarGraph)                         | Graph (see graph.md)              |

---

## Container Aliases

| Alias         | Underlying Type                                | Notes                      |
| ------------- | ---------------------------------------------- | -------------------------- |
| `List`        | `std::vector<var>`                             | Pythonic list              |
| `Set`         | `std::unordered_set<var, VarHasher, VarEqual>` | Pythonic set               |
| `OrderedSet`  | `std::set<var>`                                | Maintains sorted order     |
| `Dict`        | `std::unordered_map<std::string, var>`         | Pythonic dict              |
| `OrderedDict` | `std::map<std::string, var>`                   | Maintains sorted key order |
| `GraphPtr`    | `std::shared_ptr<VarGraphWrapper>`             | Graph object               |

---

## Notes

- All containers store elements as `var` for full dynamic typing.
- Containers and strings are heap-allocated and managed by `var`.
- For construction and API usage, see:
  - [Construction & Lifetime](Var/construction_and_lifetime.md)
  - [Container & Sequence Operations](Var/container_and_sequence_operations.md)
- Graph support is provided via [`VarGraph`](graph_helpers.md).
- Additional container types (e.g., linked lists) may be added in the future.

---

## Next check

- [Construction & Lifetime](construction_and_lifetime.md)
