# Pythonic Library Benchmark Report

**Generated:** 2026-01-16 20:02:00

This benchmark compares:
- **Native C++**: Direct C++ STL operations
- **Pythonic**: The Pythonic library's `var` type
- **Python**: Native Python 3 (when available)

## Summary

| Metric | Value |
|--------|-------|
| Total Benchmarks | 85 |
| Faster than Python | 52 |
| Slower than Python | 33 |
| No Python Data | 0 |
| Avg Overhead vs Native | 2086.65x |
| Avg Speedup vs Python | 254.26x |

## Detailed Results

| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |
|-----------|------------|----------|--------|-----------|----------|
| Integer Addition | 0.001 | 6.400 | 29.555 | 4365.69x | **4.62x faster** |
| Integer Multiplication | 0.001 | 9.420 | 35.638 | 9632.23x | **3.78x faster** |
| Double Addition | 0.001 | 6.481 | 27.479 | 4642.82x | **4.24x faster** |
| Integer Division | 0.001 | 6.363 | 25.403 | 4558.33x | **3.99x faster** |
| Integer Modulo | 0.001 | 6.363 | 38.278 | 6506.08x | **6.02x faster** |
| Integer Comparison | 0.001 | 5.113 | 42.045 | 5227.87x | **8.22x faster** |
| String Concatenation | 0.230 | 0.444 | 0.163 | 1.93x | *2.73x slower* |
| String Comparison | 0.001 | 12.456 | 19.513 | 13717.89x | **1.57x faster** |
| String upper() | 0.322 | 0.406 | 0.457 | 1.26x | **1.12x faster** |
| String lower() | 0.308 | 0.398 | 0.462 | 1.29x | **1.16x faster** |
| String strip() | 0.330 | 0.693 | 0.592 | 2.10x | *1.17x slower* |
| String replace() | 0.397 | 0.995 | 1.042 | 2.50x | **1.05x faster** |
| String find() | 0.082 | 0.221 | 0.976 | 2.70x | **4.42x faster** |
| String split() | 2.880 | 3.930 | 1.458 | 1.36x | *2.70x slower* |
| String startswith() | 0.032 | 0.193 | 0.755 | 6.03x | **3.92x faster** |
| String endswith() | 0.002 | 0.141 | 0.733 | 74.80x | **5.20x faster** |
| String isdigit() | 0.001 | 0.070 | 0.398 | 47.75x | **5.68x faster** |
| String center() | 0.711 | 0.688 | 0.757 | 0.97x | **1.10x faster** |
| String zfill() | 0.178 | 0.300 | 0.728 | 1.68x | **2.43x faster** |
| String Slice [2:8] | 0.099 | 0.405 | 0.663 | 4.09x | **1.63x faster** |
| List Slice [2:8] | 0.144 | 1.767 | 0.758 | 12.23x | *2.33x slower* |
| String Slice [::2] | 0.081 | 0.256 | 0.770 | 3.14x | **3.01x faster** |
| List Slice [::2] | 0.037 | 1.741 | 1.503 | 46.76x | *1.16x slower* |
| List Slice [-5:-1] | 0.145 | 1.322 | 0.791 | 9.11x | *1.67x slower* |
| List Slice [::-1] (Reverse) | 0.063 | 2.707 | 2.145 | 43.21x | *1.26x slower* |
| List Creation | 0.106 | 0.643 | 0.632 | 6.07x | *1.02x slower* |
| Dict Creation | 1.285 | 1.399 | 1.293 | 1.09x | *1.08x slower* |
| Set Creation | 0.994 | 1.870 | 1.304 | 1.88x | *1.43x slower* |
| List append() | 0.082 | 1.429 | 0.051 | 17.34x | *28.27x slower* |
| List extend() | 0.247 | 8.392 | 1.373 | 34.02x | *6.11x slower* |
| List Index Access | 0.002 | 3.984 | 0.047 | 2112.44x | *85.26x slower* |
| Dict Access | 0.035 | 0.135 | 0.178 | 3.84x | **1.31x faster** |
| Dict keys() | 0.148 | 1.541 | 49.443 | 10.42x | **32.08x faster** |
| Dict values() | 0.123 | 1.218 | 46.730 | 9.94x | **38.36x faster** |
| Dict items() | 0.245 | 5.264 | 207.459 | 21.47x | **39.41x faster** |
| Set add() | 0.787 | 0.679 | 0.067 | 0.86x | *10.06x slower* |
| 'in' Operator (List) | 0.003 | 2.201 | 3.110 | 670.69x | **1.41x faster** |
| 'in' Operator (Set) | 0.007 | 0.165 | 0.033 | 24.87x | *4.94x slower* |
| 'in' Operator (Dict) | 0.021 | 0.343 | 0.037 | 16.11x | *9.17x slower* |
| List + Operator | 0.080 | 0.779 | 3.285 | 9.79x | **4.22x faster** |
| List * Operator | 0.317 | 1.660 | 3.202 | 5.24x | **1.93x faster** |
| range() Iteration | 0.001 | 0.006 | 41.860 | 3.86x | **7400.97x faster** |
| range(start, stop) Iteration | 0.001 | 0.005 | 41.847 | 5.50x | **7781.19x faster** |
| range(start, stop, step) Iteration | 0.001 | 0.004 | 20.873 | 2.75x | **5434.34x faster** |
| for_in with List | 0.001 | 9.638 | 0.028 | 9864.39x | *345.49x slower* |
| enumerate() | 0.001 | 1.037 | 0.067 | 742.47x | *15.37x slower* |
| zip() | 0.001 | 1.492 | 0.054 | 1016.75x | *27.68x slower* |
| Dict Iteration (items()) | 0.002 | 11.520 | 0.039 | 6111.30x | *298.26x slower* |
| map() | 0.581 | 11.008 | 0.054 | 18.95x | *205.19x slower* |
| filter() | 0.640 | 17.804 | 0.064 | 27.84x | *276.57x slower* |
| reduce() | 0.001 | 15.112 | 0.062 | 10824.94x | *241.92x slower* |
| Chained map + filter | 1.007 | 30.910 | 0.106 | 30.70x | *291.34x slower* |
| Lambda Application | 0.001 | 8.401 | 0.692 | 9251.70x | *12.14x slower* |
| sorted() Ascending | 0.296 | 5.234 | 45.526 | 17.70x | **8.70x faster** |
| sorted() Descending | 0.332 | 5.223 | 45.588 | 15.73x | **8.73x faster** |
| Large List Sorting (1000 elements) | 0.389 | 5.218 | 51.173 | 13.40x | **9.81x faster** |
| len() | 0.001 | 0.763 | 35.172 | 780.09x | **46.10x faster** |
| sum() | 0.001 | 1.278 | 51.589 | 1307.75x | **40.38x faster** |
| min() | 0.001 | 1.874 | 97.961 | 1915.78x | **52.28x faster** |
| max() | 0.001 | 1.944 | 110.584 | 1990.15x | **56.87x faster** |
| abs() | 0.001 | 12.561 | 25.621 | 12843.57x | **2.04x faster** |
| all() | 0.001 | 0.714 | 3.955 | 486.83x | **5.54x faster** |
| any() | 0.001 | 0.407 | 5.065 | 448.80x | **12.44x faster** |
| pow() | 0.001 | 6.345 | 0.909 | 4542.03x | *6.98x slower* |
| sqrt() | 0.001 | 5.101 | 0.685 | 3651.44x | *7.44x slower* |
| floor() | 0.001 | 4.865 | 0.435 | 3484.72x | *11.19x slower* |
| ceil() | 0.001 | 5.327 | 0.475 | 5872.94x | *11.22x slower* |
| round() | 0.001 | 4.834 | 3.415 | 5323.65x | *1.42x slower* |
| Int() from String | 0.109 | 0.485 | 1.475 | 4.43x | **3.05x faster** |
| Float() from String | 0.767 | 1.251 | 1.124 | 1.63x | *1.11x slower* |
| Str() from Int | 0.092 | 0.261 | 1.123 | 2.85x | **4.31x faster** |
| Bool() from Int | 0.001 | 13.352 | 45.910 | 9564.73x | **3.44x faster** |
| Int to Double | 0.001 | 12.862 | 44.844 | 9206.67x | **3.49x faster** |
| type() | 0.001 | 16.935 | 0.295 | 17315.63x | *57.33x slower* |
| isinstance() | 0.001 | 8.462 | 0.306 | 8661.28x | *27.64x slower* |
| Graph Creation | 0.001 | 0.043 | 5.901 | 43.70x | **138.08x faster** |
| add_edge() | 0.011 | 0.022 | 0.709 | 2.05x | **32.23x faster** |
| DFS Traversal | 0.312 | 0.165 | 5.888 | 0.53x | **35.65x faster** |
| BFS Traversal | 0.315 | 0.186 | 13.551 | 0.59x | **72.90x faster** |
| has_edge() | 0.002 | 0.004 | 0.120 | 2.26x | **28.10x faster** |
| get_shortest_path() | 0.055 | 0.048 | 1.066 | 0.88x | **22.13x faster** |
| is_connected() | 0.206 | 0.074 | 3.285 | 0.36x | **44.42x faster** |
| has_cycle() | 0.001 | 0.067 | 7.902 | 68.26x | **118.36x faster** |
| topological_sort() | 0.001 | 0.013 | 0.613 | 14.77x | **45.72x faster** |
| connected_components() | 0.001 | 0.027 | 0.257 | 18.67x | **9.38x faster** |

## Interpretation

- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.
- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.
- Times are in milliseconds (ms) or microseconds (μs).

Pythonic adds abstraction overhead compared to native C++, but aims to be significantly faster than Python while providing a similar, ergonomic API.
