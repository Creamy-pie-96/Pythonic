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

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;
using namespace std::chrono;

// Benchmark configuration
const size_t ITERATIONS = 1000000;
const size_t CONTAINER_SIZE = 1000;
const size_t SMALL_ITERATIONS = 10000;

struct BenchmarkResult
{
    std::string name;
    double cpp_time_ms;
    double pythonic_time_ms;
    double python_time_ms;
    double slowdown_factor;
    double pythonic_vs_python;
};

std::vector<BenchmarkResult> results;
std::map<std::string, double> python_results;

// Utility to format time
std::string format_time(double ms)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << ms;
    return oss.str();
}

// Utility to format slowdown
std::string format_slowdown(double factor)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << factor << "x";
    return oss.str();
}

// Escape pipe characters for markdown table cells
std::string escape_markdown_pipes(const std::string &s)
{
    std::string out = s;
    size_t pos = 0;
    while ((pos = out.find('|', pos)) != std::string::npos)
    {
        out.replace(pos, 1, "\\|");
        pos += 2; // skip escaped sequence
    }
    return out;
}

// Simple JSON parser for Python results
void load_python_results()
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
        // Simple parsing: find "name": value pattern
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            // Extract name (between quotes)
            size_t first_quote = line.find('"');
            size_t second_quote = line.find('"', first_quote + 1);
            if (first_quote != std::string::npos && second_quote != std::string::npos)
            {
                std::string name = line.substr(first_quote + 1, second_quote - first_quote - 1);

                // Extract value (after colon)
                std::string value_str = line.substr(colon_pos + 1);
                // Remove whitespace and comma
                value_str.erase(0, value_str.find_first_not_of(" \t"));
                if (value_str.back() == ',')
                    value_str.pop_back();

                try
                {
                    double value = std::stod(value_str);
                    python_results[name] = value;
                }
                catch (...)
                {
                    // Skip invalid lines
                }
            }
        }
    }
    file.close();
}

void add_result(const std::string &name, double cpp_time, double pythonic_time)
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

