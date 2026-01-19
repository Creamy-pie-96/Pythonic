# Pythonic C++ Library - Development Plan

**Generated:** January 19, 2026  
**Status:** In Progress

## Current Library Assessment

### 🔴 NEXT PRIORITY: BigInt Implementation

Adding arbitrary-precision integers (BigInt) to support:
- Numbers larger than `long long` (beyond 2^63)
- Crypto applications requiring large number arithmetic
- Python-like unlimited integer precision
- Integration with existing overflow/promotion system

**Implementation Plan:**
1. Create `BigInt` class with string-based storage or digit array
2. Implement basic operations: +, -, *, /, %, comparisons
3. Add conversion to/from other numeric types
4. Integrate with `var` type as `TypeTag::BIGINT`
5. Update overflow promotion to use BigInt when long_long overflows

---

### What We Have ✅

1. **Dynamic Typing (`var`)**
   - All primitive types: bool, int, uint, long, ulong, long_long, ulong_long, float, double, long_double
   - Container types: List, Set, Dict, OrderedSet, OrderedDict
   - String type with Python-like methods
   - Graph data structure
   - None type
   - Type checking and conversion

2. **Containers**
   - List with slicing, comprehensions, operators (|, &, -)
   - Dict with keys/values/items
   - Set with union/intersection/difference (|, &, -, ^)
   - OrderedSet (sorted set)
   - OrderedDict (sorted dictionary)

3. **String Methods**
   - upper, lower, strip, split, replace, find, rfind
   - startswith, endswith, contains
   - Slicing support

4. **Math Library**
   - Basic: round, pow, sqrt, exp, log, log10, log2
   - Trigonometry: sin, cos, tan, asin, acos, atan, atan2
   - Hyperbolic: sinh, cosh, tanh
   - Constants: pi, e
   - Random: random_int, random_float, random_choice, fill_random*
   - Advanced: gcd, lcm, factorial, hypot, product
   - Aggregation: min, max, sum

5. **Overflow Handling**
   - Three policies: Throw, Promote, Wrap
   - Smart type promotion with `smallest_fit` parameter
   - Type ranking system

6. **Functional Programming**
   - map, filter, reduce
   - map_indexed
   - sorted, reverse
   - flatten, unique
   - take, drop, take_while, drop_while
   - all_of, any_of, none_of

7. **Iteration**
   - range()
   - enumerate()
   - zip()
   - reversed()
   - Loop macros (for_each, for_idx, etc.)

8. **File I/O**
   - File class with Python-like modes
   - read, write, readline, readlines
   - Context manager pattern (with_file)

9. **C++20 Features**
   - Concepts (Iterable, Container, Sized, etc.)
   - Ranges integration
   - Fast path caching for hot loops

