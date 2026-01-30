# Deep Plan for Improving pythonic::vars::var (TODOs 2, 4, 5, 6, 7)

## 2. Rule of Five: Destructor, Copy/Move Constructor, Assignment

- **Difficulty:** Moderate to High (requires careful memory management)
- **Plan:**
  - Audit all heap-allocated types (string, List, Set, Dict, etc.).
  - Implement:
    - Destructor: always call `destroy_heap_data()` for heap types.
    - Copy constructor: deep copy heap data using `copy_heap_data()`.
    - Move constructor: transfer ownership, null out source pointer.
    - Copy assignment: destroy old, deep copy new.
    - Move assignment: destroy old, move new, null out source.
  - Add tests for all types and edge cases (self-assignment, exception safety).

## 4. Smart Pointer or Copy-on-Write for Heap Objects

- **Difficulty:** Moderate
- **Plan:**
  - Replace raw `void*` for heap types with `std::shared_ptr<void>` or type-erased smart pointer.
  - For containers, use `std::shared_ptr<T>` for each type (e.g., `std::shared_ptr<List>`).
  - Optionally, implement copy-on-write: on copy, share pointer; on mutation, clone if refcount > 1.
  - Update all accessors and mutators to handle smart pointers.
  - Audit for performance and memory leaks.

## 5. Detect Mutation During Iteration (Dict/Set)

- **Difficulty:** Moderate
- **Plan:**
  - Add a mutation counter (integer) to each Dict/Set instance.
  - On every mutating operation (insert, erase, clear), increment the counter.
  - Iterators capture the counter value at creation; on each use, check if it changed.
  - If changed, throw a `pythonic::PythonicRuntimeError` ("dict/set mutated during iteration").
  - Add tests for all container types.

## 6. Enforce Deep Const-Correctness

- **Difficulty:** High
- **Plan:**
  - Audit all methods: ensure `const var` does not allow mutation of underlying data.
  - For heap types, make sure returned references/pointers are `const` in const methods.
  - Consider using `std::shared_ptr<const T>` for heap data in const `var`.
  - Add static analysis and tests to catch accidental mutation.

## 7. Operator Dispatch Table for Arithmetic

- **Difficulty:** Moderate
- **Plan:**
  - Replace large `switch`/`if` chains in arithmetic operators with a static dispatch table (matrix of function pointers or lambdas).
  - Each [TypeTag][TypeTag] pair maps to the correct function for +, -, \*, etc.
  - Improves extensibility (add new types/ops easily) and performance (no repeated type checks).
  - Add tests for all operator combinations.

---

## General Steps

1. Design and prototype each feature in isolation.
2. Write unit tests for all edge cases and error conditions.
3. Integrate features one by one, refactoring as needed.
4. Document new behaviors and update usage examples.

---

**This plan targets safety, performance, and Pythonic behavior for the var class.**
