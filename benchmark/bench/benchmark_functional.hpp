#pragma once
#include "benchmark_common.hpp"
#include <numeric>

/**
 * Benchmarks for functional programming constructs.
 */

inline void benchmark_functional_operations()
{
    std::cout << "\n=== Benchmarking Functional Operations ===" << std::endl;

    // map()
    run_benchmark("map()", []()
                  {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) v.push_back(i);
            std::vector<int> result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result.clear();
                result.reserve(v.size());
                for (int x : v)
                    result.push_back(x * 2);
            }
            (void)result; }, []()
                  {
            var v = list();
            for (int i = 0; i < 100; ++i) v.append(i);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = map([](var x) { return x * 2; }, v); });

    // filter()
    run_benchmark("filter()", []()
                  {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) v.push_back(i);
            std::vector<int> result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result.clear();
                for (int x : v)
                    if (x % 2 == 0)
                        result.push_back(x);
            }
            (void)result; }, []()
                  {
            var v = list();
            for (int i = 0; i < 100; ++i) v.append(i);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = filter([](var x) { return x % 2 == 0; }, v); });

    // reduce()
    run_benchmark("reduce()", []()
                  {
            std::vector<int> v;
            for (int i = 1; i <= 100; ++i) v.push_back(i);
            int result = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = std::accumulate(v.begin(), v.end(), 0);
            (void)result; }, []()
                  {
            var v = list();
            for (int i = 1; i <= 100; ++i) v.append(i);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = reduce([](var a, var b) { return a + b; }, v); });

    // Chained map and filter
    run_benchmark("Chained map + filter", []()
                  {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) v.push_back(i);
            std::vector<int> result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result.clear();
                for (int x : v) {
                    int doubled = x * 2;
                    if (doubled > 50)
                        result.push_back(doubled);
                }
            }
            (void)result; }, []()
                  {
            var v = list();
            for (int i = 0; i < 100; ++i) v.append(i);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                var mapped = map([](var x) { return x * 2; }, v);
                result = filter([](var x) { return x > 50; }, mapped);
            } });

    // Lambda application
    run_benchmark("Lambda Application", []()
                  {
            auto fn = [](int x) { return x * x; };
            int result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = fn(static_cast<int>(i % 100));
            (void)result; }, []()
                  {
            auto fn = [](var x) { return x * x; };
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = fn(var(static_cast<long long>(i % 100))); });
}
