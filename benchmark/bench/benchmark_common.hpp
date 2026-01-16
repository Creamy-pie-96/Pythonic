#pragma once
/**
 * Common utilities for benchmarking the Pythonic C++ library.
 */

#include "pythonic.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <functional>
#include <random>

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;
using namespace pythonic::math;
using namespace std::chrono;

// Benchmark configuration
inline const size_t ITERATIONS = 1000000;
inline const size_t CONTAINER_SIZE = 1000;
inline const size_t SMALL_ITERATIONS = 10000;
inline const size_t TINY_ITERATIONS = 1000;

struct BenchmarkResult
{
    std::string name;
    double cpp_time_ms;
    double pythonic_time_ms;
    double python_time_ms;
    double slowdown_factor;
    double pythonic_vs_python;
};

inline std::vector<BenchmarkResult> results;
inline std::map<std::string, double> python_results;

// Utility to format time
inline std::string format_time(double ms)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << ms;
    return oss.str();
}

// Utility to format slowdown
inline std::string format_slowdown(double factor)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << factor << "x";
    return oss.str();
}

// Simple JSON parser for Python results
inline void load_python_results()
{
    std::ifstream file("python_results.json");
    if (!file.is_open())
    {
        std::cerr << "Warning: Could not open python_results.json" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Find the key between quotes
        size_t first_quote = line.find('"');
        size_t second_quote = line.find('"', first_quote + 1);
        if (first_quote != std::string::npos && second_quote != std::string::npos)
        {
            std::string name = line.substr(first_quote + 1, second_quote - first_quote - 1);
            // Find the colon AFTER the closing quote (key names may contain colons like [::2])
            size_t colon_pos = line.find(':', second_quote);
            if (colon_pos != std::string::npos)
            {
                std::string value_str = line.substr(colon_pos + 1);
                value_str.erase(0, value_str.find_first_not_of(" \t"));
                if (!value_str.empty() && value_str.back() == ',')
                    value_str.pop_back();
                try
                {
                    double value = std::stod(value_str);
                    python_results[name] = value;
                }
                catch (...)
                {
                }
            }
        }
    }
    file.close();
}

inline void add_result(const std::string &name, double cpp_time, double pythonic_time)
{
    double python_time = 0.0;
    auto it = python_results.find(name);
    if (it != python_results.end())
    {
        python_time = it->second;
    }
    double slowdown = pythonic_time / cpp_time;
    double pythonic_vs_python = (python_time > 0) ? (pythonic_time / python_time) : 0.0;
    results.push_back({name, cpp_time, pythonic_time, python_time, slowdown, pythonic_vs_python});
}

// Function to run benchmark with lambdas (avoids macro comma issues)
inline void run_benchmark(const std::string &name,
                          std::function<void()> cpp_code,
                          std::function<void()> pythonic_code)
{
    auto start = high_resolution_clock::now();
    cpp_code();
    auto end = high_resolution_clock::now();
    double cpp_time = duration<double, std::milli>(end - start).count();

    start = high_resolution_clock::now();
    pythonic_code();
    end = high_resolution_clock::now();
    double pythonic_time = duration<double, std::milli>(end - start).count();

    add_result(name, cpp_time, pythonic_time);
    std::cout << "  " << name << ": C++ " << format_time(cpp_time) << "ms, Pythonic "
              << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
}
