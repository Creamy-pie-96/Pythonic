#pragma once
#include "benchmark_common.hpp"

/**
 * Benchmarks for slicing operations.
 */

inline void benchmark_slicing_operations()
{
    std::cout << "\n=== Benchmarking Slicing Operations ===" << std::endl;

    // String slicing (substring)
    run_benchmark("String Slice [2:8]", []()
                  {
            std::string s = "hello world";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.substr(2, 6);
            (void)result; }, []()
                  {
            var s = "hello world";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.slice(2, 8); });

    // List slicing
    run_benchmark("List Slice [2:8]", []()
                  {
            std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::vector<int>(v.begin() + 2, v.begin() + 8);
            (void)result; }, []()
                  {
            var v = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v.slice(2, 8); });

    // String slice with step
    run_benchmark("String Slice [::2]", []()
                  {
            std::string s = "hello world";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (size_t j = 0; j < s.size(); j += 2)
                    result += s[j];
            }
            (void)result; }, []()
                  {
            var s = "hello world";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.slice(0, static_cast<long long>(LLONG_MAX), 2); });

    // List slice with step
    run_benchmark("List Slice [::2]", []()
                  {
            std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (size_t j = 0; j < v.size(); j += 2)
                    result.push_back(v[j]);
            }
            (void)result; }, []()
                  {
            var v = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v.slice(0, static_cast<long long>(LLONG_MAX), 2); });

    // Negative slicing
    run_benchmark("List Slice [-5:-1]", []()
                  {
            std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                int start = v.size() - 5;
                int end = v.size() - 1;
                result = std::vector<int>(v.begin() + start, v.begin() + end);
            }
            (void)result; }, []()
                  {
            var v = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v.slice(-5, -1); });

    // Reverse slice
    run_benchmark("List Slice [::-1] (Reverse)", []()
                  {
            std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (int j = static_cast<int>(v.size()) - 1; j >= 0; --j)
                    result.push_back(v[j]);
            }
            (void)result; }, []()
                  {
            var v = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v.slice(static_cast<long long>(LLONG_MAX), static_cast<long long>(LLONG_MIN), -1); });
}
