# Print and Pretty Print

Document the `print` and `pprint` helpers and recommended usage.

Contents:

- `print()` basics: printing `var` values, multiple arguments, separators
- `pprint()` / pretty-print: when to use, indentation, `pretty_str()` behavior
- Integration with `str()` / `toString()` / `pretty_str()` and `operator<<`
- File output helpers: writing `var` to streams or files
- Examples:
  - print primitive and container values
  - pprint nested lists/dicts/graphs
  - redirecting output to files

Notes:

- `print` uses `str()`/`operator<<` so large containers produce readable output; use `pprint` for nested structures.
- For performance-sensitive logging, serialize selectively rather than printing large `var` blobs.
