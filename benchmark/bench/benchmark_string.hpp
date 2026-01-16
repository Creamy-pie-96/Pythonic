#pragma once
#include "benchmark_common.hpp"
#include <algorithm>
#include <cctype>

/**
 * Benchmarks for string operations.
 */

inline void benchmark_string_operations()
{
    std::cout << "\n=== Benchmarking String Operations ===" << std::endl;

    // String concatenation
    run_benchmark("String Concatenation", []()
                  {
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::string("Hello") + " " + "World";
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = var("Hello") + " " + "World"; });

    // String comparison
    run_benchmark("String Comparison", []()
                  {
            bool result = false;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = (std::string("hello") == std::string("hello"));
            (void)result; }, []()
                  {
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = (var("hello") == var("hello")); });

    // String upper()
    run_benchmark("String upper()", []()
                  {
            std::string s = "hello world";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result = s;
                std::transform(result.begin(), result.end(), result.begin(), ::toupper);
            }
            (void)result; }, []()
                  {
            var s = "hello world";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.upper(); });

    // String lower()
    run_benchmark("String lower()", []()
                  {
            std::string s = "HELLO WORLD";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result = s;
                std::transform(result.begin(), result.end(), result.begin(), ::tolower);
            }
            (void)result; }, []()
                  {
            var s = "HELLO WORLD";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.lower(); });

    // String strip()
    run_benchmark("String strip()", []()
                  {
            std::string s = "   hello world   ";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                size_t start = s.find_first_not_of(" \t\n\r");
                size_t end = s.find_last_not_of(" \t\n\r");
                result = (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
            }
            (void)result; }, []()
                  {
            var s = "   hello world   ";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.strip(); });

    // String replace()
    run_benchmark("String replace()", []()
                  {
            std::string s = "hello world hello";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result = s;
                size_t pos = 0;
                while ((pos = result.find("hello", pos)) != std::string::npos) {
                    result.replace(pos, 5, "hi");
                    pos += 2;
                }
            }
            (void)result; }, []()
                  {
            var s = "hello world hello";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.replace("hello", "hi"); });

    // String find()
    run_benchmark("String find()", []()
                  {
            std::string s = "hello world hello";
            size_t result = 0;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.find("world");
            (void)result; }, []()
                  {
            var s = "hello world hello";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.find("world"); });

    // String split()
    run_benchmark("String split()", []()
                  {
            std::string s = "one,two,three,four,five";
            std::vector<std::string> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                std::stringstream ss(s);
                std::string item;
                while (std::getline(ss, item, ','))
                    result.push_back(item);
            }
            (void)result; }, []()
                  {
            var s = "one,two,three,four,five";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.split(","); });

    // String startswith()
    run_benchmark("String startswith()", []()
                  {
            std::string s = "hello world";
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = (s.substr(0, 5) == "hello");
            (void)result; }, []()
                  {
            var s = "hello world";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.startswith("hello"); });

    // String endswith()
    run_benchmark("String endswith()", []()
                  {
            std::string s = "hello world";
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = (s.size() >= 5 && s.substr(s.size() - 5) == "world");
            (void)result; }, []()
                  {
            var s = "hello world";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.endswith("world"); });

    // String isdigit()
    run_benchmark("String isdigit()", []()
                  {
            std::string s = "12345";
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = std::all_of(s.begin(), s.end(), ::isdigit);
            (void)result; }, []()
                  {
            var s = "12345";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.isdigit(); });

    // String center()
    run_benchmark("String center()", []()
                  {
            std::string s = "hello";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                int width = 20;
                int pad = width - static_cast<int>(s.size());
                if (pad > 0) {
                    int left = pad / 2;
                    int right = pad - left;
                    result = std::string(left, ' ') + s + std::string(right, ' ');
                } else {
                    result = s;
                }
            }
            (void)result; }, []()
                  {
            var s = "hello";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.center(20); });

    // String zfill()
    run_benchmark("String zfill()", []()
                  {
            std::string s = "42";
            std::string result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                int width = 10;
                if (static_cast<int>(s.size()) < width)
                    result = std::string(width - s.size(), '0') + s;
                else
                    result = s;
            }
            (void)result; }, []()
                  {
            var s = "42";
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.zfill(10); });
}
