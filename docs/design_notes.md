[⬅ Back to Table of Contents](index.md)

# Design Notes

Rationale, tradeoffs, and internal design decisions.

---

## Core Design: `VarData` union + `TypeTag`

The `var` class uses a union (`VarData`) with a discriminator enum (`TypeTag`) for type-safe dynamic typing.

| Choice                    | Rationale                                                                                                                                                                                                |
| ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Union over `std::variant` | Better control over memory layout, no exception-based type checking                                                                                                                                      |
| Manual tag management     | Allows O(1) type dispatch via function pointer tables                                                                                                                                                    |
| 18 type tags              | Covers: `NONE`, `BOOL`, `INT`, `LONGLONG`, `DOUBLE`, `STRING`, `LIST`, `SET`, `DICT`, `ORDEREDSET`, `ORDEREDDICT`, `GRAPH`, `NODEDATA`, `GRAPHRESULT`, `FUNCTION`, `NAN_VALUE`, `INF_VALUE`, `NONE_TYPE` |

---

## Dispatch Table System

Operators use **18×18 function pointer dispatch tables** for O(1) type-based dispatch.

| Aspect   | Design                                                                                                                                                                                 |
| -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Tables   | `add_table`, `sub_table`, `mul_table`, `div_table`, `mod_table`, `eq_table`, `ne_table`, `lt_table`, `le_table`, `gt_table`, `ge_table`, `and_op_table`, `or_op_table`, `xor_op_table` |
| Lookup   | `dispatch::add_table[lhs.tag_][rhs.tag_](lhs, rhs)`                                                                                                                                    |
| Default  | Unsupported combinations throw `PythonicTypeError`                                                                                                                                     |
| Code gen | `gen_dispatch_stubs.py` generates stubs for easy extension                                                                                                                             |

**Why tables over if/else chains:**

- O(1) vs O(n) dispatch time
- Easier to extend - add function, register in table
- Cleaner code organization

---

## Mutation Detection

Python throws `RuntimeError` when modifying a container during iteration. We replicate this:

| Component             | Implementation                                                                         |
| --------------------- | -------------------------------------------------------------------------------------- |
| `version_` counter    | `mutable uint32_t` in `var`, incremented by mutating methods                           |
| `check_version()`     | Iterator method that compares stored version vs current                                |
| `increment_version()` | Called by: `append()`, `add()`, `remove()`, `clear()`, `pop()`, `extend()`, `update()` |
| Thread safety         | Not thread-safe (single-threaded design)                                               |

**Why version counters over locking:**

- Zero overhead for non-iterating code
- Simple implementation
- Matches Python's behavior exactly

---

## Type Promotion Policy

Arithmetic operations use smart type promotion:

| Operation         | Result Type   | Rationale                                |
| ----------------- | ------------- | ---------------------------------------- |
| `int + int`       | `int`         | No promotion needed                      |
| `int + double`    | `double`      | Wider type wins                          |
| `int + long long` | `long long`   | Wider type wins                          |
| `int / int`       | `double`      | Python semantics (always float division) |
| `int // int`      | `int` (floor) | Python floor division                    |

---

## Rejected Ideas

| Idea                        | Why Rejected                                   |
| --------------------------- | ---------------------------------------------- |
| Full `std::variant`         | Too much overhead, exception-based type errors |
| Virtual dispatch            | Too slow for tight arithmetic loops            |
| Shared ownership everywhere | Reference semantics would surprise users       |
| Macro-only loops            | Not type-safe, poor IDE support                |

---

## Class-Based Loop Helpers

The loop macros (`for_each`, `for_index`, etc.) are deprecated in favor of:

| Helper                          | Benefit                                       |
| ------------------------------- | --------------------------------------------- |
| `each(container, func)`         | Type-safe, IDE completion, early exit support |
| `each_indexed(container, func)` | Index + value with proper scoping             |
| `indexed(container)`            | Range-for with structured bindings            |
| `times(n, func)`                | Clean N-repetition pattern                    |

---

## Technical Debt & Recommended Refactors

| Item                               | Status | Notes                           |
| ---------------------------------- | ------ | ------------------------------- |
| Remove unused helpers              | TODO   | Audit and clean up              |
| Consider smart pointers for graphs | TODO   | Prevent cycles/leaks            |
| Add `rbegin()/rend()` to var       | TODO   | Enable `reversed()`             |
| Benchmark dispatch tables          | DONE   | ~30% faster than if/else chains |

---

## Next check

[Contributing](contributing.md)
