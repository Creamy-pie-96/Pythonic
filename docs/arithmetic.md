# Arithmetic and Type Promotion

Topics:

- `getTypeRank()` ranking and rationale
- `getPromotedType()` rules for mixed-type operations
- `addPromoted`, `subPromoted`, `mulPromoted`, `divPromoted`, `modPromoted`
- Unsigned vs signed rules
- Floating precedence and "smallest-fit" strategy
- Overflow behavior: checked same-type arithmetic vs fallback to floating
- Division always returns floating

Examples:

- `int + uint` → which tag and why
- `long * float` → result tag and precision notes
- Edge-case examples showing overflow fallback
