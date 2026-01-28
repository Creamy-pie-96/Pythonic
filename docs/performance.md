# Performance

Check out [benchmark data](../benchmark/benchmark_report.md)
Internals that affect performance and recommended hot-path strategies.

Topics:

- `VarData` union layout and memory model
- Which types are heap-allocated (string, list, dict, set, ordered variants, graph)
- Cost of copy vs move; move semantics examples
- Fast-path cache and `type_tag()` usage
- Guidelines: use `is_*()` + `as_*_unchecked()` in hot loops; prefer `type_cstr()` over `type()` in hot code

Notes on future improvements (smart pointers, variant refactor).
