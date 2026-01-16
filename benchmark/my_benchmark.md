# Pythonic Library Benchmark Report

**Generated:** 2026-01-16 17:04:47

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 50 |
| Slower than Python | 35 |
| No Python Data | 0 |
| Avg Overhead vs Native | 1991.76x |
| Avg Speedup vs Python | 247.88x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.001 | 6.882 | 29.555 | 4926.17x | **4.29x faster** |
| Integer Multiplication | 0.001 | 9.079 | 35.638 | 6188.78x | **3.93x faster** |
| Double Addition | 0.001 | 6.376 | 27.479 | 7022.56x | **4.31x faster** |
| Integer Division | 0.001 | 6.361 | 25.403 | 4553.56x | **3.99x faster** |
| Integer Modulo | 0.001 | 7.082 | 38.278 | 7241.26x | **5.41x faster** |
| Integer Comparison | 0.001 | 4.944 | 42.045 | 5060.20x | **8.50x faster** |
| String Concatenation | 0.231 | 0.482 | 0.163 | 2.08x | *2.96x slower* |
| String Comparison | 0.001 | 12.804 | 19.513 | 13091.76x | **1.52x faster** |
| String upper() | 0.351 | 0.550 | 0.457 | 1.57x | *1.20x slower* |
| String lower() | 0.353 | 0.466 | 0.462 | 1.32x | *1.01x slower* |
| String strip() | 0.375 | 0.706 | 0.592 | 1.88x | *1.19x slower* |
| String replace() | 0.435 | 0.993 | 1.042 | 2.28x | **1.05x faster** |
| String find() | 0.082 | 0.251 | 0.976 | 3.07x | **3.88x faster** |
| String split() | 2.981 | 3.843 | 1.458 | 1.29x | *2.64x slower* |
| String startswith() | 0.029 | 0.268 | 0.755 | 9.24x | **2.81x faster** |
| String endswith() | 0.001 | 0.282 | 0.733 | 310.60x | **2.60x faster** |
| String isdigit() | 0.001 | 0.084 | 0.398 | 60.34x | **4.72x faster** |
| String center() | 0.921 | 0.751 | 0.757 | 0.82x | **1.01x faster** |
| String zfill() | 0.265 | 0.373 | 0.728 | 1.41x | **1.95x faster** |
| String Slice [2:8] | 0.082 | 0.251 | 0.663 | 3.08x | **2.64x faster** |
| List Slice [2:8] | 0.182 | 1.735 | 0.758 | 9.55x | *2.29x slower* |
| String Slice [::2] | 0.089 | 0.270 | 0.770 | 3.03x | **2.85x faster** |
| List Slice [::2] | 0.035 | 1.692 | 1.503 | 48.27x | *1.13x slower* |
| List Slice [-5:-1] | 0.144 | 1.282 | 0.791 | 8.89x | *1.62x slower* |
| List Slice [::-1] (Reverse) | 0.060 | 2.817 | 2.145 | 47.23x | *1.31x slower* |
| List Creation | 0.128 | 0.676 | 0.632 | 5.29x | *1.07x slower* |
| Dict Creation | 1.229 | 1.329 | 1.293 | 1.08x | *1.03x slower* |
| Set Creation | 1.005 | 2.046 | 1.304 | 2.04x | *1.57x slower* |
| List append() | 0.137 | 1.611 | 0.051 | 11.79x | *31.88x slower* |
| List extend() | 0.239 | 9.238 | 1.373 | 38.60x | *6.73x slower* |
| List Index Access | 0.001 | 4.168 | 0.047 | 2983.40x | *89.19x slower* |
| Dict Access | 0.033 | 0.136 | 0.178 | 4.14x | **1.31x faster** |
| Dict keys() | 0.111 | 1.402 | 49.443 | 12.65x | **35.26x faster** |
| Dict values() | 0.110 | 1.207 | 46.730 | 10.94x | **38.71x faster** |
| Dict items() | 0.235 | 6.050 | 207.459 | 25.73x | **34.29x faster** |
| Set add() | 0.876 | 0.680 | 0.067 | 0.78x | *10.08x slower* |
| 'in' Operator (List) | 0.002 | 2.235 | 3.110 | 941.00x | **1.39x faster** |
| 'in' Operator (Set) | 0.008 | 0.179 | 0.033 | 21.75x | *5.37x slower* |
| 'in' Operator (Dict) | 0.021 | 0.343 | 0.037 | 16.41x | *9.16x slower* |
| List + Operator | 0.098 | 0.744 | 3.285 | 7.62x | **4.42x faster** |
| List * Operator | 0.287 | 1.727 | 3.202 | 6.02x | **1.85x faster** |
| range() Iteration | 0.001 | 0.007 | 41.860 | 5.10x | **5875.89x faster** |
| range(start, stop) Iteration | 0.001 | 0.005 | 41.847 | 5.43x | **7883.80x faster** |
| range(start, stop, step) Iteration | 0.001 | 0.003 | 20.873 | 2.24x | **6358.00x faster** |
| for_in with List | 0.002 | 10.012 | 0.028 | 5311.38x | *358.92x slower* |
| enumerate() | 0.001 | 1.377 | 0.067 | 1409.42x | *20.41x slower* |
| zip() | 0.003 | 1.478 | 0.054 | 440.95x | *27.43x slower* |
| Dict Iteration (items()) | 0.003 | 11.914 | 0.039 | 3709.07x | *308.45x slower* |
| map() | 0.614 | 24.387 | 0.054 | 39.74x | *454.61x slower* |
| filter() | 0.591 | 31.191 | 0.064 | 52.82x | *484.54x slower* |
| reduce() | 0.002 | 14.850 | 0.062 | 6252.62x | *237.73x slower* |
| Chained map + filter | 0.677 | 55.765 | 0.106 | 82.42x | *525.60x slower* |
| Lambda Application | 0.001 | 5.593 | 0.692 | 4003.63x | *8.08x slower* |
| sorted() Ascending | 0.285 | 5.327 | 45.526 | 18.69x | **8.55x faster** |
| sorted() Descending | 0.297 | 5.276 | 45.588 | 17.74x | **8.64x faster** |
| Large List Sorting (1000 elements) | 0.377 | 5.166 | 51.173 | 13.69x | **9.90x faster** |
| len() | 0.001 | 0.762 | 35.172 | 780.27x | **46.14x faster** |
| sum() | 0.001 | 1.144 | 51.589 | 819.20x | **45.08x faster** |
| min() | 0.001 | 1.973 | 97.961 | 2017.41x | **49.65x faster** |
| max() | 0.001 | 1.842 | 110.584 | 1255.63x | **60.03x faster** |
| abs() | 0.001 | 14.327 | 25.621 | 14648.91x | **1.79x faster** |
| all() | 0.001 | 0.676 | 3.955 | 461.21x | **5.85x faster** |
| any() | 0.001 | 0.370 | 5.065 | 264.77x | **13.69x faster** |
| pow() | 0.001 | 6.835 | 0.909 | 7527.60x | *7.52x slower* |
| sqrt() | 0.001 | 3.036 | 0.685 | 2173.39x | *4.43x slower* |
| floor() | 0.001 | 3.534 | 0.435 | 3617.47x | *8.13x slower* |
| ceil() | 0.001 | 3.546 | 0.475 | 2416.90x | *7.47x slower* |
| round() | 0.001 | 3.547 | 3.415 | 2538.70x | *1.04x slower* |
| Int() from String | 0.110 | 0.464 | 1.475 | 4.23x | **3.18x faster** |
| Float() from String | 0.797 | 1.295 | 1.124 | 1.63x | *1.15x slower* |
| Str() from Int | 0.117 | 0.258 | 1.123 | 2.21x | **4.35x faster** |
| Bool() from Int | 0.001 | 13.405 | 45.910 | 13706.91x | **3.42x faster** |
| Int to Double | 0.001 | 14.161 | 44.844 | 15613.44x | **3.17x faster** |
| type() | 0.001 | 20.613 | 0.295 | 21076.64x | *69.78x slower* |
| isinstance() | 0.001 | 9.058 | 0.306 | 6174.78x | *29.59x slower* |
| Graph Creation | 0.001 | 0.044 | 5.901 | 45.35x | **133.06x faster** |
| add_edge() | 0.013 | 0.024 | 0.709 | 1.84x | **30.13x faster** |
| DFS Traversal | 0.328 | 0.178 | 5.888 | 0.54x | **33.07x faster** |
| BFS Traversal | 0.294 | 0.203 | 13.551 | 0.69x | **66.63x faster** |
| has_edge() | 0.001 | 0.006 | 0.120 | 3.86x | **21.16x faster** |
| get_shortest_path() | 0.054 | 0.049 | 1.066 | 0.91x | **21.93x faster** |
| is_connected() | 0.198 | 0.074 | 3.285 | 0.37x | **44.66x faster** |
| has_cycle() | 0.001 | 0.068 | 7.902 | 48.79x | **115.92x faster** |
| topological_sort() | 0.001 | 0.022 | 0.613 | 14.95x | **27.95x faster** |
| connected_components() | 0.001 | 0.026 | 0.257 | 26.88x | **9.78x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