10. **Graph Algorithms**
    - DFS, BFS traversals
    - Shortest paths (Dijkstra, Bellman-Ford, Floyd-Warshall)
    - MST (Prim's algorithm)
    - Topological sort
    - Connected/Strongly Connected Components
    - Cycle detection

---

## Known Issues 🐛

### Issue 1: Missing `group_by` in Demo
**Status:** Known limitation  
**Description:** Demo skips `group_by` due to API constraints

---

## Missing Features & Enhancement Roadmap 📋

### Priority 1: Python Parity Features 🔴

#### 1.1 String Methods (Missing)
- [ ] `join()` - `", ".join(list)` → `"a, b, c"`
- [ ] `format()` - String formatting `"{} + {} = {}".format(1, 2, 3)`
- [ ] `isdigit()`, `isalpha()`, `isalnum()`, `isspace()`
- [ ] `center()`, `ljust()`, `rjust()` - Padding
- [ ] `count()` - Count substring occurrences
- [ ] `title()`, `capitalize()`, `swapcase()`
- [ ] `zfill()` - Zero padding
- [ ] `encode()`, `decode()` - Encoding support
- [ ] `maketrans()`, `translate()` - Character mapping

#### 1.2 List Methods (Missing/Incomplete)
- [ ] `insert(index, item)` - Insert at position
- [ ] `index(item)` - Find index of item
- [ ] `count(item)` - Count occurrences
- [ ] `copy()` - Shallow copy
- [ ] `clear()` - Remove all items
- [ ] `pop(index=-1)` - Pop with index parameter

#### 1.3 Dict Methods (Missing)
- [ ] `get(key, default)` - Get with default value
- [ ] `setdefault(key, default)` - Set default if missing
- [ ] `pop(key, default)` - Remove and return
- [ ] `popitem()` - Remove and return last item
- [ ] `update(other_dict)` - Merge dictionaries
- [ ] `copy()` - Shallow copy
- [ ] `clear()` - Remove all items

#### 1.4 Set Methods (Missing)
- [ ] `copy()` - Shallow copy
- [ ] `clear()` - Remove all items
- [ ] `pop()` - Remove and return arbitrary element
- [ ] `discard(item)` - Remove without error if missing
- [ ] `issubset()`, `issuperset()`, `isdisjoint()`

### Priority 2: Usability Improvements 🟡

#### 2.1 Better Error Messages
- [ ] Include type names in error messages
- [ ] Add suggestions for common mistakes
- [ ] Better stack traces with source_location

#### 2.2 Iterator Protocol
- [ ] `iter()` function to create iterators
- [ ] `next()` function
- [ ] StopIteration exception
- [ ] Generator support (yield-like behavior)

#### 2.3 Context Managers
- [ ] `with_statement` macro/helper
- [ ] Resource cleanup guarantees
- [ ] Exception-safe cleanup

#### 2.4 Tuple Support
- [ ] Immutable tuple type
- [ ] Tuple unpacking
- [ ] Named tuples (like Python's namedtuple)

### Priority 3: Advanced Features 🟢

#### 3.1 Regular Expressions
- [ ] `re_match()`, `re_search()`
- [ ] `re_findall()`, `re_sub()`
- [ ] Compiled pattern caching

#### 3.2 JSON Support
- [ ] `json_loads()` - Parse JSON string to var
- [ ] `json_dumps()` - Serialize var to JSON string
- [ ] Pretty printing options
- [ ] JSON file read/write

#### 3.3 Date/Time
- [ ] `datetime` type
- [ ] Date arithmetic
- [ ] Formatting and parsing
- [ ] Timezone support

#### 3.4 More Math Functions
- [ ] `ceil()`, `floor()` (may exist but verify)
- [ ] `copysign()`, `fmod()`
- [ ] `isnan()`, `isinf()`, `isfinite()`
- [ ] `frexp()`, `ldexp()`, `modf()`
- [ ] `erf()`, `gamma()`, `lgamma()`
- [ ] Statistics: `mean()`, `median()`, `stdev()`, `variance()`

#### 3.5 Collections Module
- [ ] `Counter` - Count hashable objects
- [ ] `defaultdict` - Dict with default factory
- [ ] `deque` - Double-ended queue
- [ ] `ChainMap` - Group dicts together

#### 3.6 Itertools Module
- [ ] `chain()` - Chain iterables
- [ ] `cycle()` - Infinite cycling
- [ ] `repeat()` - Repeat value
- [ ] `combinations()`, `permutations()`
- [ ] `product()` - Cartesian product
- [ ] `accumulate()` - Running totals
- [ ] `groupby()` - Group consecutive elements

### Priority 4: Performance & Optimization 🔵

#### 4.1 Memory Optimization
- [ ] Small string optimization
- [ ] Custom allocator support
- [ ] Move semantics audit
- [ ] Copy-on-write for large containers

#### 4.2 Parallel Processing
- [ ] Parallel map/filter/reduce
- [ ] Thread-safe containers
- [ ] Async file I/O

#### 4.3 SIMD Optimization
- [ ] Vectorized arithmetic operations
- [ ] Batch processing helpers

### Priority 5: Ecosystem 🟣

#### 5.1 Testing
- [ ] Comprehensive unit test suite
- [ ] Integration tests
- [ ] Fuzz testing
- [ ] Property-based testing

#### 5.2 Documentation
- [ ] API reference (Doxygen)
- [ ] More examples
- [ ] Performance guide
- [ ] Migration guide from Python

#### 5.3 Build System
- [ ] Package managers (vcpkg, Conan)
- [ ] CI/CD pipeline
- [ ] Code coverage

---

## Implementation Order Recommendation

1. **Phase 1: Core Completeness** (1-2 weeks)
   - Complete string methods (join, format, is* methods)
   - Complete list methods (insert, index, count)
   - Complete dict methods (get, setdefault, pop)
   - Fix known bugs

2. **Phase 2: Python Parity** (2-3 weeks)
   - Tuple support
   - Iterator protocol
   - JSON support
   - Regular expressions

3. **Phase 3: Advanced Features** (3-4 weeks)
   - Date/time
   - Collections module
   - Itertools module
   - Statistics functions

4. **Phase 4: Polish** (ongoing)
   - Performance optimization
   - Better error messages
   - Documentation
   - Testing

---

## Benchmark Notes

Run benchmarks with:
```bash
cd benchmark
cmake .
make
./benchmark
```

Also run Python benchmarks for comparison:
```bash
python benchmark.py
```

---

## Notes

- All changes should maintain backward compatibility
- Focus on Python 3.x semantics
- Prefer compile-time checks over runtime where possible
- Keep header-only design for easy distribution
