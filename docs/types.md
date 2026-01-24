# Types

Covers the `TypeTag` enum and mappings:

- List of `TypeTag` entries (NONE, BOOL, INT, UINT, LONG, ... STRING, LIST, DICT, SET, ORDERED\* , GRAPH)
- What `type_cstr()` returns for each tag
- How C++ native types map to tags
- `type_name_to_tag()` helper behavior
- `NoneType` semantics and comparisons

Quick table: C++ type → `TypeTag` → example

Notes about GraphPtr and other special types.
