/**
 * Comprehensive Benchmark Suite for Pythonic Library
 *
 * This benchmark compares the performance of Pythonic's var type and operations
 * against native C++ equivalents. Results are compared against Python benchmarks.
 *
 * Categories benchmarked:
 * - Arithmetic operations
 * - String operations
 * - Slicing operations
 * - Container operations
 * - Loop constructs
 * - Functional programming
 * - Sorting operations
 * - Built-in functions
 * - Type conversions
 * - Graph operations
 */

// Include all benchmark modules
#include "bench/benchmark_common.hpp"
#include "bench/benchmark_arithmetic.hpp"
#include "bench/benchmark_string.hpp"
#include "bench/benchmark_slicing.hpp"
#include "bench/benchmark_containers.hpp"
#include "bench/benchmark_loops.hpp"
#include "bench/benchmark_functional.hpp"
#include "bench/benchmark_sorting.hpp"
#include "bench/benchmark_builtins.hpp"
#include "bench/benchmark_conversions.hpp"
#include "bench/benchmark_graph.hpp"

#include <fstream>
#include <ctime>

// Helper to write markdown report
void write_markdown_report(const std::string &filename)
{
    std::ofstream md(filename);
    if (!md.is_open())
    {
        std::cerr << "Warning: Could not create " << filename << std::endl;
        return;
    }

    // Get current date
    time_t now = time(nullptr);
    char date_buf[100];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    md << "# Pythonic Library Benchmark Report\n\n";
    md << "**Generated:** " << date_buf << "\n\n";
    md << "This benchmark compares:\n";
    md << "- **Native C++**: Direct C++ STL operations\n";
    md << "- **Pythonic**: The Pythonic library's `var` type\n";
    md << "- **Python**: Native Python 3 (when available)\n\n";

    // Statistics
    int total = 0;
    int faster_than_python = 0;
    int slower_than_python = 0;
    int no_python_data = 0;
    double total_native_overhead = 0;
    double total_python_speedup = 0;

    for (const auto &r : results)
    {
        total++;
        if (r.cpp_time_ms > 0)
        {
            total_native_overhead += r.pythonic_time_ms / r.cpp_time_ms;
        }
        if (r.python_time_ms > 0)
        {
            double speedup = r.python_time_ms / r.pythonic_time_ms;
            total_python_speedup += speedup;
            if (speedup >= 1.0)
                faster_than_python++;
            else
                slower_than_python++;
        }
        else
        {
            no_python_data++;
        }
    }

    md << "## Summary\n\n";
    md << "| Metric | Value |\n";
    md << "|--------|-------|\n";
    md << "| Total Benchmarks | " << total << " |\n";
    md << "| Faster than Python | " << faster_than_python << " |\n";
    md << "| Slower than Python | " << slower_than_python << " |\n";
    md << "| No Python Data | " << no_python_data << " |\n";
    if (total > 0)
    {
        md << "| Avg Overhead vs Native | " << std::fixed << std::setprecision(2)
           << (total_native_overhead / total) << "x |\n";
    }
    if (faster_than_python + slower_than_python > 0)
    {
        md << "| Avg Speedup vs Python | " << std::fixed << std::setprecision(2)
           << (total_python_speedup / (faster_than_python + slower_than_python)) << "x |\n";
    }
    md << "\n";

    md << "## Detailed Results\n\n";
    md << "| Operation | Native C++ | Pythonic | Python | vs Native | vs Python |\n";
    md << "|-----------|------------|----------|--------|-----------|----------|\n";

    for (const auto &r : results)
    {
        std::string native_cmp = "N/A";
        std::string python_cmp = "No data";

        if (r.cpp_time_ms > 0)
        {
            double overhead = r.pythonic_time_ms / r.cpp_time_ms;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << overhead << "x";
            native_cmp = oss.str();
        }

        if (r.python_time_ms > 0)
        {
            double speedup = r.python_time_ms / r.pythonic_time_ms;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            if (speedup >= 1.0)
            {
                oss << "**" << speedup << "x faster**";
            }
            else
            {
                oss << "*" << (1.0 / speedup) << "x slower*";
            }
            python_cmp = oss.str();
        }

        md << "| " << r.name << " | "
           << format_time(r.cpp_time_ms) << " | "
           << format_time(r.pythonic_time_ms) << " | "
           << (r.python_time_ms > 0 ? format_time(r.python_time_ms) : "N/A") << " | "
           << native_cmp << " | "
           << python_cmp << " |\n";
    }

    md << "\n## Interpretation\n\n";
    md << "- **vs Native**: How much slower Pythonic is compared to native C++. Lower is better.\n";
    md << "- **vs Python**: How much faster Pythonic is compared to Python. Higher is better.\n";
    md << "- Times are in milliseconds (ms) or microseconds (μs).\n\n";
    md << "Pythonic adds abstraction overhead compared to native C++, but aims to be ";
    md << "significantly faster than Python while providing a similar, ergonomic API.\n";

    md.close();
    std::cout << "\n✓ Benchmark report saved to " << filename << "\n";
}

