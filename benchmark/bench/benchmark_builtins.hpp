#pragma once
#include "benchmark_common.hpp"
#include <algorithm>
#include <cmath>

/**
 * Benchmarks for built-in functions.
 */

inline void benchmark_builtin_operations()
{
    std::cout << "\n=== Benchmarking Built-in Functions ===" << std::endl;

    // len()
    run_benchmark("len()", []()
                  {
            std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            size_t result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = v.size();
            (void)result; }, []()
                  {
            var v = list(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
            size_t result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = len(v);
            (void)result; });

    // sum()
    run_benchmark("sum()", []()
                  {
            std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            int result = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter) {
                result = 0;
                for (int x : v) result += x;
            }
            (void)result; }, []()
                  {
            var v = list(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = sum(v); });

    // min()
    run_benchmark("min()", []()
                  {
            std::vector<int> v = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
            int result = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = *std::min_element(v.begin(), v.end());
            (void)result; }, []()
                  {
            var v = list(5, 2, 8, 1, 9, 3, 7, 4, 6, 0);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = min(v); });

    // max()
    run_benchmark("max()", []()
                  {
            std::vector<int> v = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
            int result = 0;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = *std::max_element(v.begin(), v.end());
            (void)result; }, []()
                  {
            var v = list(5, 2, 8, 1, 9, 3, 7, 4, 6, 0);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = max(v); });

    // abs()
    run_benchmark("abs()", []()
                  {
            int result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::abs(-42);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = abs(var(-42)); });

    // all()
    run_benchmark("all()", []()
                  {
            std::vector<bool> v = {true, true, true, true, true};
            bool result = false;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = std::all_of(v.begin(), v.end(), [](bool b) { return b; });
            (void)result; }, []()
                  {
            var v = list(true, true, true, true, true);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = all(v); });

    // any()
    run_benchmark("any()", []()
                  {
            std::vector<bool> v = {false, false, true, false, false};
            bool result = false;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = std::any_of(v.begin(), v.end(), [](bool b) { return b; });
            (void)result; }, []()
                  {
            var v = list(false, false, true, false, false);
            var result;
            for (size_t iter = 0; iter < SMALL_ITERATIONS; ++iter)
                result = any(v); });

    // pow()
    run_benchmark("pow()", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::pow(2.0, 10.0);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = pow(var(2), var(10)); });

    // sqrt()
    run_benchmark("sqrt()", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::sqrt(144.0);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = sqrt(var(144)); });

    // floor()
    run_benchmark("floor()", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::floor(3.7);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = floor(var(3.7)); });

    // ceil()
    run_benchmark("ceil()", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::ceil(3.2);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = ceil(var(3.2)); });

    // round()
    run_benchmark("round()", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = std::round(3.567);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = round(var(3.567)); });
}
