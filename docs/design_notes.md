# Design Notes

Rationale, tradeoffs, and internal design decisions.

Topics:

- Why `VarData` union + `TypeTag` (performance vs safety)
- Promotion policy rationale and alternatives considered
- Rejected ideas and why (e.g., full `std::variant`, different promotion rules)
- Technical debt and recommended refactors (remove unused helpers, consider smart pointers)

Notes for maintainers and contributors.
