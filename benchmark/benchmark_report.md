# Pythonic Library Benchmark Report

**Generated:** 2026-01-20 00:27:21

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 56 |
| Slower than Python | 28 |
| No Python Data | 1 |
| Avg Overhead vs Native | 9653.77x |
| Avg Speedup vs Python | 21158.85x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.000 | 0.133 | 29.555 | 739.52x | **222.03x faster** |
| Integer Multiplication | 0.000 | 8.078 | 35.638 | 269276.00x | **4.41x faster** |
| Double Addition | 0.000 | 0.130 | 27.479 | 4339.93x | **211.06x faster** |
| Integer Division | 0.000 | 0.000 | 25.403 | 4.03x | **209940.37x faster** |
| Integer Modulo | 0.000 | 0.125 | 38.278 | 4176.30x | **305.52x faster** |
| Integer Comparison | 0.000 | 0.125 | 42.045 | 4182.63x | **335.08x faster** |
| String Concatenation | 0.226 | 0.892 | 0.163 | 3.95x | *5.48x slower* |
| String Comparison | 0.000 | 38.942 | 19.513 | 354020.31x | *2.00x slower* |
| String upper() | 0.264 | 0.496 | 0.457 | 1.88x | *1.08x slower* |
| String lower() | 0.291 | 0.532 | 0.462 | 1.83x | *1.15x slower* |
| String strip() | 0.132 | 0.643 | 0.592 | 4.86x | *1.09x slower* |
| String replace() | 0.274 | 1.243 | 1.042 | 4.53x | *1.19x slower* |
| String find() | 0.000 | 0.206 | 0.976 | 2284.90x | **4.75x faster** |
| String split() | 2.835 | 4.684 | 1.458 | 1.65x | *3.21x slower* |
| String startswith() | 0.037 | 0.392 | 0.755 | 10.63x | **1.93x faster** |
| String endswith() | 0.000 | 0.383 | 0.733 | 2738.99x | **1.91x faster** |
| String isdigit() | 0.000 | 0.003 | 0.398 | 21.73x | **140.94x faster** |
| String center() | 0.592 | 0.967 | 0.757 | 1.63x | *1.28x slower* |
| String zfill() | 0.159 | 0.376 | 0.728 | 2.37x | **1.94x faster** |
| String Slice [2:8] | 0.089 | 0.399 | 0.663 | 4.51x | **1.66x faster** |
| List Slice [2:8] | 0.154 | 1.429 | 0.758 | 9.28x | *1.88x slower* |
| String Slice [::2] | 0.100 | 0.387 | 0.770 | 3.88x | **1.99x faster** |
| List Slice [::2] | 0.035 | 1.324 | 1.503 | 37.36x | **1.14x faster** |
| List Slice [-5:-1] | 0.152 | 1.033 | 0.791 | 6.79x | *1.31x slower* |
| List Slice [::-1] (Reverse) | 0.083 | 1.737 | 2.145 | 20.89x | **1.24x faster** |
| List Creation | 0.161 | 0.814 | 0.632 | 5.05x | *1.29x slower* |
| Dict Creation | 1.490 | 1.630 | 1.293 | 1.09x | *1.26x slower* |
| Set Creation | 1.043 | 1.856 | 1.304 | 1.78x | *1.42x slower* |
| List append() | 0.105 | 0.530 | 0.051 | 5.04x | *10.48x slower* |
| List extend() | 0.277 | 2.469 | 1.373 | 8.90x | *1.80x slower* |
| List Index Access | 0.000 | 1.835 | 0.047 | 5082.29x | *39.26x slower* |
| Dict Access | 0.038 | 0.236 | 0.178 | 6.24x | *1.32x slower* |
| Dict keys() | 0.149 | 1.830 | 49.443 | 12.29x | **27.02x faster** |
| Dict values() | 0.087 | 0.850 | 46.730 | 9.82x | **54.97x faster** |
| Dict items() | 0.229 | 2.052 | 207.459 | 8.98x | **101.12x faster** |
| Set add() | 0.774 | 0.622 | 0.067 | 0.80x | *9.22x slower* |
| 'in' Operator (List) | 0.001 | 2.411 | 3.110 | 3253.82x | **1.29x faster** |
| 'in' Operator (Set) | 0.008 | 0.123 | 0.033 | 15.42x | *3.69x slower* |
| 'in' Operator (Dict) | 0.087 | 0.487 | 0.037 | 5.59x | *13.01x slower* |
| List + Operator | 0.086 | 0.785 | 3.285 | 9.11x | **4.18x faster** |
| List * Operator | 0.053 | 1.022 | 3.202 | 19.23x | **3.13x faster** |
| range() Iteration | 0.000 | 0.007 | 41.860 | 168.82x | **6198.71x faster** |
| range(start, stop) Iteration | 0.000 | 0.006 | 41.847 | 200.03x | **6973.38x faster** |
| range(start, stop, step) Iteration | 0.000 | 0.003 | 20.873 | 164.85x | **6331.00x faster** |
| for_each with List | 0.001 | 8.655 | N/A | 10415.12x | No data |
| enumerate() | 0.000 | 2.296 | 0.067 | 4675.47x | *34.02x slower* |
| zip() | 0.000 | 1.091 | 0.054 | 4935.67x | *20.24x slower* |
| Dict Iteration (items()) | 0.001 | 6.768 | 0.039 | 5238.02x | *175.22x slower* |
| map() | 0.575 | 8.394 | 0.054 | 14.59x | *156.47x slower* |
| filter() | 0.521 | 12.003 | 0.064 | 23.04x | *186.46x slower* |
| reduce() | 0.001 | 10.977 | 0.062 | 11778.32x | *175.73x slower* |
| Chained map + filter | 0.652 | 21.904 | 0.106 | 33.58x | *206.46x slower* |
| Lambda Application | 0.000 | 2.586 | 0.692 | 64637.65x | *3.74x slower* |
| sorted() Ascending | 0.271 | 3.847 | 45.526 | 14.21x | **11.83x faster** |
| sorted() Descending | 0.238 | 3.558 | 45.588 | 14.97x | **12.81x faster** |
| Large List Sorting (1000 elements) | 0.448 | 3.653 | 51.173 | 8.16x | **14.01x faster** |
| len() | 0.000 | 1.763 | 35.172 | 5668.66x | **19.95x faster** |
| sum() | 0.000 | 0.916 | 51.589 | 13086.01x | **56.32x faster** |
| min() | 0.000 | 1.626 | 97.961 | 13547.26x | **60.26x faster** |
| max() | 0.002 | 1.655 | 110.584 | 917.57x | **66.81x faster** |
| abs() | 0.000 | 0.133 | 25.621 | 4446.77x | **192.05x faster** |
| all() | 0.002 | 0.515 | 3.955 | 304.15x | **7.68x faster** |
| any() | 0.000 | 0.362 | 5.065 | 2010.04x | **14.00x faster** |
| pow() | 0.000 | 0.000 | 0.909 | 1.00x | **30310.95x faster** |
| sqrt() | 0.000 | 0.139 | 0.685 | 4630.13x | **4.93x faster** |
| floor() | 0.000 | 0.140 | 0.435 | 4665.87x | **3.11x faster** |
| ceil() | 0.000 | 0.140 | 0.475 | 4659.87x | **3.40x faster** |
| round() | 0.000 | 0.140 | 3.415 | 4660.87x | **24.42x faster** |
| Int() from String | 0.117 | 0.123 | 1.475 | 1.05x | **11.98x faster** |
| Float() from String | 0.777 | 0.865 | 1.124 | 1.11x | **1.30x faster** |
| Str() from Int | 0.030 | 0.316 | 1.123 | 10.64x | **3.55x faster** |
| Bool() from Int | 0.000 | 0.146 | 45.910 | 4877.27x | **313.77x faster** |
| Int to Double | 0.000 | 0.000 | 44.844 | 1.50x | **1494789.12x faster** |
| type() | 0.000 | 0.000 | 0.295 | 0.75x | **9846.69x faster** |
| isinstance() | 0.000 | 0.000 | 0.306 | 1.00x | **10204.32x faster** |
| Graph Creation | 0.000 | 0.057 | 5.901 | 177.69x | **103.46x faster** |
| add_edge() | 0.016 | 0.022 | 0.709 | 1.34x | **32.72x faster** |
| DFS Traversal | 0.318 | 0.217 | 5.888 | 0.68x | **27.12x faster** |
| BFS Traversal | 0.325 | 0.189 | 13.551 | 0.58x | **71.75x faster** |
| has_edge() | 0.001 | 0.003 | 0.120 | 5.04x | **47.40x faster** |
| get_shortest_path() | 0.051 | 0.052 | 1.066 | 1.03x | **20.50x faster** |
| is_connected() | 0.216 | 0.074 | 3.285 | 0.34x | **44.65x faster** |
| has_cycle() | 0.000 | 0.079 | 7.902 | 2629.67x | **100.16x faster** |
| topological_sort() | 0.000 | 0.020 | 0.613 | 650.93x | **31.39x faster** |
| connected_components() | 0.000 | 0.028 | 0.257 | 943.47x | **9.07x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
