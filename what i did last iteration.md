# Pythonic Library Enhancement Plan (C++20)

**Target**: GCC 13 with full C++20 support

---

## Key Decisions (User-Confirmed)

| Topic | Decision |
|-------|----------|
| **C++ Standard** | Full C++20 |
| **Overflow Handling** | Promote types (int→long→long long→etc), then throw [PythonicOverflowError](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp#42-47) |
| **Fast Path Cache** | Deferred to future iteration - **REMINDER at end** |
| **User Containers** | Yes, `Iterable` concept enables user-defined containers |

---

## C++20 Features to Leverage

### 1. Concepts (Major)
Replace SFINAE with clean `requires` clauses:

```cpp
template<typename T>
concept Iterable = requires(T& t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() }   -> std::sentinel_for<decltype(t.begin())>;
};

template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

// Usage:
template<Iterable C>
var map(auto&& func, C&& container);  // Much cleaner!
```

### 2. Spaceship Operator `<=>`
Replace 6 comparison operators with one:

```cpp
// Current: operator<, operator<=, operator>, operator>=, operator==, operator!=
// New:
std::strong_ordering operator<=>(const var& other) const;
bool operator==(const var& other) const = default;  // Auto-generated
```

### 3. `[[likely]]` / `[[unlikely]]` Attributes
Optimize hot paths in switch statements:

```cpp
switch (tag_) {
case TypeTag::INT: [[likely]]
    return var(var_get<int>() + other.var_get<int>());
case TypeTag::GRAPH: [[unlikely]]
    throw PythonicTypeError("...");
}
```

### 4. Ranges Library
Simplify functional operations:

```cpp
#include <ranges>
// Instead of manual loops:
auto result = container | std::views::filter(pred) | std::views::transform(func);
```

### 5. `constexpr` Everywhere
Many helper functions can be `constexpr`:

```cpp
constexpr bool is_heap_type(TypeTag tag) noexcept { ... }
constexpr int getTypeRank(TypeTag t) noexcept { ... }
```

### 6. `std::span` for Views
Safe, non-owning views of contiguous data:

```cpp
std::span<const var> as_span() const;  // For list data
```

### 7. Lambda Improvements
- Template lambdas: `[](auto&&... args) { ... }`
- `[=, this]` capture
- Lambdas in unevaluated contexts

---

## Proposed Changes

### Phase 1: Error Handling System

#### [MODIFY] [pythonicError.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp)

Expand hierarchy with new error types:

```cpp
class PythonicRuntimeError : public PythonicError;
class PythonicAttributeError : public PythonicError;
class PythonicFileError : public PythonicError;
class PythonicIterationError : public PythonicError;
class PythonicGraphError : public PythonicError;
class PythonicNotImplementedError : public PythonicError;
class PythonicStopIteration : public PythonicError;

// Helper macro
#define PYTHONIC_THROW(ExType, msg) \
    throw ExType("pythonic: " + std::string(msg))
```

---

#### [MODIFY] [pythonicVars.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicVars.hpp)

- Replace ~90 `std::runtime_error` with Pythonic errors
- Add `[[likely]]`/`[[unlikely]]` to switch cases
- Make helper functions `constexpr`
- Add spaceship operator (optional: large change)

---

#### [MODIFY] [Graph.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/Graph.hpp)

Replace ~22 `std::runtime_error` with `PythonicGraphError`, [PythonicIndexError](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp#21-26), `PythonicFileError`

---

#### [MODIFY] [pythonicFile.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFile.hpp)

Replace ~15 `std::runtime_error` with `PythonicFileError`, [PythonicValueError](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp#31-36)

---

#### [MODIFY] [pythonicLoop.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp)

Replace ~4 `std::runtime_error` with [PythonicValueError](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp#31-36)

---

### Phase 2: Iterator/Container Unification

#### [MODIFY] [pythonicLoop.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp)

Add C++20 concepts at top of file:

```cpp
namespace pythonic::traits {

template<typename T>
concept Iterable = requires(T& t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() }   -> std::sentinel_for<decltype(t.begin())>;
};

template<typename T>
concept Container = Iterable<T> && requires(T& t) {
    typename T::value_type;
    { t.size() } -> std::convertible_to<std::size_t>;
};

} // namespace pythonic::traits
```

**This enables user-defined containers** that implement [begin()](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFile.hpp#408-413)/[end()](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp#358-359) to work with [map](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFunction.hpp#46-68), [filter](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFunction.hpp#113-141), [reduce](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFunction.hpp#198-232), [enumerate](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp#209-214), [zip](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp#339-344).

---

### Phase 3: Overflow Detection

#### [NEW] [pythonicOverflow.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicOverflow.hpp)

Checked arithmetic with type promotion:

```cpp
namespace pythonic::safe {

// Promote and add, throw only when largest type overflows
template<std::integral T>
auto checked_add(T a, T b) -> decltype(a + b) {
    // Try in current type, if overflow, promote
    if constexpr (sizeof(T) < sizeof(long long)) {
        return static_cast<long long>(a) + static_cast<long long>(b);
    } else {
        // At largest type - check and throw
        if ((b > 0 && a > std::numeric_limits<T>::max() - b) ||
            (b < 0 && a < std::numeric_limits<T>::min() - b)) {
            throw PythonicOverflowError("pythonic: integer overflow");
        }
        return a + b;
    }
}

} // namespace pythonic::safe
```

---

## Verification Plan

### Test Commands

```bash
cd /home/DATA/CODE/code/pythonic
g++ -std=c++20 -O2 -Wall -Wextra -I include test/test.cpp -o test_main && ./test_main
g++ -std=c++20 -O2 -I include test/test_type_promotion.cpp -o test_types && ./test_types
g++ -std=c++20 -O2 -I include test/test_math.cpp -o test_math && ./test_math
```

### New Error Handling Tests

Tests will catch exceptions properly without crashing:

```cpp
TEST("PythonicTypeError on string multiply");
bool caught_pythonic = false;
try {
    var x = var("a") * var("b");
} catch (const PythonicTypeError&) { 
    caught_pythonic = true;
} catch (...) { /* other exceptions */ }
ASSERT(caught_pythonic, "Should throw PythonicTypeError");
```

---

## Implementation Order

1. ✅ Expand [pythonicError.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicError.hpp) with full hierarchy
2. Replace errors in [pythonicVars.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicVars.hpp) (largest file)
3. Replace errors in [Graph.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/Graph.hpp), [pythonicFile.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicFile.hpp), [pythonicLoop.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp)
4. Add `Iterable` concept to [pythonicLoop.hpp](file:///home/DATA/CODE/code/pythonic/include/pythonic/pythonicLoop.hpp)  
5. Add `pythonicOverflow.hpp` for checked arithmetic
6. Integrate checked arithmetic (compile-time flag)
7. Add `[[likely]]`/`[[unlikely]]` attributes to hot paths
8. Run all tests

---

## Deferred (Future Iteration)

> [!NOTE]  
> **Fast Path Cache**: The `CachedBinOp` system for hot-loop optimization was skipped this iteration. Implement when performance profiling indicates a need.