void benchmark_arithmetic_operations()
{
    std::cout << "\n=== Benchmarking Arithmetic Operations ===" << std::endl;

    // Integer addition
    {
        auto start = high_resolution_clock::now();
        int sum = 0;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            sum = sum + 1;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_sum = 0;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            var_sum = var_sum + 1;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Integer Addition", cpp_time, pythonic_time);
        std::cout << "  Integer Addition: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Integer multiplication
    {
        auto start = high_resolution_clock::now();
        int prod = 1;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            prod = prod * 2;
            if (prod > 1000000)
                prod = 1;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_prod = 1;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            var_prod = var_prod * 2;
            if (var_prod.get<int>() > 1000000)
                var_prod = 1;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Integer Multiplication", cpp_time, pythonic_time);
        std::cout << "  Integer Multiplication: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Double operations
    {
        auto start = high_resolution_clock::now();
        double sum = 0.0;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            sum = sum + 1.5;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_sum = 0.0;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            var_sum = var_sum + 1.5;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Double Addition", cpp_time, pythonic_time);
        std::cout << "  Double Addition: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Comparisons
    {
        auto start = high_resolution_clock::now();
        bool result = false;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            result = (i % 2 == 0);
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_result = false;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            var_result = (var(i) % 2 == 0);
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Integer Comparison", cpp_time, pythonic_time);
        std::cout << "  Integer Comparison: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_string_operations()
{
    std::cout << "\n=== Benchmarking String Operations ===" << std::endl;

    // String concatenation
    {
        auto start = high_resolution_clock::now();
        std::string result;
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            result = std::string("Hello") + " " + "World";
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_result;
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            var_result = var("Hello") + " " + "World";
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("String Concatenation", cpp_time, pythonic_time);
        std::cout << "  String Concatenation: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // String comparison
    {
        auto start = high_resolution_clock::now();
        bool result = false;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            result = (std::string("hello") == std::string("hello"));
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_result = false;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            var_result = (var("hello") == var("hello"));
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("String Comparison", cpp_time, pythonic_time);
        std::cout << "  String Comparison: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_container_creation()
{
    std::cout << "\n=== Benchmarking Container Creation ===" << std::endl;

    // List creation
    {
        auto start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            var lst = list(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("List Creation (10 elements)", cpp_time, pythonic_time);
        std::cout << "  List Creation: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Set creation
    {
        auto start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            std::set<int> s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            var s = set(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Set Creation (10 elements)", cpp_time, pythonic_time);
        std::cout << "  Set Creation: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_container_operations()
{
    std::cout << "\n=== Benchmarking Container Operations ===" << std::endl;

    // List append
    {
        auto start = high_resolution_clock::now();
        std::vector<int> vec;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            vec.push_back(i);
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var lst = list();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            lst.append(var(i));
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("List Append", cpp_time, pythonic_time);
        std::cout << "  List Append: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // List access
    {
        std::vector<int> vec;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            vec.push_back(i);

        auto start = high_resolution_clock::now();
        int sum = 0;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            sum += vec[i];
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var lst = list();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            lst.append(var(i));

        start = high_resolution_clock::now();
        var var_sum = 0;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            var_sum = var_sum + lst[i];
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("List Access (indexed)", cpp_time, pythonic_time);
        std::cout << "  List Access: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Set insertion
    {
        auto start = high_resolution_clock::now();
        std::set<int> s;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            s.insert(i);
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_set = set();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            var_set.add(var(i));
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Set Insertion", cpp_time, pythonic_time);
        std::cout << "  Set Insertion: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Dict insertion
    {
        auto start = high_resolution_clock::now();
        std::map<std::string, int> m;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            m["key" + std::to_string(i)] = i;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var d = dict();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
        {
            d["key" + std::to_string(i)] = var(i);
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Dict Insertion", cpp_time, pythonic_time);
        std::cout << "  Dict Insertion: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_container_operators()
{
    std::cout << "\n=== Benchmarking Container Operators ===" << std::endl;

    // Set union
    {
        std::set<int> s1, s2;
        for (size_t i = 0; i < 100; ++i)
            s1.insert(i);
        for (size_t i = 50; i < 150; ++i)
            s2.insert(i);

        auto start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            std::set<int> result;
            result.insert(s1.begin(), s1.end());
            result.insert(s2.begin(), s2.end());
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var set1 = set(), set2 = set();
        for (size_t i = 0; i < 100; ++i)
            set1.add(var(i));
        for (size_t i = 50; i < 150; ++i)
            set2.add(var(i));

        start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            var result = set1 | set2;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Set Union (|)", cpp_time, pythonic_time);
        std::cout << "  Set Union: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // List concatenation
    {
        std::vector<int> v1, v2;
        for (size_t i = 0; i < 100; ++i)
            v1.push_back(i);
        for (size_t i = 0; i < 100; ++i)
            v2.push_back(i);

        auto start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            std::vector<int> result = v1;
            result.insert(result.end(), v2.begin(), v2.end());
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var list1 = list(), list2 = list();
        for (size_t i = 0; i < 100; ++i)
            list1.append(var(i));
        for (size_t i = 0; i < 100; ++i)
            list2.append(var(i));

        start = high_resolution_clock::now();
        for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
        {
            var result = list1 | list2;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("List Concatenation (|)", cpp_time, pythonic_time);
        std::cout << "  List Concatenation: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_loops()
{
    std::cout << "\n=== Benchmarking Loop Constructs ===" << std::endl;

    // C++ for loop
    {
        auto start = high_resolution_clock::now();
        int sum = 0;
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            sum += i;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        start = high_resolution_clock::now();
        var var_sum = 0;
        for_in(i, range(ITERATIONS))
        {
            var_sum = var_sum + i;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Loop Iteration (for_in + range)", cpp_time, pythonic_time);
        std::cout << "  Loop (for_in + range): C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Range-based for loop
    {
        std::vector<int> vec;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            vec.push_back(i);

        auto start = high_resolution_clock::now();
        int sum = 0;
        for (auto &x : vec)
        {
            sum += x;
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var lst = list();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            lst.append(var(i));

        start = high_resolution_clock::now();
        var var_sum = 0;
        for_in(x, lst)
        {
            var_sum = var_sum + x;
        }
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Loop over Container (for_in)", cpp_time, pythonic_time);
        std::cout << "  Loop over Container: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void benchmark_functional()
{
    std::cout << "\n=== Benchmarking Functional Operations ===" << std::endl;

    // Map operation
    {
        std::vector<int> vec;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            vec.push_back(i);

        auto start = high_resolution_clock::now();
        std::vector<int> result;
        for (auto &x : vec)
        {
            result.push_back(x * 2);
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var lst = list();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            lst.append(var(i));

        start = high_resolution_clock::now();
        var mapped = map(lambda_(x, x * 2), lst);
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Map (transform)", cpp_time, pythonic_time);
        std::cout << "  Map: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }

    // Filter operation
    {
        std::vector<int> vec;
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            vec.push_back(i);

        auto start = high_resolution_clock::now();
        std::vector<int> result;
        for (auto &x : vec)
        {
            if (x % 2 == 0)
                result.push_back(x);
        }
        auto end = high_resolution_clock::now();
        double cpp_time = duration<double, std::milli>(end - start).count();

        var lst = list();
        for (size_t i = 0; i < CONTAINER_SIZE; ++i)
            lst.append(var(i));

        start = high_resolution_clock::now();
        var filtered = filter(lambda_(x, x % 2 == 0), lst);
        end = high_resolution_clock::now();
        double pythonic_time = duration<double, std::milli>(end - start).count();

        add_result("Filter", cpp_time, pythonic_time);
        std::cout << "  Filter: C++ " << format_time(cpp_time) << "ms, Pythonic "
                  << format_time(pythonic_time) << "ms (" << format_slowdown(pythonic_time / cpp_time) << ")" << std::endl;
    }
}

void generate_markdown_report(const std::string &filename)
{
    std::ofstream out(filename);

    out << "# Pythonic C++ Library Benchmark Report\n\n";
    out << "Generated on: " << __DATE__ << " " << __TIME__ << "\n\n";
    out << "## Configuration\n\n";
    out << "- **Iterations (Arithmetic/Comparisons)**: " << ITERATIONS << "\n";
    out << "- **Small Iterations (Strings/Containers)**: " << SMALL_ITERATIONS << "\n";
    out << "- **Container Size**: " << CONTAINER_SIZE << "\n\n";

    out << "## Three-Way Comparison\n\n";
    out << "| Operation | C++ (ms) | Pythonic (ms) | Python (ms) | Pythonic vs C++ | Pythonic vs Python |\n";
    out << "|-----------|----------|---------------|-------------|-----------------|--------------------|\n";

    double total_slowdown_cpp = 0.0;
    double total_slowdown_python = 0.0;
    int python_count = 0;

    for (const auto &result : results)
    {
        out << "| " << escape_markdown_pipes(result.name) << " | "
            << format_time(result.cpp_time_ms) << " | "
            << format_time(result.pythonic_time_ms) << " | ";

        if (result.python_time_ms > 0)
        {
            out << format_time(result.python_time_ms) << " | "
                << format_slowdown(result.slowdown_factor) << " | "
                << format_slowdown(result.pythonic_vs_python) << " |\n";
            total_slowdown_python += result.pythonic_vs_python;
            python_count++;
        }
        else
        {
            out << "N/A | " << format_slowdown(result.slowdown_factor) << " | N/A |\n";
        }
        total_slowdown_cpp += result.slowdown_factor;
    }

    double avg_slowdown_cpp = total_slowdown_cpp / results.size();
    double avg_slowdown_python = (python_count > 0) ? (total_slowdown_python / python_count) : 0.0;

    out << "\n## Analysis\n\n";
    out << "- **Average Pythonic C++ vs C++**: " << format_slowdown(avg_slowdown_cpp) << "\n";
    if (python_count > 0)
    {
        out << "- **Average Pythonic C++ vs Python**: " << format_slowdown(avg_slowdown_python) << "\n";
    }
    out << "- **Total Benchmarks**: " << results.size() << "\n";
    if (python_count > 0)
    {
        out << "- **Python Benchmarks Available**: " << python_count << " / " << results.size() << "\n";
    }
    out << "\n";

    // Find best and worst vs C++
    auto best = std::min_element(results.begin(), results.end(),
                                 [](const BenchmarkResult &a, const BenchmarkResult &b)
                                 { return a.slowdown_factor < b.slowdown_factor; });
    auto worst = std::max_element(results.begin(), results.end(),
                                  [](const BenchmarkResult &a, const BenchmarkResult &b)
                                  { return a.slowdown_factor < b.slowdown_factor; });

    out << "### Best Performance (vs C++)\n\n";
    out << "**" << best->name << "**: " << format_slowdown(best->slowdown_factor)
        << " slower than native C++\n\n";

    out << "### Worst Performance (vs C++)\n\n";
    out << "**" << worst->name << "**: " << format_slowdown(worst->slowdown_factor)
        << " slower than native C++\n\n";

    // Find best and worst vs Python
    if (python_count > 0)
    {
        auto best_py = std::min_element(results.begin(), results.end(),
                                        [](const BenchmarkResult &a, const BenchmarkResult &b)
                                        {
                                            if (a.python_time_ms == 0)
                                                return false;
                                            if (b.python_time_ms == 0)
                                                return true;
                                            return a.pythonic_vs_python < b.pythonic_vs_python;
                                        });
        auto worst_py = std::max_element(results.begin(), results.end(),
                                         [](const BenchmarkResult &a, const BenchmarkResult &b)
                                         {
                                             if (a.python_time_ms == 0)
                                                 return true;
                                             if (b.python_time_ms == 0)
                                                 return false;
                                             return a.pythonic_vs_python < b.pythonic_vs_python;
                                         });

        if (best_py->python_time_ms > 0)
        {
            out << "### Best Performance (vs Python)\n\n";
            out << "**" << best_py->name << "**: " << format_slowdown(best_py->pythonic_vs_python);
            if (best_py->pythonic_vs_python < 1.0)
            {
                out << " (FASTER than Python!)\n\n";
            }
            else
            {
                out << "\n\n";
            }
        }

        if (worst_py->python_time_ms > 0)
        {
            out << "### Worst Performance (vs Python)\n\n";
            out << "**" << worst_py->name << "**: " << format_slowdown(worst_py->pythonic_vs_python) << "\n\n";
        }
    }

    out << "## Performance Overview\n\n";
    if (python_count > 0)
    {
        if (avg_slowdown_python < 1.0)
        {
            out << "**Pythonic C++ is on average FASTER than Python** ("
                << format_slowdown(avg_slowdown_python) << "), showing that the library provides ";
            out << "Python-like syntax while maintaining significant C++ performance advantages.\n\n";
        }
        else if (avg_slowdown_python < 2.0)
        {
            out << "**Pythonic C++ performs comparably to Python** ("
                << format_slowdown(avg_slowdown_python) << "), providing similar performance ";
            out << "with Python-like syntax in a compiled language.\n\n";
        }
        else
        {
            out << "**Pythonic C++ is slower than Python in these microbenchmarks** ("
                << format_slowdown(avg_slowdown_python) << "). Note that real-world performance ";
            out << "varies based on usage patterns and compiler optimizations.\n\n";
        }
    }

    out << "**Pythonic C++ vs Native C++**: " << format_slowdown(avg_slowdown_cpp)
        << " average overhead for dynamic typing and Python-like syntax.\n\n";

    out << "## Detailed Results\n\n";

    // Group by category
    out << "### Arithmetic Operations\n\n";
    for (const auto &result : results)
    {
        if (result.name.find("Integer") != std::string::npos ||
            result.name.find("Double") != std::string::npos ||
            result.name.find("Comparison") != std::string::npos)
        {
            out << "- **" << result.name << "**: "
                << format_slowdown(result.slowdown_factor) << " (C++: "
                << format_time(result.cpp_time_ms) << "ms, Pythonic: "
                << format_time(result.pythonic_time_ms) << "ms)\n";
        }
    }

    out << "\n### String Operations\n\n";
    for (const auto &result : results)
    {
        if (result.name.find("String") != std::string::npos)
        {
            out << "- **" << result.name << "**: "
                << format_slowdown(result.slowdown_factor) << " (C++: "
                << format_time(result.cpp_time_ms) << "ms, Pythonic: "
                << format_time(result.pythonic_time_ms) << "ms)\n";
        }
    }

    out << "\n### Container Operations\n\n";
    for (const auto &result : results)
    {
        if (result.name.find("List") != std::string::npos ||
            result.name.find("Set") != std::string::npos ||
            result.name.find("Dict") != std::string::npos)
        {
            out << "- **" << result.name << "**: "
                << format_slowdown(result.slowdown_factor) << " (C++: "
                << format_time(result.cpp_time_ms) << "ms, Pythonic: "
                << format_time(result.pythonic_time_ms) << "ms)\n";
        }
    }

    out << "\n### Loop Constructs\n\n";
    for (const auto &result : results)
    {
        if (result.name.find("Loop") != std::string::npos)
        {
            out << "- **" << result.name << "**: "
                << format_slowdown(result.slowdown_factor) << " (C++: "
                << format_time(result.cpp_time_ms) << "ms, Pythonic: "
                << format_time(result.pythonic_time_ms) << "ms)\n";
        }
    }

    out << "\n### Functional Operations\n\n";
    for (const auto &result : results)
    {
        if (result.name.find("Map") != std::string::npos ||
            result.name.find("Filter") != std::string::npos)
        {
            out << "- **" << result.name << "**: "
                << format_slowdown(result.slowdown_factor) << " (C++: "
                << format_time(result.cpp_time_ms) << "ms, Pythonic: "
                << format_time(result.pythonic_time_ms) << "ms)\n";
        }
    }

    out << "\n## Interpretation\n\n";
    out << "The Pythonic C++ library provides Python-like syntax at the cost of performance. ";
    out << "The overhead comes from:\n\n";
    out << "1. **Type erasure**: Using `std::variant` for dynamic typing\n";
    out << "2. **Virtual dispatch**: Pattern matching with `std::visit`\n";
    out << "3. **Allocation overhead**: More dynamic allocations than native C++\n";
    out << "4. **Wrapper overhead**: Function call overhead for operations\n\n";
    out << "**When to use Pythonic C++**:\n";
    out << "- Rapid prototyping where Python-like syntax helps\n";
    out << "- Applications where developer productivity > raw performance\n";
    out << "- Mixed workloads where convenience matters more than speed\n\n";
    out << "**When to avoid**:\n";
    out << "- Performance-critical inner loops\n";
    out << "- Real-time systems with strict timing requirements\n";
    out << "- High-frequency trading or game engines\n\n";

    out.close();
    std::cout << "\n Benchmark report saved to: " << filename << std::endl;
}

int main(int argc, char *argv[])
{
    std::string report_file = "benchmark_report.md";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc)
        {
            report_file = argv[i + 1];
            break;
        }
    }

    // Run Python benchmark first
    std::cout << "Running Python benchmark..." << std::endl;
    int py_result = system("python3 benchmark.py");
    if (py_result != 0)
    {
        std::cerr << "Warning: Python benchmark failed or not available" << std::endl;
    }

    // Load Python results
    load_python_results();

    std::cout << "\n==================================================" << std::endl;
    std::cout << "   PYTHONIC C++ LIBRARY PERFORMANCE BENCHMARK     " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Iterations: " << ITERATIONS << std::endl;
    std::cout << "  Small Iterations: " << SMALL_ITERATIONS << std::endl;
    std::cout << "  Container Size: " << CONTAINER_SIZE << std::endl;

    benchmark_arithmetic_operations();
    benchmark_string_operations();
    benchmark_container_creation();
    benchmark_container_operations();
    benchmark_container_operators();
    benchmark_loops();
    benchmark_functional();

    std::cout << "\n==================================================" << std::endl;
    std::cout << "              BENCHMARK COMPLETE                  " << std::endl;
    std::cout << "==================================================" << std::endl;

    generate_markdown_report(report_file);

    return 0;
}
