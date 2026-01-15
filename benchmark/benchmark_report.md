# Pythonic C++ Library Benchmark Report

Generated on: Jan 15 2026 23:30:46

## Configuration

- **Iterations (Arithmetic/Comparisons)**: 1000000
- **Small Iterations (Strings/Containers)**: 10000
- **Container Size**: 1000

## Three-Way Comparison

| Operation | C++ (ms) | Pythonic (ms) | Python (ms) | Pythonic vs C++ | Pythonic vs Python |
|-----------|----------|---------------|-------------|-----------------|--------------------|
| Integer Addition | 0.026 | 4.226 | 29.400 | 161.35x | 0.14x |
| Integer Multiplication | 0.001 | 5.226 | 34.831 | 5343.66x | 0.15x |
| Double Addition | 0.001 | 5.072 | 25.851 | 5186.41x | 0.20x |
| Integer Comparison | 0.001 | 12.488 | 42.027 | 13753.54x | 0.30x |
| String Concatenation | 0.210 | 0.805 | 0.161 | 3.84x | 5.01x |
| String Comparison | 0.001 | 13.037 | 21.126 | 13329.94x | 0.62x |
| List Creation (10 elements) | 0.106 | 3.142 | 0.543 | 29.71x | 5.78x |
| Set Creation (10 elements) | 2.182 | 9.629 | 1.328 | 4.41x | 7.25x |
| List Append | 0.022 | 0.130 | 0.067 | 5.81x | 1.93x |
| List Access (indexed) | 0.001 | 0.018 | 0.053 | 13.00x | 0.34x |
| Set Insertion | 0.069 | 0.238 | 0.063 | 3.47x | 3.78x |
| Dict Insertion | 0.247 | 0.202 | 0.281 | 0.82x | 0.72x |
| Set Union (\|) | 53.759 | 181.285 | 17.285 | 3.37x | 10.49x |
| List Concatenation (\|) | 0.363 | 19.681 | 3.203 | 54.26x | 6.14x |
| Loop Iteration (for_in + range) | 0.001 | 4.557 | 44.644 | 4659.17x | 0.10x |
| Loop over Container (for_in) | 0.001 | 0.016 | 0.033 | 16.00x | 0.48x |
| Map (transform) | 0.005 | 0.097 | 0.062 | 20.47x | 1.57x |
| Filter | 0.002 | 0.028 | 0.072 | 11.63x | 0.39x |

## Analysis

- **Average Pythonic C++ vs C++**: 2366.72x
- **Average Pythonic C++ vs Python**: 2.52x
- **Total Benchmarks**: 18
- **Python Benchmarks Available**: 18 / 18

### Best Performance (vs C++)

**Dict Insertion**: 0.82x slower than native C++

### Worst Performance (vs C++)

**Integer Comparison**: 13753.54x slower than native C++

### Best Performance (vs Python)

**Loop Iteration (for_in + range)**: 0.10x (FASTER than Python!)

### Worst Performance (vs Python)

**Set Union (|)**: 10.49x

## Performance Overview

**Pythonic C++ is slower than Python in these microbenchmarks** (2.52x). Note that real-world performance varies based on usage patterns and compiler optimizations.

**Pythonic C++ vs Native C++**: 2366.72x average overhead for dynamic typing and Python-like syntax.

## Detailed Results

### Arithmetic Operations

- **Integer Addition**: 161.35x (C++: 0.026ms, Pythonic: 4.226ms)
- **Integer Multiplication**: 5343.66x (C++: 0.001ms, Pythonic: 5.226ms)
- **Double Addition**: 5186.41x (C++: 0.001ms, Pythonic: 5.072ms)
- **Integer Comparison**: 13753.54x (C++: 0.001ms, Pythonic: 12.488ms)
- **String Comparison**: 13329.94x (C++: 0.001ms, Pythonic: 13.037ms)

### String Operations

- **String Concatenation**: 3.84x (C++: 0.210ms, Pythonic: 0.805ms)
- **String Comparison**: 13329.94x (C++: 0.001ms, Pythonic: 13.037ms)

### Container Operations

- **List Creation (10 elements)**: 29.71x (C++: 0.106ms, Pythonic: 3.142ms)
- **Set Creation (10 elements)**: 4.41x (C++: 2.182ms, Pythonic: 9.629ms)
- **List Append**: 5.81x (C++: 0.022ms, Pythonic: 0.130ms)
- **List Access (indexed)**: 13.00x (C++: 0.001ms, Pythonic: 0.018ms)
- **Set Insertion**: 3.47x (C++: 0.069ms, Pythonic: 0.238ms)
- **Dict Insertion**: 0.82x (C++: 0.247ms, Pythonic: 0.202ms)
- **Set Union (|)**: 3.37x (C++: 53.759ms, Pythonic: 181.285ms)
- **List Concatenation (|)**: 54.26x (C++: 0.363ms, Pythonic: 19.681ms)

### Loop Constructs

- **Loop Iteration (for_in + range)**: 4659.17x (C++: 0.001ms, Pythonic: 4.557ms)
- **Loop over Container (for_in)**: 16.00x (C++: 0.001ms, Pythonic: 0.016ms)

### Functional Operations

- **Map (transform)**: 20.47x (C++: 0.005ms, Pythonic: 0.097ms)
- **Filter**: 11.63x (C++: 0.002ms, Pythonic: 0.028ms)

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

