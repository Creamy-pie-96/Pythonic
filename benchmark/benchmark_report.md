# Pythonic C++ Library Benchmark Report

Generated on: Jan 15 2026 22:26:19

## Configuration

- **Iterations (Arithmetic/Comparisons)**: 1000000
- **Small Iterations (Strings/Containers)**: 10000
- **Container Size**: 1000

## Three-Way Comparison

| Operation | C++ (ms) | Pythonic (ms) | Python (ms) | Pythonic vs C++ | Pythonic vs Python |
|-----------|----------|---------------|-------------|-----------------|--------------------|
| Integer Addition | 0.001 | 6.184 | 32.418 | 4426.57x | 0.19x |
| Integer Multiplication | 0.001 | 5.475 | 36.536 | 3734.72x | 0.15x |
| Double Addition | 0.001 | 6.144 | 25.107 | 4398.17x | 0.24x |
| Integer Comparison | 0.001 | 10.542 | 42.404 | 7546.14x | 0.25x |
| String Concatenation | 0.243 | 0.720 | 0.165 | 2.96x | 4.35x |
| String Comparison | 0.001 | 19.773 | 20.549 | 13478.38x | 0.96x |
| List Creation (10 elements) | 0.143 | 3.446 | 0.546 | 24.03x | 6.32x |
| Set Creation (10 elements) | 2.226 | 8.521 | 1.389 | 3.83x | 6.14x |
| List Append | 0.020 | 0.097 | 0.054 | 4.86x | 1.81x |
| List Access (indexed) | 0.001 | 0.008 | 0.048 | 8.28x | 0.17x |
| Set Insertion | 0.051 | 0.112 | 0.075 | 2.18x | 1.50x |
| Dict Insertion | 0.217 | 0.120 | 0.227 | 0.55x | 0.53x |
| Set Union (\|) | 49.680 | 131.786 | 16.983 | 2.65x | 7.76x |
| List Concatenation (\|) | 0.448 | 26.931 | 3.225 | 60.10x | 8.35x |
| Loop Iteration (for_in + range) | 0.001 | 7.076 | 44.115 | 5064.79x | 0.16x |
| Loop over Container (for_in) | 0.002 | 0.014 | 0.033 | 7.22x | 0.43x |
| Map (transform) | 0.004 | 0.209 | 0.056 | 49.17x | 3.75x |
| Filter | 0.004 | 0.025 | 0.074 | 6.03x | 0.34x |

## Analysis

- **Average Pythonic C++ vs C++**: 2156.70x
- **Average Pythonic C++ vs Python**: 2.41x
- **Total Benchmarks**: 18
- **Python Benchmarks Available**: 18 / 18

### Best Performance (vs C++)

**Dict Insertion**: 0.55x slower than native C++

### Worst Performance (vs C++)

**String Comparison**: 13478.38x slower than native C++

### Best Performance (vs Python)

**Integer Multiplication**: 0.15x (FASTER than Python!)

### Worst Performance (vs Python)

**List Concatenation (|)**: 8.35x

## Performance Overview

**Pythonic C++ is slower than Python in these microbenchmarks** (2.41x). Note that real-world performance varies based on usage patterns and compiler optimizations.

**Pythonic C++ vs Native C++**: 2156.70x average overhead for dynamic typing and Python-like syntax.

## Detailed Results

### Arithmetic Operations

- **Integer Addition**: 4426.57x (C++: 0.001ms, Pythonic: 6.184ms)
- **Integer Multiplication**: 3734.72x (C++: 0.001ms, Pythonic: 5.475ms)
- **Double Addition**: 4398.17x (C++: 0.001ms, Pythonic: 6.144ms)
- **Integer Comparison**: 7546.14x (C++: 0.001ms, Pythonic: 10.542ms)
- **String Comparison**: 13478.38x (C++: 0.001ms, Pythonic: 19.773ms)

### String Operations

- **String Concatenation**: 2.96x (C++: 0.243ms, Pythonic: 0.720ms)
- **String Comparison**: 13478.38x (C++: 0.001ms, Pythonic: 19.773ms)

### Container Operations

- **List Creation (10 elements)**: 24.03x (C++: 0.143ms, Pythonic: 3.446ms)
- **Set Creation (10 elements)**: 3.83x (C++: 2.226ms, Pythonic: 8.521ms)
- **List Append**: 4.86x (C++: 0.020ms, Pythonic: 0.097ms)
- **List Access (indexed)**: 8.28x (C++: 0.001ms, Pythonic: 0.008ms)
- **Set Insertion**: 2.18x (C++: 0.051ms, Pythonic: 0.112ms)
- **Dict Insertion**: 0.55x (C++: 0.217ms, Pythonic: 0.120ms)
- **Set Union (|)**: 2.65x (C++: 49.680ms, Pythonic: 131.786ms)
- **List Concatenation (|)**: 60.10x (C++: 0.448ms, Pythonic: 26.931ms)

### Loop Constructs

- **Loop Iteration (for_in + range)**: 5064.79x (C++: 0.001ms, Pythonic: 7.076ms)
- **Loop over Container (for_in)**: 7.22x (C++: 0.002ms, Pythonic: 0.014ms)

### Functional Operations

- **Map (transform)**: 49.17x (C++: 0.004ms, Pythonic: 0.209ms)
- **Filter**: 6.03x (C++: 0.004ms, Pythonic: 0.025ms)

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

