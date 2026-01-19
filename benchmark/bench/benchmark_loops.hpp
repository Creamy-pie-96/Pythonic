#pragma once
#include "benchmark_common.hpp"

/**
 * Benchmarks for loop constructs.
 */

inline void benchmark_loop_operations()
{
    std::cout << "\n=== Benchmarking Loop Operations ===" << std::endl;

    // range() iteration
    run_benchmark("range() Iteration", []()
                  {
            long long sum = 0;
            for (int i = 0; i < 1000; ++i)
                sum += i;
            (void)sum; }, []()
                  {
            var sum = 0;
            for_each(i, range(1000))
                sum = sum + i; });

    // range(start, stop) iteration
    run_benchmark("range(start, stop) Iteration", []()
                  {
            long long sum = 0;
            for (int i = 100; i < 1000; ++i)
                sum += i;
            (void)sum; }, []()
                  {
            var sum = 0;
            for_each(i, range(100, 1000))
                sum = sum + i; });

    // range(start, stop, step) iteration
    run_benchmark("range(start, stop, step) Iteration", []()
                  {
            long long sum = 0;
            for (int i = 0; i < 1000; i += 2)
                sum += i;
            (void)sum; }, []()
                  {
            var sum = 0;
            for_each(i, range(0, 1000, 2))
                sum = sum + i; });

    // for_each with list
    run_benchmark("for_each with List", []()
                  {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) v.push_back(i);
            long long sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                for (int x : v)
                    sum += x;
            (void)sum; }, []()
                  {
            var v = list();
            for (int i = 0; i < 100; ++i) v.append(i);
            var sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                for_each(x, v)
                    sum = sum + x; });

    // enumerate()
    run_benchmark("enumerate()", []()
                  {
            std::vector<std::string> v = {"a", "b", "c", "d", "e"};
            long long sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                for (size_t i = 0; i < v.size(); ++i)
                    sum += static_cast<long long>(i);
            }
            (void)sum; }, []()
                  {
            var v = list("a", "b", "c", "d", "e");
            var sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                size_t idx = 0;
                for_each(item, enumerate(v)) {
                    sum = sum + var(static_cast<long long>(std::get<0>(item)));
                }
            } });

    // zip()
    run_benchmark("zip()", []()
                  {
            std::vector<int> v1 = {1, 2, 3, 4, 5};
            std::vector<int> v2 = {10, 20, 30, 40, 50};
            long long sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                for (size_t i = 0; i < std::min(v1.size(), v2.size()); ++i)
                    sum += v1[i] + v2[i];
            }
            (void)sum; }, []()
                  {
            var v1 = list(1, 2, 3, 4, 5);
            var v2 = list(10, 20, 30, 40, 50);
            var sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                for_each(p, zip(v1, v2))
                    sum = sum + std::get<0>(p) + std::get<1>(p);
            } });

    // dict iteration with items()
    run_benchmark("Dict Iteration (items())", []()
                  {
            std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"e", 5}};
            long long sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                for (const auto& p : m)
                    sum += p.second;
            (void)sum; }, []()
                  {
            var d = dict();
            d["a"] = 1;
            d["b"] = 2;
            d["c"] = 3;
            d["d"] = 4;
            d["e"] = 5;
            var sum = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                for_each(item, d.items())
                    sum = sum + item[1]; });
}
