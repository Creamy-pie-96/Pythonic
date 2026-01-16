# Pythonic Library Benchmark Report

**Generated:** 2026-01-16 17:01:00

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 51 |
| Slower than Python | 30 |
| No Python Data | 4 |
| Avg Overhead vs Native | 2149.95x |
| Avg Speedup vs Python | 288.34x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.001 | 5.879 | 29.555 | 4208.31x | **5.03x faster** |
| Integer Multiplication | 0.000 | 8.732 | 35.638 | 17855.94x | **4.08x faster** |
| Double Addition | 0.000 | 6.847 | 27.479 | 14029.76x | **4.01x faster** |
| Integer Division | 0.001 | 6.072 | 25.403 | 4138.85x | **4.18x faster** |
| Integer Modulo | 0.001 | 6.334 | 38.278 | 6476.15x | **6.04x faster** |
| Integer Comparison | 0.001 | 4.997 | 42.045 | 3577.08x | **8.41x faster** |
| String Concatenation | 0.236 | 0.488 | 0.163 | 2.07x | *3.00x slower* |
| String Comparison | 0.001 | 12.794 | 19.513 | 13081.91x | **1.53x faster** |
| String upper() | 0.323 | 0.484 | 0.457 | 1.50x | *1.06x slower* |
| String lower() | 0.339 | 0.417 | 0.462 | 1.23x | **1.11x faster** |
| String strip() | 0.382 | 0.697 | 0.592 | 1.82x | *1.18x slower* |
| String replace() | 0.428 | 1.028 | 1.042 | 2.40x | **1.01x faster** |
| String find() | 0.084 | 0.247 | 0.976 | 2.95x | **3.95x faster** |
| String split() | 3.076 | 3.977 | 1.458 | 1.29x | *2.73x slower* |
| String startswith() | 0.029 | 0.278 | 0.755 | 9.72x | **2.72x faster** |
| String endswith() | 0.001 | 0.272 | 0.733 | 194.68x | **2.69x faster** |
| String isdigit() | 0.002 | 0.064 | 0.398 | 35.46x | **6.18x faster** |
| String center() | 1.096 | 0.712 | 0.757 | 0.65x | **1.06x faster** |
| String zfill() | 0.241 | 0.346 | 0.728 | 1.44x | **2.11x faster** |
| String Slice [2:8] | 0.116 | 0.299 | 8.000 | 2.58x | **26.78x faster** |
| List Slice [2:8] | 0.138 | 1.831 | 8.000 | 13.23x | **4.37x faster** |
| String Slice [::2] | 0.090 | 0.302 | N/A | 3.38x | No data |
| List Slice [::2] | 0.048 | 1.911 | N/A | 39.76x | No data |
| List Slice [-5:-1] | 0.137 | 1.458 | N/A | 10.63x | No data |
| List Slice [::-1] (Reverse) | 0.115 | 2.871 | N/A | 24.99x | No data |
| List Creation | 0.106 | 0.655 | 0.632 | 6.18x | *1.04x slower* |
| Dict Creation | 1.233 | 1.399 | 1.293 | 1.14x | *1.08x slower* |
| Set Creation | 1.177 | 2.013 | 1.304 | 1.71x | *1.54x slower* |
| List append() | 0.169 | 1.533 | 0.051 | 9.08x | *30.33x slower* |
| List extend() | 0.251 | 9.770 | 1.373 | 38.89x | *7.12x slower* |
| List Index Access | 0.002 | 4.076 | 0.047 | 2160.95x | *87.21x slower* |
| Dict Access | 0.033 | 0.144 | 0.178 | 4.39x | **1.23x faster** |
| Dict keys() | 0.111 | 1.562 | 49.443 | 14.06x | **31.65x faster** |
| Dict values() | 0.079 | 1.298 | 46.730 | 16.50x | **35.99x faster** |
| Dict items() | 0.237 | 5.745 | 207.459 | 24.19x | **36.11x faster** |
| Set add() | 0.888 | 0.664 | 0.067 | 0.75x | *9.84x slower* |
| 'in' Operator (List) | 0.002 | 2.215 | 3.110 | 932.62x | **1.40x faster** |
| 'in' Operator (Set) | 0.008 | 0.167 | 0.033 | 21.87x | *4.99x slower* |
| 'in' Operator (Dict) | 0.021 | 0.339 | 0.037 | 16.16x | *9.05x slower* |
| List + Operator | 0.110 | 0.739 | 3.285 | 6.75x | **4.44x faster** |
| List * Operator | 0.335 | 1.539 | 3.202 | 4.59x | **2.08x faster** |
| range() Iteration | 0.001 | 0.006 | 41.860 | 5.78x | **7399.66x faster** |
| range(start, stop) Iteration | 0.001 | 0.005 | 41.847 | 5.23x | **8811.80x faster** |
| range(start, stop, step) Iteration | 0.001 | 0.003 | 20.873 | 2.40x | **6225.26x faster** |
| for_in with List | 0.003 | 9.892 | 0.028 | 3371.38x | *354.60x slower* |
| enumerate() | 0.002 | 1.373 | 0.067 | 728.13x | *20.34x slower* |
| zip() | 0.003 | 1.488 | 0.054 | 463.05x | *27.61x slower* |
| Dict Iteration (items()) | 0.002 | 11.812 | 0.039 | 6041.69x | *305.81x slower* |
| map() | 0.597 | 23.900 | 0.054 | 40.04x | *445.52x slower* |
| filter() | 0.925 | 31.409 | 0.064 | 33.97x | *487.92x slower* |
| reduce() | 0.002 | 14.247 | 0.062 | 7283.69x | *228.08x slower* |
| Chained map + filter | 0.733 | 60.498 | 0.106 | 82.58x | *570.22x slower* |
| Lambda Application | 0.001 | 5.746 | 0.692 | 5874.78x | *8.30x slower* |
| sorted() Ascending | 0.273 | 5.280 | 45.526 | 19.36x | **8.62x faster** |
| sorted() Descending | 0.332 | 5.259 | 45.588 | 15.86x | **8.67x faster** |
| Large List Sorting (1000 elements) | 0.441 | 5.254 | 51.173 | 11.91x | **9.74x faster** |
| len() | 0.002 | 0.764 | 35.172 | 321.76x | **46.05x faster** |
| sum() | 0.001 | 1.324 | 51.589 | 947.74x | **38.96x faster** |
| min() | 0.001 | 1.844 | 97.961 | 2030.57x | **53.13x faster** |
| max() | 0.001 | 1.732 | 110.584 | 1907.73x | **63.84x faster** |
| abs() | 0.001 | 13.452 | 25.621 | 9175.81x | **1.90x faster** |
| all() | 0.002 | 0.745 | 3.955 | 394.79x | **5.31x faster** |
| any() | 0.002 | 0.458 | 5.065 | 242.78x | **11.06x faster** |
| pow() | 0.001 | 6.256 | 0.909 | 6397.17x | *6.88x slower* |
| sqrt() | 0.001 | 3.207 | 0.685 | 2297.43x | *4.68x slower* |
| floor() | 0.001 | 3.351 | 0.435 | 2398.97x | *7.71x slower* |
| ceil() | 0.001 | 3.676 | 0.475 | 2631.59x | *7.74x slower* |
| round() | 0.001 | 3.428 | 3.415 | 3775.38x | *1.00x slower* |
| Int() from String | 0.110 | 0.498 | 1.475 | 4.55x | **2.96x faster** |
| Float() from String | 0.866 | 1.324 | 1.124 | 1.53x | *1.18x slower* |
| Str() from Int | 0.117 | 0.240 | 1.123 | 2.05x | **4.67x faster** |
| Bool() from Int | 0.001 | 13.758 | 45.910 | 14067.63x | **3.34x faster** |
| Int to Double | 0.001 | 14.362 | 44.844 | 14684.56x | **3.12x faster** |
| type() | 0.001 | 18.070 | 0.295 | 19900.76x | *61.17x slower* |
| isinstance() | 0.001 | 10.229 | 0.306 | 10458.92x | *33.41x slower* |
| Graph Creation | 0.001 | 0.054 | 5.901 | 36.59x | **110.02x faster** |
| add_edge() | 0.021 | 0.032 | 0.709 | 1.52x | **21.88x faster** |
| DFS Traversal | 0.331 | 0.185 | 5.888 | 0.56x | **31.87x faster** |
| BFS Traversal | 0.291 | 0.197 | 13.551 | 0.68x | **68.83x faster** |
| has_edge() | 0.001 | 0.007 | 0.120 | 7.85x | **16.80x faster** |
| get_shortest_path() | 0.057 | 0.049 | 1.066 | 0.86x | **21.68x faster** |
| is_connected() | 0.213 | 0.077 | 3.285 | 0.36x | **42.56x faster** |
| has_cycle() | 0.001 | 0.084 | 7.902 | 57.32x | **93.97x faster** |
| topological_sort() | 0.001 | 0.018 | 0.613 | 19.54x | **34.55x faster** |
| connected_components() | 0.001 | 0.030 | 0.257 | 21.15x | **8.69x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
