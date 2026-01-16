#pragma once
#include "benchmark_common.hpp"
#include <algorithm>

/**
 * Benchmarks for sorting operations.
 */

inline void benchmark_sorting_operations()
{
    std::cout << "\n=== Benchmarking Sorting Operations ===" << std::endl;

    // sorted() ascending
    run_benchmark("sorted() Ascending", []()
                  {
            std::vector<int> result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
                std::sort(result.begin(), result.end());
            }
            (void)result; }, []()
                  {
            var v = list(5, 2, 8, 1, 9, 3, 7, 4, 6, 0);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = pythonic::func::sorted(v); });

    // sorted() descending
    run_benchmark("sorted() Descending", []()
                  {
            std::vector<int> result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
                std::sort(result.begin(), result.end(), std::greater<int>());
            }
            (void)result; }, []()
                  {
            var v = list(5, 2, 8, 1, 9, 3, 7, 4, 6, 0);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = pythonic::func::sorted(v, true); });

    // reversed() - skip since it returns a wrapper, not var
    // list.sort() - skip since var doesn't have sort method

    // Large list sorting
    run_benchmark("Large List Sorting (1000 elements)", []()
                  {
            std::vector<int> v;
            for (int i = 1000; i > 0; --i) v.push_back(i);
            for (size_t iter = 0; iter < 100; ++iter) {
                std::vector<int> result = v;
                std::sort(result.begin(), result.end());
            } }, []()
                  {
            var v = list();
            for (int i = 1000; i > 0; --i) v.append(i);
            for (size_t iter = 0; iter < 100; ++iter) {
                var result = pythonic::func::sorted(v);
            } });
}
