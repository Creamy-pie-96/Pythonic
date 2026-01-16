# Pythonic Loop & Function Optimization Plan

## Current State (from benchmark_after_optimized.md)

- **Map (transform)**: 28.71x slower than C++ (0.120ms vs 0.004ms)
- **Filter**: 13.06x slower than C++ (0.031ms vs 0.002ms)
- **Loop (for_in + range)**: 4657.64x slower than C++ (4.555ms vs 0.001ms)
- **Loop over Container**: 11.47x slower than C++ (0.017ms vs 0.001ms)

---

## Optimization Targets

### 1. `pythonicLoop.hpp` — Range Iterator

**Problem**: The `range` class iterator uses virtual-like checks and branching on every increment/comparison.

**Optimizations**:
| Change | Why |
|--------|-----|
| Remove `forward_` bool from iterator | Replace with compile-time or constructor-time logic |
| Inline `operator++` and `operator!=` | Avoid function call overhead |
| Use `[[likely]]` / `[[unlikely]]` hints | Help branch prediction |
| Consider separate `forward_range` / `reverse_range` | Eliminate runtime branching entirely |
| Pre-compute end sentinel | Avoid recalculating in `end()` |

**Alternative**: Template the step direction (`range<true>` for forward, `range<false>` for backward).

---

### 2. `pythonicLoop.hpp` — Enumerate / Zip / Reversed

**Problem**: These wrappers use `std::variant` or type-erased iterators internally.

**Optimizations**:
| Change | Why |
|--------|-----|
| Use direct iterator storage (no variant) | Avoid variant dispatch |
| Mark methods `constexpr` / `inline` | Enable more inlining |
| Avoid `std::distance` in `end()` | Pre-compute or cache size |

---

### 3. `pythonicFunction.hpp` — Map / Filter / Reduce

**Problem**: Every iteration:

1. Checks `if constexpr` (compile-time OK, but template bloat)
2. Creates temporary `var` objects
3. Pushes to `List` (which is `std::vector<var>`)

**Optimizations**:
| Change | Why |
|--------|-----|
| Reserve capacity in result `List` | Avoid reallocations |
| Use `emplace_back` instead of `push_back` | Avoid copy |
| Add overloads for native containers | Skip `var` wrapping when not needed |
| Consider lazy evaluation (generator-style) | Avoid materializing full list |

---

### 4. `pythonicFunction.hpp` — Sorted / Slice / Flatten

**Problem**: These copy entire containers before operating.

**Optimizations**:
| Change | Why |
|--------|-----|
| Reserve capacity before loop | Avoid reallocations |
| Move instead of copy where possible | Reduce allocations |
| Use `std::move` semantics for rvalues | Enable move optimization |

---

### 5. General Patterns

| Pattern                      | Optimization                                                                                   |
| ---------------------------- | ---------------------------------------------------------------------------------------------- |
| `for (auto item : iterable)` | Change to `for (auto& item : iterable)` or `for (const auto& item : iterable)` to avoid copies |
| `result.push_back(var(...))` | Use `result.emplace_back(...)`                                                                 |
| `std::distance(begin, end)`  | Cache or avoid if possible                                                                     |
| Template branches            | Prefer tag dispatch or SFINAE to reduce instantiation overhead                                 |

---

## Implementation Priority

1. **High Impact**: Reserve capacity in `map`, `filter`, `list_comp`, `set_comp`
2. **High Impact**: Avoid copies with `const auto&` in loops
3. **Medium Impact**: Optimize `range` iterator (remove runtime branching)
4. **Medium Impact**: Use `emplace_back` everywhere
5. **Low Impact**: Lazy iterators (more complex, future work)

---

## Metrics to Track

| Metric                | Before  | After |
| --------------------- | ------- | ----- |
| Map (transform)       | 0.120ms | ?     |
| Filter                | 0.031ms | ?     |
| Loop (for_in + range) | 4.555ms | ?     |
| Loop over Container   | 0.017ms | ?     |

---

## Next Steps

1. Run baseline benchmark (in progress)
2. Apply optimizations in order of priority
3. Re-run benchmark after each batch
4. Document improvements

---

_Created: Jan 16, 2026_
