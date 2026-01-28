[⬅ Back to Table of Contents](index.md)
[⬅ Back to Math](../Math/math.md)

# Errors and Exceptions

This page documents the error and exception model in Pythonic, including the full error hierarchy, common error types, and idiomatic error handling.

---

## Error Hierarchy

All Pythonic exceptions inherit from `pythonic::PythonicError`, allowing you to catch all library errors with a single catch block, while still supporting granular error handling.

| Exception Type                | Description                                      | Example Usage                      |
|-------------------------------|--------------------------------------------------|------------------------------------|
| `PythonicError`               | Base class for all Pythonic errors               | `catch (const PythonicError& e)`   |
| `PythonicWarning`             | Base for warnings (not errors)                   |                                    |
| `PythonicTypeError`           | Wrong type operation                            | `catch (const PythonicTypeError& e)`|
| `PythonicValueError`          | Right type, wrong value                         |                                    |
| `PythonicIndexError`          | Sequence index out of bounds                    |                                    |
| `PythonicKeyError`            | Mapping key not found                           |                                    |
| `PythonicOverflowError`       | Numeric overflow                                |                                    |
| `PythonicZeroDivisionError`   | Division or modulo by zero                      |                                    |
| `PythonicFileError`           | File operation failed                           |                                    |
| `PythonicAttributeError`      | Attribute reference/assignment failed           |                                    |
| `PythonicGraphError`          | Graph-specific operation failure                |                                    |
| `PythonicIterationError`      | Iterator exhausted or used incorrectly          |                                    |
| `PythonicStopIteration`       | Signals iterator exhaustion (like Python)        |                                    |
| `PythonicRuntimeError`        | General runtime error                           |                                    |
| `PythonicNotImplementedError` | Feature not yet implemented                     |                                    |

---

## Common Error Types

### Type Errors
Raised when an operation receives a value of the wrong type.
- Example: `"hello" * "world"`, `Int("not a number")`

### Value Errors
Raised when a value is inappropriate (right type, wrong content).
- Example: `range(0, 10, 0)`, `sqrt(-1)`

### Index Errors
Raised when a sequence index is out of range.
- Example: `list[100]` when `len(list) == 5`

### Key Errors
Raised when a mapping key is not found.
- Example: `dict["nonexistent_key"]`

### Arithmetic Errors
- `PythonicOverflowError`: Raised when numeric operation overflows and promotion is exhausted.
- `PythonicZeroDivisionError`: Raised on division or modulo by zero.

### File/IO Errors
Raised when a file operation fails (not found, cannot open, not open).

### Attribute Errors
Raised when an attribute reference or assignment fails.
- Example: Calling `.upper()` on an int

### Graph Errors
Raised for graph-specific operation failures (invalid node, edge not found, cycle detected).

### Iterator Errors
- `PythonicIterationError`: Raised when an iterator is exhausted or used incorrectly.
- `PythonicStopIteration`: Signals iterator exhaustion (like Python's `StopIteration`).

### Runtime & Not Implemented Errors
- `PythonicRuntimeError`: General runtime errors.
- `PythonicNotImplementedError`: Feature not yet implemented.

---

## Idiomatic Error Handling

You can catch specific errors or all Pythonic errors:

```cpp
try {
    // Some operation that may fail
    var result = Int("not a number");
} catch (const pythonic::PythonicTypeError& e) {
    print("Type error:", e.what());
} catch (const pythonic::PythonicError& e) {
    print("Any pythonic error:", e.what());
}
```

### Example: Arithmetic Error
```cpp
try {
    var x = INT_MAX;
    var y = 1;
    print(add(x, y)); // May throw PythonicOverflowError
} catch (const pythonic::PythonicOverflowError& e) {
    print("Overflow:", e.what());
}
```

### Example: Index Error
```cpp
try {
    var lst = list(1,2,3);
    print(lst[10]); // Throws PythonicIndexError
} catch (const pythonic::PythonicIndexError& e) {
    print("Index error:", e.what());
}
```

---

## Notes

- All error messages are prefixed with `pythonic:` for clarity.
- Most errors support optional source location tracking for debugging (in debug builds).
- Use the `PYTHONIC_THROW` macro to throw with source location info.
- Warnings (`PythonicWarning`) can be caught separately from errors.
- The error hierarchy allows both broad and fine-grained exception handling.

---

## Next Check

- [Iteration](../Loops/iteration.md)