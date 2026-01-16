#pragma once
#include "benchmark_common.hpp"

/**
 * Benchmarks for container creation and operations.
 */

inline void benchmark_container_operations()
{
    std::cout << "\n=== Benchmarking Container Operations ===" << std::endl;

    // List creation
    run_benchmark("List Creation", []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
                (void)v;
            } }, []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                var v = list(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
                (void)v;
            } });

    // Dict creation
    run_benchmark("Dict Creation", []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                std::map<std::string, int> m;
                m["a"] = 1;
                m["b"] = 2;
                m["c"] = 3;
                (void)m;
            } }, []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                var d = dict();
                d["a"] = 1;
                d["b"] = 2;
                d["c"] = 3;
                (void)d;
            } });

    // Set creation
    run_benchmark("Set Creation", []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                std::set<int> s = {1, 2, 3, 4, 5};
                (void)s;
            } }, []()
                  {
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                var s = set(1, 2, 3, 4, 5);
                (void)s;
            } });

    // List append
    run_benchmark("List append()", []()
                  {
            std::vector<int> v;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                v.push_back(static_cast<int>(i)); }, []()
                  {
            var v = list();
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                v.append(var(static_cast<long long>(i))); });

    // List extend()
    run_benchmark("List extend()", []()
                  {
            std::vector<int> v1;
            std::vector<int> v2 = {1, 2, 3, 4, 5};
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                v1.insert(v1.end(), v2.begin(), v2.end());
            (void)v1; }, []()
                  {
            var v1 = list();
            var v2 = list(1, 2, 3, 4, 5);
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                v1.extend(v2); });

    // List index access
    run_benchmark("List Index Access", []()
                  {
            std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            int result = 0;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = v[i % 10];
            (void)result; }, []()
                  {
            var v = list(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
            var result;
            for (size_t i = 0; i < ITERATIONS; ++i)
                result = v[i % 10]; });

    // Dict access
    run_benchmark("Dict Access", []()
                  {
            std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
            int result = 0;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = m["b"];
            (void)result; }, []()
                  {
            var d = dict();
            d["a"] = 1;
            d["b"] = 2;
            d["c"] = 3;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = d["b"]; });

    // Dict keys()
    run_benchmark("Dict keys()", []()
                  {
            std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
            std::vector<std::string> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (const auto& p : m)
                    result.push_back(p.first);
            }
            (void)result; }, []()
                  {
            var d = dict();
            d["a"] = 1;
            d["b"] = 2;
            d["c"] = 3;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = d.keys(); });

    // Dict values()
    run_benchmark("Dict values()", []()
                  {
            std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (const auto& p : m)
                    result.push_back(p.second);
            }
            (void)result; }, []()
                  {
            var d = dict();
            d["a"] = 1;
            d["b"] = 2;
            d["c"] = 3;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = d.values(); });

    // Dict items()
    run_benchmark("Dict items()", []()
                  {
            std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
            std::vector<std::pair<std::string, int>> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (const auto& p : m)
                    result.push_back(p);
            }
            (void)result; }, []()
                  {
            var d = dict();
            d["a"] = 1;
            d["b"] = 2;
            d["c"] = 3;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = d.items(); });

    // Set add()
    run_benchmark("Set add()", []()
                  {
            std::set<int> s;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                s.insert(static_cast<int>(i)); }, []()
                  {
            var s = set();
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                s.add(var(static_cast<long long>(i))); });

    // 'in' operator for list
    run_benchmark("'in' Operator (List)", []()
                  {
            std::vector<int> v;
            for (int i = 0; i < 100; ++i) v.push_back(i);
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = (std::find(v.begin(), v.end(), 50) != v.end());
            (void)result; }, []()
                  {
            var v = list();
            for (int i = 0; i < 100; ++i) v.append(i);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v.contains(50); });

    // 'in' operator for set
    run_benchmark("'in' Operator (Set)", []()
                  {
            std::set<int> s;
            for (int i = 0; i < 100; ++i) s.insert(i);
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = (s.find(50) != s.end());
            (void)result; }, []()
                  {
            var s = set();
            for (int i = 0; i < 100; ++i) s.add(i);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = s.contains(50); });

    // 'in' operator for dict
    run_benchmark("'in' Operator (Dict)", []()
                  {
            std::map<std::string, int> m;
            for (int i = 0; i < 100; ++i) m["key" + std::to_string(i)] = i;
            bool result = false;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = (m.find("key50") != m.end());
            (void)result; }, []()
                  {
            var d = dict();
            for (int i = 0; i < 100; ++i) d["key" + std::to_string(i)] = i;
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = d.contains("key50"); });

    // List + operator
    run_benchmark("List + Operator", []()
                  {
            std::vector<int> v1 = {1, 2, 3, 4, 5};
            std::vector<int> v2 = {6, 7, 8, 9, 10};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result = v1;
                result.insert(result.end(), v2.begin(), v2.end());
            }
            (void)result; }, []()
                  {
            var v1 = list(1, 2, 3, 4, 5);
            var v2 = list(6, 7, 8, 9, 10);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v1 + v2; });

    // List * operator
    run_benchmark("List * Operator", []()
                  {
            std::vector<int> v = {1, 2, 3};
            std::vector<int> result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i) {
                result.clear();
                for (int j = 0; j < 5; ++j)
                    result.insert(result.end(), v.begin(), v.end());
            }
            (void)result; }, []()
                  {
            var v = list(1, 2, 3);
            var result;
            for (size_t i = 0; i < SMALL_ITERATIONS; ++i)
                result = v * 5; });
}
