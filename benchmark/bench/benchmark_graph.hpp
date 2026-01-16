#pragma once
#include "benchmark_common.hpp"
#include <queue>
#include <stack>

/**
 * Benchmarks for graph operations.
 */

namespace graph_bench
{
    constexpr size_t GRAPH_ITERATIONS = 1000;
    constexpr size_t NUM_NODES = 6;

    inline std::map<size_t, std::vector<std::pair<size_t, double>>> build_native_graph()
    {
        std::map<size_t, std::vector<std::pair<size_t, double>>> adj;
        adj[0].push_back({1, 1.0}); // A -> B
        adj[0].push_back({2, 1.0}); // A -> C
        adj[1].push_back({3, 1.0}); // B -> D
        adj[1].push_back({4, 1.0}); // B -> E
        adj[2].push_back({5, 1.0}); // C -> F
        adj[3] = {};
        adj[4] = {};
        adj[5] = {};
        return adj;
    }

    inline var build_var_graph()
    {
        var g = graph(NUM_NODES);
        g.add_edge(0, 1, 1.0); // A -> B
        g.add_edge(0, 2, 1.0); // A -> C
        g.add_edge(1, 3, 1.0); // B -> D
        g.add_edge(1, 4, 1.0); // B -> E
        g.add_edge(2, 5, 1.0); // C -> F
        return g;
    }
}

inline void benchmark_graph_operations()
{
    using namespace graph_bench;
    std::cout << "\n=== Benchmarking Graph Operations ===" << std::endl;

    // Graph creation
    run_benchmark("Graph Creation", []()
                  {
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                std::map<size_t, std::vector<std::pair<size_t, double>>> adj;
                (void)adj;
            } }, []()
                  {
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                var g = graph(NUM_NODES);
                (void)g;
            } });

    // add_edge (uses numeric nodes)
    run_benchmark("add_edge()", []()
                  {
            std::map<size_t, std::vector<std::pair<size_t, double>>> adj;
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                adj[0].push_back({1, 1.0});
            }
            (void)adj; }, []()
                  {
            var g = graph(NUM_NODES);
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                g.add_edge(0, 1, 1.0);
            } });

    // DFS traversal
    run_benchmark("DFS Traversal", []()
                  {
            auto adj = build_native_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                std::vector<size_t> result;
                std::set<size_t> visited;
                std::stack<size_t> st;
                st.push(0);
                while (!st.empty()) {
                    size_t node = st.top();
                    st.pop();
                    if (visited.count(node)) continue;
                    visited.insert(node);
                    result.push_back(node);
                    auto it = adj.find(node);
                    if (it != adj.end()) {
                        for (auto rit = it->second.rbegin(); rit != it->second.rend(); ++rit)
                            st.push(rit->first);
                    }
                }
                (void)result;
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                var result = g.dfs(0);
                (void)result;
            } });

    // BFS traversal
    run_benchmark("BFS Traversal", []()
                  {
            auto adj = build_native_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                std::vector<size_t> result;
                std::set<size_t> visited;
                std::queue<size_t> q;
                q.push(0);
                visited.insert(0);
                while (!q.empty()) {
                    size_t node = q.front();
                    q.pop();
                    result.push_back(node);
                    auto it = adj.find(node);
                    if (it != adj.end()) {
                        for (const auto& edge : it->second) {
                            if (!visited.count(edge.first)) {
                                visited.insert(edge.first);
                                q.push(edge.first);
                            }
                        }
                    }
                }
                (void)result;
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                var result = g.bfs(0);
                (void)result;
            } });

    // has_edge
    run_benchmark("has_edge()", []()
                  {
            auto adj = build_native_graph();
            bool result = false;
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                auto it = adj.find(0);
                if (it != adj.end()) {
                    for (const auto& edge : it->second) {
                        if (edge.first == 1) {
                            result = true;
                            break;
                        }
                    }
                }
            }
            (void)result; }, []()
                  {
            var g = build_var_graph();
            var result;
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter)
                result = g.has_edge(0, 1); });

    // Shortest path (Dijkstra)
    run_benchmark("get_shortest_path()", []()
                  {
            auto adj = build_native_graph();
            for (size_t iter = 0; iter < 100; ++iter) {
                std::map<size_t, double> dist;
                std::map<size_t, size_t> prev;
                std::priority_queue<std::pair<double, size_t>,
                                  std::vector<std::pair<double, size_t>>,
                                  std::greater<>> pq;
                dist[0] = 0;
                pq.push({0, 0});
                while (!pq.empty()) {
                    auto [d, u] = pq.top();
                    pq.pop();
                    if (dist.count(u) && d > dist[u]) continue;
                    auto it = adj.find(u);
                    if (it != adj.end()) {
                        for (const auto& [v, w] : it->second) {
                            if (dist.find(v) == dist.end() || dist[u] + w < dist[v]) {
                                dist[v] = dist[u] + w;
                                prev[v] = u;
                                pq.push({dist[v], v});
                            }
                        }
                    }
                }
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < 100; ++iter) {
                var path = g.get_shortest_path(0, 5);
                (void)path;
            } });

    // is_connected
    run_benchmark("is_connected()", []()
                  {
            auto adj = build_native_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                std::set<size_t> visited;
                std::queue<size_t> q;
                if (!adj.empty()) {
                    q.push(adj.begin()->first);
                    visited.insert(adj.begin()->first);
                }
                while (!q.empty()) {
                    size_t node = q.front();
                    q.pop();
                    auto it = adj.find(node);
                    if (it != adj.end()) {
                        for (const auto& edge : it->second) {
                            if (!visited.count(edge.first)) {
                                visited.insert(edge.first);
                                q.push(edge.first);
                            }
                        }
                    }
                }
                bool connected = (visited.size() == adj.size());
                (void)connected;
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                var result = g.is_connected();
                (void)result;
            } });

    // has_cycle
    run_benchmark("has_cycle()", []()
                  {
            // Native implementation complex - placeholder
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                // Placeholder
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < GRAPH_ITERATIONS; ++iter) {
                var result = g.has_cycle();
                (void)result;
            } });

    // topological_sort
    run_benchmark("topological_sort()", []()
                  {
            // Native implementation complex - placeholder
            for (size_t iter = 0; iter < 100; ++iter) {
                // Placeholder
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < 100; ++iter) {
                var result = g.topological_sort();
                (void)result;
            } });

    // connected_components
    run_benchmark("connected_components()", []()
                  {
            // Native implementation complex - placeholder
            for (size_t iter = 0; iter < 100; ++iter) {
                // Placeholder
            } }, []()
                  {
            var g = build_var_graph();
            for (size_t iter = 0; iter < 100; ++iter) {
                var result = g.connected_components();
                (void)result;
            } });
}
