#pragma once
#include "benchmark_common.hpp"

/**
 * Benchmarks for type conversions.
 */

inline void benchmark_conversion_operations()
{
    std::cout << "\n=== Benchmarking Type Conversions ===" << std::endl;

    // Int()
    run_benchmark("Int() from String", []()
                  {
            std::string s = "12345";
            int result = 0;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::stoi(s);
            (void)result; }, []()
                  {
            var s = "12345";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = Int(s); });

    // Float()
    run_benchmark("Float() from String", []()
                  {
            std::string s = "123.456";
            double result = 0;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::stod(s);
            (void)result; }, []()
                  {
            var s = "123.456";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = Float(s); });

    // Str() from Int
    run_benchmark("Str() from Int", []()
                  {
            int n = 12345;
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::to_string(n);
            (void)result; }, []()
                  {
            var n = 12345;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = Str(n); });

    // Bool()
    run_benchmark("Bool() from Int", []()
                  {
            int n = 42;
            bool result = false;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = (n != 0);
            (void)result; }, []()
                  {
            var n = 42;
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = Bool(n); });

    // Int to Double
    run_benchmark("Int to Double", []()
                  {
            int n = 42;
            double result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = static_cast<double>(n);
            (void)result; }, []()
                  {
            var n = 42;
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = Float(n); });

    // type()
    run_benchmark("type()", []()
                  {
            // Native C++ doesn't have dynamic type checking
            for (size_t i = 0; i < ITERATIONS; ++i) {
                // Placeholder - native comparison not meaningful
            } }, []()
                  {
            var n = 42;
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = n.type(); });

    // isinstance()
    run_benchmark("isinstance()", []()
                  {
            // Native C++ doesn't have isinstance
            for (size_t i = 0; i < ITERATIONS; ++i) {
                // Placeholder
            } }, []()
                  {
            var n = 42;
            bool result = false;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = isinstance(n, "int");
            (void)result; });
}
