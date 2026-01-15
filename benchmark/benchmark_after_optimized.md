# Pythonic C++ Library Benchmark Report

Generated on: Jan 16 2026 00:41:06

## Configuration

- **Iterations (Arithmetic/Comparisons)**: 1000000
- **Small Iterations (Strings/Containers)**: 10000
- **Container Size**: 1000

## Three-Way Comparison

| Operation | C++ (ms) | Pythonic (ms) | Python (ms) | Pythonic vs C++ | Pythonic vs Python |
|-----------|----------|---------------|-------------|-----------------|--------------------|
| Integer Addition | 0.001 | 6.542 | 303.132 | 4459.68x | 0.02x |
| Integer Multiplication | 0.001 | 7.029 | 370.322 | 7186.80x | 0.02x |
| Double Addition | 0.001 | 6.061 | 264.909 | 4338.66x | 0.02x |
| Integer Comparison | 0.001 | 15.744 | 427.482 | 16098.21x | 0.04x |
| String Concatenation | 0.239 | 0.714 | 1.949 | 2.99x | 0.37x |
| String Comparison | 0.001 | 12.429 | 205.008 | 13688.73x | 0.06x |
| List Creation (10 elements) | 0.105 | 2.632 | 5.887 | 25.01x | 0.45x |
| Set Creation (10 elements) | 2.042 | 9.061 | 13.683 | 4.44x | 0.66x |
| List Append | 0.023 | 0.129 | 0.055 | 5.53x | 2.34x |
| List Access (indexed) | 0.001 | 0.013 | 0.041 | 14.54x | 0.32x |
| Set Insertion | 0.055 | 0.117 | 0.048 | 2.11x | 2.43x |
| Dict Insertion | 0.184 | 0.155 | 0.226 | 0.84x | 0.69x |
| Set Union (\|) | 47.821 | 119.023 | 165.565 | 2.49x | 0.72x |
| List Concatenation (\|) | 0.378 | 8.086 | 33.199 | 21.39x | 0.24x |
| Loop Iteration (for_in + range) | 0.001 | 4.555 | 442.942 | 4657.64x | 0.01x |
| Loop over Container (for_in) | 0.001 | 0.017 | 0.034 | 11.47x | 0.49x |
| Map (transform) | 0.004 | 0.120 | 0.057 | 28.71x | 2.12x |
| Filter | 0.002 | 0.031 | 0.072 | 13.06x | 0.43x |

## Analysis

- **Average Pythonic C++ vs C++**: 2809.02x
- **Average Pythonic C++ vs Python**: 0.64x
- **Total Benchmarks**: 18
- **Python Benchmarks Available**: 18 / 18

### Best Performance (vs C++)

**Dict Insertion**: 0.84x slower than native C++

### Worst Performance (vs C++)

**Integer Comparison**: 16098.21x slower than native C++

### Best Performance (vs Python)

**Loop Iteration (for_in + range)**: 0.01x (FASTER than Python!)

### Worst Performance (vs Python)

**Set Insertion**: 2.43x

## Performance Overview

**Pythonic C++ is on average FASTER than Python** (0.64x), showing that the library provides Python-like syntax while maintaining significant C++ performance advantages.

**Pythonic C++ vs Native C++**: 2809.02x average overhead for dynamic typing and Python-like syntax.

## Detailed Results

### Arithmetic Operations

- **Integer Addition**: 4459.68x (C++: 0.001ms, Pythonic: 6.542ms)
- **Integer Multiplication**: 7186.80x (C++: 0.001ms, Pythonic: 7.029ms)
- **Double Addition**: 4338.66x (C++: 0.001ms, Pythonic: 6.061ms)
- **Integer Comparison**: 16098.21x (C++: 0.001ms, Pythonic: 15.744ms)
- **String Comparison**: 13688.73x (C++: 0.001ms, Pythonic: 12.429ms)

### String Operations

- **String Concatenation**: 2.99x (C++: 0.239ms, Pythonic: 0.714ms)
- **String Comparison**: 13688.73x (C++: 0.001ms, Pythonic: 12.429ms)

### Container Operations

- **List Creation (10 elements)**: 25.01x (C++: 0.105ms, Pythonic: 2.632ms)
- **Set Creation (10 elements)**: 4.44x (C++: 2.042ms, Pythonic: 9.061ms)
- **List Append**: 5.53x (C++: 0.023ms, Pythonic: 0.129ms)
- **List Access (indexed)**: 14.54x (C++: 0.001ms, Pythonic: 0.013ms)
- **Set Insertion**: 2.11x (C++: 0.055ms, Pythonic: 0.117ms)
- **Dict Insertion**: 0.84x (C++: 0.184ms, Pythonic: 0.155ms)
- **Set Union (|)**: 2.49x (C++: 47.821ms, Pythonic: 119.023ms)
- **List Concatenation (|)**: 21.39x (C++: 0.378ms, Pythonic: 8.086ms)

### Loop Constructs

- **Loop Iteration (for_in + range)**: 4657.64x (C++: 0.001ms, Pythonic: 4.555ms)
- **Loop over Container (for_in)**: 11.47x (C++: 0.001ms, Pythonic: 0.017ms)

### Functional Operations

- **Map (transform)**: 28.71x (C++: 0.004ms, Pythonic: 0.120ms)
- **Filter**: 13.06x (C++: 0.002ms, Pythonic: 0.031ms)

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

