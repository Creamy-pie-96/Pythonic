[⬅ Back to main readme file](../README.md)

# Common Pitfalls

- **C++20 required** - Won't compile with older standards
- **No automatic conversions from strings** - Use `Int()`, `Float()`, etc. when converting from strings
- **Dictionary keys** - Currently only support string keys
- **Index checking** - Out of bounds returns None instead of throwing
- **Comparison** - Different types compare as not equal (no implicit conversion)
- **Operator ambiguity with slicing** - When using `slice()` with `None`, wrap numeric literals in `var()`: `slice(None, None, var(-1))`
- **Index ambiguity** - Use `size_t` for list indices when 0 might be ambiguous: `lst[size_t(0)]`
- **DynamicVar arithmetic** - Convert `let()` variables to `var` first for arithmetic: `var v = let(x); let(x) = v + 50;`
- **Type Conversions** - The `var` type provides explicit conversion operators, so `static_cast<int>()`, `static_cast<double>()` work correctly. However, for clarity and best practice, prefer using the explicit conversion methods `.toInt()`, `.toDouble()`, `.toString()`:

  ```cpp
  var x = 42;
  int i1 = static_cast<int>(x);    // Works ✓ (returns 42)
  int i2 = x.toInt();              // Recommended ✓ (clearer intent)

  // For list/path access, both work:
  var path = list(0, 2, 4, 3);
  size_t idx1 = static_cast<size_t>(static_cast<int>(path[1]));  // Works ✓
  size_t idx2 = static_cast<size_t>(path[1].toInt());            // Clearer ✓
  ```

# Safety Features

- **Overflow Detection** - Integer arithmetic operations (+, -, \*, /) are checked for overflow and throw `PythonicOverflowError`
- **Zero Division** - Division/Modulo by zero throws `PythonicZeroDivisionError`
- **Safe Iteration** - `as_span()` provides safe views of list data
