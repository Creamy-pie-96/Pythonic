#pragma once
#include "benchmark_common.hpp"

/**
 * Benchmarks for arithmetic operations.
 */

inline void benchmark_arithmetic_operations()
{
    std::cout << "\n=== Benchmarking Arithmetic Operations ===" << std::endl;

    run_benchmark("Integer Addition", []()
                  {
            long long result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = 42 + 58;
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = var(42) + var(58); });

    run_benchmark("Integer Multiplication", []()
                  {
            long long result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = 42 * 58;
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = var(42) * var(58); });

    run_benchmark("Double Addition", []()
                  {
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = 3.14 + 2.86;
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = var(3.14) + var(2.86); });

    run_benchmark("Integer Division", []()
                  {
            long long result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = 100 / 7;
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = var(100) / var(7); });

    run_benchmark("Integer Modulo", []()
                  {
            long long result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = 100 % 7;
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = var(100) % var(7); });

    run_benchmark("Integer Comparison", []()
                  {
            bool result = false;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = (42 < 58);
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = (var(42) < var(58)); });
}
