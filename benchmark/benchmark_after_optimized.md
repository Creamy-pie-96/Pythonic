# Pythonic C++ Library Benchmark Report

Generated on: Jan 16 2026 00:51:38

## Configuration

- **Iterations (Arithmetic/Comparisons)**: 1000000
- **Small Iterations (Strings/Containers)**: 10000
- **Container Size**: 1000

## Three-Way Comparison

| Operation | C++ (ms) | Pythonic (ms) | Python (ms) | Pythonic vs C++ | Pythonic vs Python |
|-----------|----------|---------------|-------------|-----------------|--------------------|
| Integer Addition | 0.002 | 4.949 | 29.413 | 2625.43x | 0.17x |
| Integer Multiplication | 0.001 | 4.881 | 34.616 | 5381.60x | 0.14x |
| Double Addition | 0.001 | 4.839 | 25.910 | 3300.82x | 0.19x |
| Integer Comparison | 0.001 | 12.432 | 42.201 | 12712.01x | 0.29x |
| String Concatenation | 0.231 | 0.656 | 0.161 | 2.84x | 4.08x |
| String Comparison | 0.001 | 12.479 | 19.462 | 12759.93x | 0.64x |
| List Creation (10 elements) | 0.122 | 2.628 | 0.528 | 21.60x | 4.98x |
| Set Creation (10 elements) | 2.126 | 9.971 | 1.329 | 4.69x | 7.50x |
| List Append | 0.016 | 0.117 | 0.047 | 7.23x | 2.47x |
| List Access (indexed) | 0.001 | 0.013 | 0.041 | 9.51x | 0.32x |
| Set Insertion | 0.057 | 0.129 | 0.049 | 2.28x | 2.62x |
| Dict Insertion | 0.192 | 0.175 | 0.224 | 0.92x | 0.78x |
| Set Union (\|) | 47.597 | 127.651 | 15.728 | 2.68x | 8.12x |
| List Concatenation (\|) | 0.372 | 8.038 | 3.174 | 21.58x | 2.53x |
| Loop Iteration (for_in + range) | 0.001 | 4.602 | 44.465 | 3136.80x | 0.10x |
| Loop over Container (for_in) | 0.001 | 0.015 | 0.036 | 10.05x | 0.41x |
| Map (transform) | 0.005 | 0.102 | 0.057 | 21.70x | 1.78x |
| Filter | 0.003 | 0.031 | 0.078 | 10.86x | 0.40x |

## Analysis

- **Average Pythonic C++ vs C++**: 2224.03x
- **Average Pythonic C++ vs Python**: 2.08x
- **Total Benchmarks**: 18
- **Python Benchmarks Available**: 18 / 18

### Best Performance (vs C++)

**Dict Insertion**: 0.92x slower than native C++

### Worst Performance (vs C++)

**String Comparison**: 12759.93x slower than native C++

### Best Performance (vs Python)

**Loop Iteration (for_in + range)**: 0.10x (FASTER than Python!)

### Worst Performance (vs Python)

**Set Union (|)**: 8.12x

## Performance Overview

**Pythonic C++ is slower than Python in these microbenchmarks** (2.08x). Note that real-world performance varies based on usage patterns and compiler optimizations.

**Pythonic C++ vs Native C++**: 2224.03x average overhead for dynamic typing and Python-like syntax.

## Detailed Results

### Arithmetic Operations

- **Integer Addition**: 2625.43x (C++: 0.002ms, Pythonic: 4.949ms)
- **Integer Multiplication**: 5381.60x (C++: 0.001ms, Pythonic: 4.881ms)
- **Double Addition**: 3300.82x (C++: 0.001ms, Pythonic: 4.839ms)
- **Integer Comparison**: 12712.01x (C++: 0.001ms, Pythonic: 12.432ms)
- **String Comparison**: 12759.93x (C++: 0.001ms, Pythonic: 12.479ms)

### String Operations

- **String Concatenation**: 2.84x (C++: 0.231ms, Pythonic: 0.656ms)
- **String Comparison**: 12759.93x (C++: 0.001ms, Pythonic: 12.479ms)

### Container Operations

- **List Creation (10 elements)**: 21.60x (C++: 0.122ms, Pythonic: 2.628ms)
- **Set Creation (10 elements)**: 4.69x (C++: 2.126ms, Pythonic: 9.971ms)
- **List Append**: 7.23x (C++: 0.016ms, Pythonic: 0.117ms)
- **List Access (indexed)**: 9.51x (C++: 0.001ms, Pythonic: 0.013ms)
- **Set Insertion**: 2.28x (C++: 0.057ms, Pythonic: 0.129ms)
- **Dict Insertion**: 0.92x (C++: 0.192ms, Pythonic: 0.175ms)
- **Set Union (|)**: 2.68x (C++: 47.597ms, Pythonic: 127.651ms)
- **List Concatenation (|)**: 21.58x (C++: 0.372ms, Pythonic: 8.038ms)

### Loop Constructs

- **Loop Iteration (for_in + range)**: 3136.80x (C++: 0.001ms, Pythonic: 4.602ms)
- **Loop over Container (for_in)**: 10.05x (C++: 0.001ms, Pythonic: 0.015ms)

### Functional Operations

- **Map (transform)**: 21.70x (C++: 0.005ms, Pythonic: 0.102ms)
- **Filter**: 10.86x (C++: 0.003ms, Pythonic: 0.031ms)

## Interpretation

The Pythonic C++ library provides Python-like syntax at the cost of performance. The overhead comes from:

1. **Type erasure**: Using `std::variant` for dynamic typing
2. **Virtual dispatch**: Pattern matching with `std::visit`
3. **Allocation overhead**: More dynamic allocations than native C++
4. **Wrapper overhead**: Function call overhead for operations

**When to use Pythonic C++**:
- Rapid prototyping where Python-like syntax helps
- Applications where developer productivity > raw performance
- Mixed workloads where convenience matters more than speed

**When to avoid**:
- Performance-critical inner loops
- Real-time systems with strict timing requirements
- High-frequency trading or game engines