// Global variable for report filename
std::string report_filename = "benchmark_report.md";

int main(int argc, char *argv[])
{
    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--report" && i + 1 < argc)
        {
            report_filename = argv[++i];
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n";
            std::cout << "Options:\n";
            std::cout << "  --report <filename>  Save benchmark report to specified file\n";
            std::cout << "                       (default: benchmark_report.md)\n";
            std::cout << "  --help, -h           Show this help message\n";
            return 0;
        }
    }

    std::cout << "╔══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        Pythonic Library Comprehensive Benchmark Suite            ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Load Python results for comparison
    load_python_results();

    // Run all benchmark categories
    benchmark_arithmetic_operations();
    benchmark_string_operations();
    benchmark_slicing_operations();
    benchmark_container_operations();
    benchmark_loop_operations();
    benchmark_functional_operations();
    benchmark_sorting_operations();
    benchmark_builtin_operations();
    benchmark_conversion_operations();
    benchmark_graph_operations();

    // Print summary report
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                    BENCHMARK SUMMARY REPORT                                                                          ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════╦═══════════════════╦═══════════════════╦═══════════════════╦═══════════════════════╦═══════════════════╣" << std::endl;
    std::cout << "║ Operation                                     ║ Native C++        ║ Pythonic          ║ Python            ║ Pythonic vs Native    ║ Pythonic vs Python║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════╬═══════════════════╬═══════════════════╬═══════════════════╬═══════════════════════╬═══════════════════╣" << std::endl;

    // Statistics
    int total = 0;
    int faster_than_python = 0;
    int slower_than_python = 0;
    int no_python_data = 0;
    double total_native_overhead = 0;
    double total_python_speedup = 0;

    for (const auto &r : results)
    {
        total++;

        // Calculate overhead vs native
        double native_overhead = 0;
        std::string native_comparison;
        if (r.cpp_time_ms > 0)
        {
            native_overhead = r.pythonic_time_ms / r.cpp_time_ms;
            total_native_overhead += native_overhead;
            native_comparison = format_slowdown(native_overhead);
        }
        else
        {
            native_comparison = "N/A";
        }

        // Calculate speedup vs Python
        std::string python_comparison;
        if (r.python_time_ms > 0)
        {
            double python_speedup = r.python_time_ms / r.pythonic_time_ms;
            total_python_speedup += python_speedup;
            if (python_speedup >= 1.0)
            {
                python_comparison = "\033[32m" + std::to_string(python_speedup).substr(0, 5) + "x faster\033[0m";
                faster_than_python++;
            }
            else
            {
                python_comparison = "\033[31m" + std::to_string(1.0 / python_speedup).substr(0, 5) + "x slower\033[0m";
                slower_than_python++;
            }
        }
        else
        {
            python_comparison = "No data";
            no_python_data++;
        }

        // Print row
        std::cout << "║ " << std::left << std::setw(45) << r.name.substr(0, 45) << " ║ "
                  << std::setw(17) << format_time(r.cpp_time_ms) << " ║ "
                  << std::setw(17) << format_time(r.pythonic_time_ms) << " ║ "
                  << std::setw(17) << (r.python_time_ms > 0 ? format_time(r.python_time_ms) : "N/A") << " ║ "
                  << std::setw(21) << native_comparison << " ║ "
                  << std::setw(17) << python_comparison << " ║" << std::endl;
    }

    std::cout << "╚═══════════════════════════════════════════════╩═══════════════════╩═══════════════════╩═══════════════════╩═══════════════════════╩═══════════════════╝" << std::endl;

    // Print statistics
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                         STATISTICS                               ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ Total benchmarks: " << std::setw(47) << total << " ║" << std::endl;
    std::cout << "║ Faster than Python: " << std::setw(45) << faster_than_python << " ║" << std::endl;
    std::cout << "║ Slower than Python: " << std::setw(45) << slower_than_python << " ║" << std::endl;
    std::cout << "║ No Python comparison data: " << std::setw(38) << no_python_data << " ║" << std::endl;

    if (total > 0)
    {
        std::cout << "║ Average overhead vs Native: " << std::setw(37)
                  << (std::to_string(total_native_overhead / total).substr(0, 5) + "x") << " ║" << std::endl;
    }
    if (faster_than_python + slower_than_python > 0)
    {
        std::cout << "║ Average speedup vs Python: " << std::setw(38)
                  << (std::to_string(total_python_speedup / (faster_than_python + slower_than_python)).substr(0, 5) + "x") << " ║" << std::endl;
    }

    std::cout << "╚══════════════════════════════════════════════════════════════════╝" << std::endl;

    std::cout << "\n";
    std::cout << "Note: Pythonic adds abstraction overhead compared to native C++, but aims to be\n";
    std::cout << "significantly faster than Python while providing a similar, ergonomic API.\n";

    // Write markdown report
    write_markdown_report(report_filename);

    return 0;
}
