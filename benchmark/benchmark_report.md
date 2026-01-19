# Pythonic Library Benchmark Report

**Generated:** 2026-01-19 23:58:59

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 54 |
| Slower than Python | 30 |
| No Python Data | 1 |
| Avg Overhead vs Native | 35682.21x |
| Avg Speedup vs Python | 2280.65x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.000 | 0.125 | 29.555 | 696.48x | **235.74x faster** |
| Integer Multiplication | 0.000 | 8.167 | 35.638 | 204173.02x | **4.36x faster** |
| Double Addition | 0.000 | 0.129 | 27.479 | 4147.19x | **213.74x faster** |
| Integer Division | 0.000 | 0.000 | 25.403 | 6.03x | **140346.87x faster** |
| Integer Modulo | 0.000 | 0.127 | 38.278 | 4111.97x | **300.29x faster** |
| Integer Comparison | 0.000 | 0.128 | 42.045 | 4272.43x | **328.04x faster** |
| String Concatenation | 0.226 | 0.907 | 0.163 | 4.01x | *5.57x slower* |
| String Comparison | 0.000 | 39.843 | 19.513 | 398433.92x | *2.04x slower* |
| String upper() | 0.268 | 0.529 | 0.457 | 1.98x | *1.16x slower* |
| String lower() | 0.276 | 0.500 | 0.462 | 1.81x | *1.08x slower* |
| String strip() | 0.128 | 0.652 | 0.592 | 5.09x | *1.10x slower* |
| String replace() | 0.266 | 1.223 | 1.042 | 4.59x | *1.17x slower* |
| String find() | 0.000 | 0.210 | 0.976 | 2337.64x | **4.64x faster** |
| String split() | 2.737 | 4.573 | 1.458 | 1.67x | *3.14x slower* |
| String startswith() | 0.031 | 0.394 | 0.755 | 12.64x | **1.92x faster** |
| String endswith() | 0.000 | 0.395 | 0.733 | 1794.35x | **1.86x faster** |
| String isdigit() | 0.000 | 0.003 | 0.398 | 22.27x | **147.74x faster** |
| String center() | 0.587 | 0.967 | 0.757 | 1.65x | *1.28x slower* |
| String zfill() | 0.163 | 0.358 | 0.728 | 2.20x | **2.03x faster** |
| String Slice [2:8] | 0.099 | 0.412 | 0.663 | 4.15x | **1.61x faster** |
| List Slice [2:8] | 0.137 | 1.370 | 0.758 | 9.97x | *1.81x slower* |
| String Slice [::2] | 0.082 | 0.406 | 0.770 | 4.92x | **1.90x faster** |
| List Slice [::2] | 0.048 | 1.319 | 1.503 | 27.36x | **1.14x faster** |
| List Slice [-5:-1] | 0.141 | 0.985 | 0.791 | 6.99x | *1.25x slower* |
| List Slice [::-1] (Reverse) | 0.084 | 1.686 | 2.145 | 19.99x | **1.27x faster** |
| List Creation | 0.113 | 0.672 | 0.632 | 5.95x | *1.06x slower* |
| Dict Creation | 1.516 | 1.644 | 1.293 | 1.08x | *1.27x slower* |
| Set Creation | 0.997 | 1.862 | 1.304 | 1.87x | *1.43x slower* |
| List append() | 0.107 | 0.525 | 0.051 | 4.90x | *10.39x slower* |
| List extend() | 0.234 | 2.605 | 1.373 | 11.11x | *1.90x slower* |
| List Index Access | 0.000 | 1.772 | 0.047 | 5352.96x | *37.92x slower* |
| Dict Access | 0.042 | 0.163 | 0.178 | 3.91x | **1.09x faster** |
| Dict keys() | 0.123 | 1.730 | 49.443 | 14.07x | **28.58x faster** |
| Dict values() | 0.094 | 0.770 | 46.730 | 8.16x | **60.69x faster** |
| Dict items() | 0.229 | 6.598 | 207.459 | 28.77x | **31.45x faster** |
| Set add() | 0.776 | 0.650 | 0.067 | 0.84x | *9.64x slower* |
| 'in' Operator (List) | 0.001 | 2.423 | 3.110 | 3451.23x | **1.28x faster** |
| 'in' Operator (Set) | 0.006 | 0.157 | 0.033 | 26.40x | *4.70x slower* |
| 'in' Operator (Dict) | 0.111 | 0.478 | 0.037 | 4.31x | *12.78x slower* |
| List + Operator | 0.107 | 0.738 | 3.285 | 6.92x | **4.45x faster** |
| List * Operator | 0.054 | 1.073 | 3.202 | 20.01x | **2.98x faster** |
| range() Iteration | 0.000 | 0.007 | 41.860 | 233.10x | **5985.97x faster** |
| range(start, stop) Iteration | 0.000 | 0.006 | 41.847 | 158.55x | **6598.43x faster** |
| range(start, stop, step) Iteration | 0.000 | 0.003 | 20.873 | 114.23x | **6090.84x faster** |
| for_each with List | 0.001 | 8.684 | N/A | 11119.54x | No data |
| enumerate() | 0.000 | 2.370 | 0.067 | 5374.97x | *35.13x slower* |
| zip() | 0.001 | 1.102 | 0.054 | 2200.39x | *20.46x slower* |
| Dict Iteration (items()) | 0.002 | 14.633 | 0.039 | 7301.87x | *378.86x slower* |
| map() | 0.589 | 8.420 | 0.054 | 14.30x | *156.97x slower* |
| filter() | 0.590 | 11.501 | 0.064 | 19.48x | *178.67x slower* |
| reduce() | 0.001 | 10.028 | 0.062 | 15404.33x | *160.54x slower* |
| Chained map + filter | 0.652 | 21.369 | 0.106 | 32.76x | *201.41x slower* |
| Lambda Application | 0.000 | 2.604 | 0.692 | 86787.43x | *3.76x slower* |
| sorted() Ascending | 0.231 | 3.908 | 45.526 | 16.90x | **11.65x faster** |
| sorted() Descending | 0.278 | 3.985 | 45.588 | 14.33x | **11.44x faster** |
| Large List Sorting (1000 elements) | 0.459 | 3.516 | 51.173 | 7.67x | **14.55x faster** |
| len() | 0.002 | 2.288 | 35.172 | 1454.77x | **15.37x faster** |
| sum() | 0.000 | 1.071 | 51.589 | 5073.76x | **48.19x faster** |
| min() | 0.000 | 1.617 | 97.961 | 8986.11x | **60.56x faster** |
| max() | 0.000 | 1.638 | 110.584 | 8622.83x | **67.50x faster** |
| abs() | 0.000 | 9.110 | 25.621 | 293876.58x | **2.81x faster** |
| all() | 0.001 | 0.573 | 3.955 | 664.48x | **6.91x faster** |
| any() | 0.000 | 0.356 | 5.065 | 2224.71x | **14.23x faster** |
| pow() | 0.000 | 0.000 | 0.909 | 1.00x | **30310.95x faster** |
| sqrt() | 0.000 | 0.141 | 0.685 | 4709.23x | **4.85x faster** |
| floor() | 0.000 | 0.133 | 0.435 | 4430.03x | **3.27x faster** |
| ceil() | 0.000 | 0.128 | 0.475 | 4268.07x | **3.71x faster** |
| round() | 0.000 | 0.136 | 3.415 | 4540.93x | **25.07x faster** |
| Int() from String | 0.110 | 0.464 | 1.475 | 4.21x | **3.18x faster** |
| Float() from String | 0.795 | 1.366 | 1.124 | 1.72x | *1.22x slower* |
| Str() from Int | 0.034 | 0.333 | 1.123 | 9.81x | **3.37x faster** |
| Bool() from Int | 0.000 | 14.297 | 45.910 | 476574.40x | **3.21x faster** |
| Int to Double | 0.000 | 18.748 | 44.844 | 624943.90x | **2.39x faster** |
| type() | 0.000 | 29.649 | 0.295 | 592986.36x | *100.37x slower* |
| isinstance() | 0.000 | 9.505 | 0.306 | 237632.30x | *31.05x slower* |
| Graph Creation | 0.000 | 0.051 | 5.901 | 460.50x | **115.45x faster** |
| add_edge() | 0.007 | 0.012 | 0.709 | 1.57x | **61.17x faster** |
| DFS Traversal | 0.320 | 0.183 | 5.888 | 0.57x | **32.13x faster** |
| BFS Traversal | 0.310 | 0.191 | 13.551 | 0.62x | **70.89x faster** |
| has_edge() | 0.000 | 0.003 | 0.120 | 6.11x | **45.42x faster** |
| get_shortest_path() | 0.067 | 0.051 | 1.066 | 0.76x | **20.94x faster** |
| is_connected() | 0.233 | 0.083 | 3.285 | 0.35x | **39.72x faster** |
| has_cycle() | 0.000 | 0.068 | 7.902 | 2250.60x | **117.03x faster** |
| topological_sort() | 0.000 | 0.016 | 0.613 | 529.00x | **38.62x faster** |
| connected_components() | 0.000 | 0.027 | 0.257 | 886.00x | **9.66x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
