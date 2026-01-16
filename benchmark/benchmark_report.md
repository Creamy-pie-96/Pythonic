# Pythonic Library Benchmark Report

**Generated:** 2026-01-16 23:13:25

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 53 |
| Slower than Python | 32 |
| No Python Data | 0 |
| Avg Overhead vs Native | 1294.48x |
| Avg Speedup vs Python | 537.87x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.001 | 0.173 | 29.555 | 124.04x | **170.56x faster** |
| Integer Multiplication | 0.001 | 8.901 | 35.638 | 9101.39x | **4.00x faster** |
| Double Addition | 0.001 | 0.204 | 27.479 | 208.17x | **134.97x faster** |
| Integer Division | 0.001 | 0.127 | 25.403 | 139.53x | **200.51x faster** |
| Integer Modulo | 0.001 | 0.127 | 38.278 | 139.76x | **301.63x faster** |
| Integer Comparison | 0.001 | 0.152 | 42.045 | 155.75x | **276.02x faster** |
| String Concatenation | 0.223 | 0.925 | 0.163 | 4.15x | *5.68x slower* |
| String Comparison | 0.001 | 33.345 | 19.513 | 22730.23x | *1.71x slower* |
| String upper() | 0.364 | 0.605 | 0.457 | 1.66x | *1.32x slower* |
| String lower() | 0.364 | 0.565 | 0.462 | 1.55x | *1.22x slower* |
| String strip() | 0.395 | 0.776 | 0.592 | 1.96x | *1.31x slower* |
| String replace() | 0.459 | 1.689 | 1.042 | 3.68x | *1.62x slower* |
| String find() | 0.147 | 0.433 | 0.976 | 2.95x | **2.25x faster** |
| String split() | 3.244 | 6.235 | 1.458 | 1.92x | *4.28x slower* |
| String startswith() | 0.033 | 0.348 | 0.755 | 10.59x | **2.17x faster** |
| String endswith() | 0.001 | 0.368 | 0.733 | 376.71x | **1.99x faster** |
| String isdigit() | 0.001 | 0.010 | 0.398 | 6.81x | **39.86x faster** |
| String center() | 0.711 | 1.069 | 0.757 | 1.50x | *1.41x slower* |
| String zfill() | 0.239 | 0.471 | 0.728 | 1.97x | **1.55x faster** |
| String Slice [2:8] | 0.101 | 0.489 | 0.663 | 4.85x | **1.36x faster** |
| List Slice [2:8] | 0.189 | 1.836 | 0.758 | 9.70x | *2.42x slower* |
| String Slice [::2] | 0.119 | 0.416 | 0.770 | 3.50x | **1.85x faster** |
| List Slice [::2] | 0.044 | 1.543 | 1.503 | 34.74x | *1.03x slower* |
| List Slice [-5:-1] | 0.185 | 1.352 | 0.791 | 7.30x | *1.71x slower* |
| List Slice [::-1] (Reverse) | 0.111 | 2.297 | 2.145 | 20.74x | *1.07x slower* |
| List Creation | 0.151 | 0.763 | 0.632 | 5.05x | *1.21x slower* |
| Dict Creation | 1.298 | 1.551 | 1.293 | 1.20x | *1.20x slower* |
| Set Creation | 1.060 | 2.017 | 1.304 | 1.90x | *1.55x slower* |
| List append() | 0.112 | 0.570 | 0.051 | 5.09x | *11.27x slower* |
| List extend() | 0.338 | 3.392 | 1.373 | 10.02x | *2.47x slower* |
| List Index Access | 0.002 | 1.841 | 0.047 | 941.89x | *39.41x slower* |
| Dict Access | 0.038 | 0.175 | 0.178 | 4.67x | **1.01x faster** |
| Dict keys() | 0.152 | 2.132 | 49.443 | 14.06x | **23.19x faster** |
| Dict values() | 0.104 | 1.013 | 46.730 | 9.70x | **46.15x faster** |
| Dict items() | 0.239 | 6.818 | 207.459 | 28.54x | **30.43x faster** |
| Set add() | 1.107 | 0.826 | 0.067 | 0.75x | *12.24x slower* |
| 'in' Operator (List) | 0.002 | 0.297 | 3.110 | 125.01x | **10.48x faster** |
| 'in' Operator (Set) | 0.009 | 0.162 | 0.033 | 17.21x | *4.86x slower* |
| 'in' Operator (Dict) | 0.027 | 0.413 | 0.037 | 15.32x | *11.03x slower* |
| List + Operator | 0.080 | 0.745 | 3.285 | 9.33x | **4.41x faster** |
| List * Operator | 0.282 | 1.037 | 3.202 | 3.68x | **3.09x faster** |
| range() Iteration | 0.001 | 0.008 | 41.860 | 5.29x | **5399.88x faster** |
| range(start, stop) Iteration | 0.001 | 0.007 | 41.847 | 4.75x | **6307.04x faster** |
| range(start, stop, step) Iteration | 0.001 | 0.004 | 20.873 | 3.93x | **5434.34x faster** |
| for_in with List | 0.005 | 6.851 | 0.028 | 1325.46x | *245.61x slower* |
| enumerate() | 0.003 | 1.422 | 0.067 | 496.61x | *21.07x slower* |
| zip() | 0.001 | 1.095 | 0.054 | 1120.49x | *20.32x slower* |
| Dict Iteration (items()) | 0.003 | 14.117 | 0.039 | 4811.64x | *365.51x slower* |
| map() | 0.637 | 8.571 | 0.054 | 13.45x | *159.78x slower* |
| filter() | 0.701 | 5.521 | 0.064 | 7.87x | *85.76x slower* |
| reduce() | 0.003 | 10.672 | 0.062 | 3117.84x | *170.85x slower* |
| Chained map + filter | 0.998 | 18.314 | 0.106 | 18.36x | *172.62x slower* |
| Lambda Application | 0.001 | 0.183 | 0.692 | 124.64x | **3.79x faster** |
| sorted() Ascending | 0.304 | 3.710 | 45.526 | 12.21x | **12.27x faster** |
| sorted() Descending | 0.287 | 3.555 | 45.588 | 12.39x | **12.82x faster** |
| Large List Sorting (1000 elements) | 0.377 | 3.453 | 51.173 | 9.16x | **14.82x faster** |
| len() | 0.002 | 0.001 | 35.172 | 0.70x | **26505.07x faster** |
| sum() | 0.001 | 0.915 | 51.589 | 936.62x | **56.38x faster** |
| min() | 0.001 | 1.580 | 97.961 | 1617.46x | **61.99x faster** |
| max() | 0.002 | 1.435 | 110.584 | 604.03x | **77.09x faster** |
| abs() | 0.001 | 9.289 | 25.621 | 9498.24x | **2.76x faster** |
| all() | 0.004 | 0.574 | 3.955 | 130.37x | **6.90x faster** |
| any() | 0.001 | 0.328 | 5.065 | 360.91x | **15.46x faster** |
| pow() | 0.001 | 0.159 | 0.909 | 175.22x | **5.72x faster** |
| sqrt() | 0.001 | 0.165 | 0.685 | 181.61x | **4.16x faster** |
| floor() | 0.001 | 0.174 | 0.435 | 191.68x | **2.50x faster** |
| ceil() | 0.000 | 0.161 | 0.475 | 329.79x | **2.95x faster** |
| round() | 0.001 | 0.172 | 3.415 | 175.96x | **19.84x faster** |
| Int() from String | 0.154 | 0.458 | 1.475 | 2.98x | **3.22x faster** |
| Float() from String | 0.781 | 1.301 | 1.124 | 1.67x | *1.16x slower* |
| Str() from Int | 0.094 | 0.387 | 1.123 | 4.12x | **2.90x faster** |
| Bool() from Int | 0.001 | 14.072 | 45.910 | 10072.99x | **3.26x faster** |
| Int to Double | 0.001 | 9.288 | 44.844 | 10228.87x | **4.83x faster** |
| type() | 0.001 | 30.093 | 0.295 | 21541.28x | *101.87x slower* |
| isinstance() | 0.001 | 7.665 | 0.306 | 8442.11x | *25.04x slower* |
| Graph Creation | 0.002 | 0.071 | 5.901 | 36.14x | **83.49x faster** |
| add_edge() | 0.013 | 0.023 | 0.709 | 1.78x | **30.95x faster** |
| DFS Traversal | 0.321 | 0.186 | 5.888 | 0.58x | **31.61x faster** |
| BFS Traversal | 0.311 | 0.217 | 13.551 | 0.70x | **62.53x faster** |
| has_edge() | 0.001 | 0.002 | 0.120 | 1.28x | **63.49x faster** |
| get_shortest_path() | 0.056 | 0.049 | 1.066 | 0.88x | **21.65x faster** |
| is_connected() | 0.219 | 0.082 | 3.285 | 0.37x | **40.20x faster** |
| has_cycle() | 0.001 | 0.077 | 7.902 | 78.41x | **103.04x faster** |
| topological_sort() | 0.001 | 0.014 | 0.613 | 9.52x | **43.88x faster** |
| connected_components() | 0.001 | 0.029 | 0.257 | 32.08x | **8.82x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
