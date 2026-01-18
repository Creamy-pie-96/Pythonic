#ifndef PYTHONIC_GRAPH_HPP
#define PYTHONIC_GRAPH_HPP

#include <vector>
#include <iostream>
#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include "pythonicError.hpp"
#include <limits>
#include <algorithm>
#include <functional>
#include <optional>

namespace pythonic
{
    namespace graph
    {

        // INF constant for unreachable distances
        inline const double INF = std::numeric_limits<double>::infinity();

        // Min-heap of (distance, node)
        using DistNode = std::pair<double, size_t>;
        using MinHeap = std::priority_queue<DistNode, std::vector<DistNode>, std::greater<DistNode>>;

        /**
         * @brief NODE_data class is for storing the meta data of each node.
         * @tparam T is type of data and Data is the data the node holds
         *
         */
        template <typename T>
        class NODE_data
        {
            T Data;

        public:
            NODE_data() = default;
            NODE_data(const T &data) : Data(data) {};
            void set(const T &data)
            {
                Data = data;
            }
            T &get() { return Data; }
            const T &get() const { return Data; }
        };

        /**
         * @brief Represents an edge in the graph.
         *
         * An edge connects two nodes and may be directed or undirected.
         * Uses double for weight to match Python's float semantics.
         * 
         * @param id Destination node index (the node this edge points to).
         * @param weight Weight of the edge (default is 0.0).
         * @param directed True if the edge is directed, false if undirected.
         */
        struct Edge
        {
            size_t id;
            double weight;
            bool directed;

        public:
            /**
             * @brief Construct an edge.
             * @param id Destination node index.
             * @param weight Edge weight.
             * @param directed True if edge is directed.
             */
            Edge(size_t id, double weight, bool directed) : id(id), weight(weight), directed(directed) {}
        };

        template <typename T>
        /**
         * @brief Generic graph data structure.
         *
         * Graph stores `nodes` nodes indexed 0..nodes-1 and an adjacency list of
         * `Edge` objects. Supports directed or undirected edges, optional weights,
         * and per-node metadata of type `T`.
         *
         * Edge weights use double to match Python's float semantics.
         *
         * @tparam T Type stored as node metadata (e.g. string, int, custom struct).
         */
        class Graph
        {

            bool DAG;                 ///< True if the last added edge was directed (simple flag)
            bool has_negative_weight; ///< True if any edge has negative weight(neeed to check which algorithm to use for shortest path)
            bool is_weighted;         ///< True if the any edge has non 0 weights
            size_t non_zero_edge;
            size_t negative_edges;

            size_t nodes;                                       ///< Number of nodes in the graph
            std::unordered_map<size_t, NODE_data<T>> meta_data; ///< Optional metadata per node
            std::vector<std::vector<Edge>> edges;               ///< Adjacency list: edges[u] is vector of edges from u

            /**
             * @brief Recursive DFS helper.
             *
             * Marks `node` visited and appends it to `result`, then recursively
             * visits all unvisited neighbors.
             *
             * @param visited Vector of visited flags sized `nodes`.
             * @param node Current node index.
             * @param result Traversal order being built.
             */
            void dfs_helper_rec(std::vector<bool> &visited, size_t node, std::vector<size_t> &result)
            {
                visited[node] = true;
                result.push_back(node);
                for (auto &neighbor : edges[node])
                {
                    if (!visited[neighbor.id])
                    {
                        dfs_helper_rec(visited, neighbor.id, result);
                    }
                }
            }

            /**
             * @brief Iterative DFS helper using explicit stack.
             *
             * Uses a stack to mimic recursion. Neighbors are traversed in reverse so
             * the leftmost neighbor (in insertion order) is processed first, matching
             * recursive DFS ordering.
             */
            void dfs_helper_iter(std::vector<bool> &visited, size_t node, std::vector<size_t> &result)
            {
                visited[node] = true;
                std::stack<size_t> s;
                s.push(node);

                while (!s.empty())
                {
                    node = s.top();
                    s.pop();
                    result.push_back(node);
                    for (auto it = edges[node].rbegin(); it != edges[node].rend(); ++it)
                    {
                        if (!visited[it->id])
                        {
                            visited[it->id] = true;
                            s.push(it->id);
                        }
                    }
                }
            }

            std::vector<size_t> reconstruct_path(size_t src, size_t dest, const std::vector<size_t> &prev) const
            {
                std::vector<size_t> path;
                if (dest >= nodes)
                    return path;
                // prev uses (size_t)-1 as sentinel for "no predecessor".
                for (size_t v = dest; v != (size_t)-1; v = prev[v])
                {
                    path.push_back(v);
                    // safety: if prev[v] is out-of-range, break to avoid UB
                    if (prev[v] >= nodes)
                        break;
                }
                std::reverse(path.begin(), path.end());
                if (path.empty() || path.front() != src || path.back() != dest)
                    return {};
                return path;
            }

            std::pair<std::vector<double>, std::vector<size_t>> dijkstra_all(size_t src, size_t dest = -1, bool path_construct = false) const
            {
                std::vector<double> dist(nodes, INF);
                std::vector<size_t> prev(nodes, (size_t)-1);
                MinHeap pq;

                dist[src] = 0.0;
                pq.push({0.0, src});

                while (!pq.empty())
                {
                    auto [d, u] = pq.top();
                    pq.pop();

                    if (d > dist[u])
                        continue;
                    for (const auto &e : edges[u])
                    {
                        double nd = d + e.weight;
                        if (nd < dist[e.id])
                        {
                            dist[e.id] = nd;
                            prev[e.id] = u;
                            pq.push({nd, e.id});
                        }
                    }
                }
                if (!path_construct)
                    return {dist, prev};
                else
                {
                    std::vector<size_t> path = reconstruct_path(src, dest, prev);
                    return {dist, path};
                }
            }

            std::pair<std::vector<size_t>, double> bfs_shortest_path(size_t start, size_t goal)
            {
                if (start >= nodes || goal >= nodes)
                    throw pythonic::PythonicGraphError("invalid start or goal");

                std::vector<bool> visited(nodes, false);
                std::vector<size_t> parent(nodes, (size_t)-1);
                std::vector<size_t> dist(nodes, (size_t)-1);  // distance from start
                std::queue<size_t> q;
                visited[start] = true;
                dist[start] = 0;
                q.push(start);

                while (!q.empty())
                {
                    size_t u = q.front();
                    q.pop();

                    if (u == goal)
                        break;

                    for (auto &neigh : edges[u])
                    {
                        size_t v = neigh.id;
                        if (!visited[v])
                        {
                            visited[v] = true;
                            dist[v] = dist[u] + 1;
                            parent[v] = u;
                            q.push(v);
                        }
                    }
                }

                // Check if goal is reachable
                if (!visited[goal])
                {
                    return {std::vector<size_t>{}, INF};
                }

                // reconstruct path
                std::vector<size_t> path;
                for (size_t v = goal; v != (size_t)-1; v = parent[v])
                    path.push_back(v);

                std::reverse(path.begin(), path.end());
                return {path, static_cast<double>(dist[goal])};
            }

        public:
            /**
             * @brief Construct a graph with `n` nodes (0..n-1).
             * @param n Number of nodes.
             */
            Graph(size_t n)
                : nodes(n)
            {
                edges.resize(nodes);
                DAG = true;
                has_negative_weight = false;
                is_weighted = false;
                non_zero_edge = 0;
                negative_edges = 0;
            }

            /**
             * @brief Add a new node to the graph.
             * @return Index of the newly added node.
             *
             * Usage:
             *   size_t new_node = graph.add_node();
             *   graph.add_edge(new_node, 0);
             */
            size_t add_node()
            {
                edges.emplace_back();
                return nodes++;
            }

            /**
             * @brief Add a new node with metadata.
             * @param data Initial metadata for the node.
             * @return Index of the newly added node.
             */
            size_t add_node(const T &data)
            {
                size_t idx = add_node();
                set_node_data(idx, data);
                return idx;
            }

            /**
             * @brief Get number of nodes (Python-like len() / size()).
             * @return Number of nodes in the graph.
             */
            size_t size() const { return nodes; }

            /**
             * @brief Get neighbor node indices for a given node.
             * @param node Node index.
             * @return Vector of neighbor node indices.
             * @throws PythonicGraphError if node is invalid.
             *
             * Usage:
             *   for (size_t neighbor : graph.neighbors(0)) { ... }
             */
            std::vector<size_t> neighbors(size_t node) const
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("invalid node index: " + std::to_string(node));
                std::vector<size_t> result;
                result.reserve(edges[node].size());
                for (const auto &e : edges[node])
                {
                    result.push_back(e.id);
                }
                return result;
            }

            /**
             * @brief Assign metadata for a node.
             * @param node Node index.
             * @param data Metadata value to set.
             * @throws PythonicError if node is out of range.
             */
            void set_node_data(size_t node, const T &data)
            {
                if (node >= this->nodes)
                {
                    throw pythonic::PythonicGraphError("invalid node");
                }
                meta_data[node] = NODE_data<T>(data);
            }

            /**
             * @brief Get modifiable metadata for a node.
             * @param node Node index.
             * @return Reference to metadata stored for `node`.
             */
            T &get_node_data(size_t node)
            {
                return meta_data[node].get();
            }

            /**
             * @brief Get read-only metadata for a node.
             * @param node Node index.
             * @return Const reference to metadata.
             */
            const T &get_node_data(size_t node) const
            {
                return meta_data.at(node).get();
            }

            /**
             * @brief Subscript operator for accessing node's adjacency list.
             * @param node Node index.
             * @return Reference to vector of edges from this node.
             * @throws PythonicGraphError if node is out of range.
             *
             * Usage:
             *   for (auto& edge : graph[0]) { ... }  // iterate edges from node 0
             */
            std::vector<Edge> &operator[](size_t node)
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("invalid node index: " + std::to_string(node));
                return edges[node];
            }

            /**
             * @brief Const subscript operator for accessing node's adjacency list.
             * @param node Node index.
             * @return Const reference to vector of edges from this node.
             * @throws PythonicGraphError if node is out of range.
             */
            const std::vector<Edge> &operator[](size_t node) const
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("invalid node index: " + std::to_string(node));
                return edges[node];
            }

            /**
             * @brief Change weight of edge (from -> to).
             * @throws PythonicError if edge is not found.
             */
            void set_edge_weight(size_t from, size_t to, double weight)
            {
                for (auto &edge : edges[from])
                {
                    if (edge.id == to)
                    {
                        double old_weight = edge.weight;

                        // Update non_zero_edge counter
                        if (old_weight == 0 && weight != 0)
                        {
                            non_zero_edge++;
                        }
                        else if (old_weight != 0 && weight == 0)
                        {
                            non_zero_edge--;
                        }

                        // Update negative_edges counter
                        if (old_weight < 0 && weight >= 0)
                        {
                            negative_edges--;
                        }
                        else if (old_weight >= 0 && weight < 0)
                        {
                            negative_edges++;
                        }

                        edge.weight = weight;

                        // Update boolean flags based on counters
                        is_weighted = (non_zero_edge > 0);
                        has_negative_weight = (negative_edges > 0);

                        return;
                    }
                }
                throw pythonic::PythonicGraphError::edge_not_found(from, to);
            }

            /**
             * @brief Add an edge between two nodes.
             *
             * For directed edges, only from->to is added. For undirected edges, both
             * directions are added. `w1` is used for the u->v weight; `w2` is used for
             * the reverse v->u when adding an undirected edge (defaults to w1 for symmetric edges).
             * 
             * @param u Source node index.
             * @param v Destination node index.
             * @param w1 Weight for u->v edge (default: 0.0).
             * @param w2 Weight for v->u edge in undirected graphs. If NaN (default), uses w1.
             * @param directional If true, only adds u->v edge. If false, adds both directions.
             */
            void add_edge(size_t u, size_t v, double w1 = 0.0, double w2 = std::numeric_limits<double>::quiet_NaN(), bool directional = false)
            {
                Edge e_uv(v, w1, directional);
                edges[u].push_back(e_uv);
                if (!directional)
                {
                    // For undirected edges, use w1 as the reverse weight if w2 is NaN (not explicitly set)
                    double reverse_weight = std::isnan(w2) ? w1 : w2;
                    Edge e_vu(u, reverse_weight, directional);
                    edges[v].push_back(e_vu);
                }
                DAG = directional;

                // Update counters for the edge(s) we just added
                if (w1 != 0.0)
                {
                    non_zero_edge++;
                    if (w1 < 0)
                        negative_edges++;
                }

                if (!directional)
                {
                    double reverse_weight = std::isnan(w2) ? w1 : w2;
                    if (reverse_weight != 0.0)
                    {
                        non_zero_edge++;
                        if (reverse_weight < 0)
                            negative_edges++;
                    }
                }

                // Update boolean flags
                is_weighted = (non_zero_edge > 0);
                has_negative_weight = (negative_edges > 0);
            }

            /**
             * @brief Return a copy of the adjacency list for `node`.
             * @throws PythonicError if node is invalid.
             */
            std::vector<Edge> get_edges(size_t node)
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("invalid node");
                return edges[node];
            }

            /**
             * @brief Export graph to Graphviz DOT format.
             *
             * Writes a DOT file representing the current graph. Directed edges are
             * written using `->`. Undirected edges are written using `--`.
             *
             * @param filename Path to write the DOT file to.
             * @param show_weights If true, include edge weights in labels.
             */
            void to_dot(const std::string &filename, bool show_weights = true) const
            {
                std::ofstream out(filename);
                if (!out)
                    throw pythonic::PythonicFileError("unable to open file for writing DOT");

                // Determine whether we need a directed graph header. If the graph
                // contains any directed edges we use `digraph` and emit all edges
                // using `->`. Undirected edges are represented once with
                // `dir=none` so they render without arrows. If there are only
                // undirected edges we emit `graph` and use `--`.
                bool has_directed = false;
                bool has_undirected = false;
                for (const auto &vec : edges)
                {
                    for (const auto &e : vec)
                    {
                        if (e.directed)
                            has_directed = true;
                        else
                            has_undirected = true;
                    }
                }

                if (has_directed)
                    out << "digraph G {\n";
                else
                    out << "graph G {\n";
                out << "  node [shape=circle];\n";

                // Write nodes (optionally include labels from metadata if present)
                for (size_t u = 0; u < nodes; ++u)
                {
                    out << "  " << u;
                    auto it = meta_data.find(u);
                    if (it != meta_data.end())
                    {
                        std::ostringstream label;
                        label << it->second.get();
                        out << " [label=\"" << label.str() << "\"]";
                    }
                    out << ";\n";
                }

                // Write edges. For undirected edges we print only once (u < v) to
                // avoid duplicate lines because undirected edges are stored in both
                // adjacency lists.
                for (size_t u = 0; u < nodes; ++u)
                {
                    for (const auto &e : edges[u])
                    {
                        if (has_directed)
                        {
                            // In digraph mode: emit all edges as '->'. For logical
                            // undirected edges, emit only once (u < v) and add
                            // 'dir=none' so no arrow is drawn.
                            if (!e.directed)
                            {
                                if (u > e.id)
                                    continue;
                                out << "  " << u << " -> " << e.id << " [";
                                out << "dir=none";
                                if (show_weights)
                                {
                                    out << ",label=\"" << e.weight << "\"";
                                }
                                out << "]";
                            }
                            else
                            {
                                out << "  " << u << " -> " << e.id;
                                if (show_weights)
                                {
                                    out << " [label=\"" << e.weight << "\"]";
                                }
                            }
                            out << ";\n";
                        }
                        else
                        {
                            // graph (undirected) mode: use '--' and print each
                            // undirected edge only once.
                            if (u > e.id)
                                continue;
                            out << "  " << u << " -- " << e.id;
                            if (show_weights)
                            {
                                out << " [label=\"" << e.weight << "\"]";
                            }
                            out << ";\n";
                        }
                    }
                }

                out << "}\n";
                out.close();
#ifdef GRAPHVIZ_AVAILABLE
                // Use std::filesystem to replace the extension safely
                std::filesystem::path p(filename);
                p.replace_extension(".svg");
                std::string output_file_name = p.string();
                std::string cmd = "dot -Tsvg " + filename + " -o " + output_file_name;
                int rc = system(cmd.c_str());
                if (rc != 0)
                {
                    std::cerr << "Warning: Graphviz 'dot' command failed with code " << rc << "\n";
                }
#endif
            }

            /**
             * @brief Get shortest path between nodes using the optimal algorithm.
             *
             * Algorithm selection:
             * - Unweighted graph: BFS (O(V+E))
             * - Weighted without negative edges: Dijkstra (O((V+E)logV))
             * - Weighted with negative edges: Bellman-Ford (O(VE))
             *
             * @param src Source node index.
             * @param dest Destination node index.
             * @return Pair of (path as vector of node indices, total distance).
             *         Returns ({}, INF) if no path exists.
             */
            std::pair<std::vector<size_t>, double> get_shortest_path(size_t src, size_t dest)
            {
                if (src >= nodes || dest >= nodes)
                {
                    throw pythonic::PythonicGraphError("invalid nodes");
                }

                // Unweighted: use BFS
                if (!is_weighted)
                {
                    return bfs_shortest_path(src, dest);
                }

                // Weighted with negative edges: use Bellman-Ford
                if (has_negative_weight)
                {
                    auto [dist, prev] = bellman_ford(src);
                    if (dist.empty())
                    {
                        throw pythonic::PythonicGraphError("Graph contains a negative cycle");
                    }
                    if (dist[dest] == INF)
                    {
                        return {std::vector<size_t>{}, INF};
                    }
                    std::vector<size_t> path = reconstruct_path(src, dest, prev);
                    return {path, dist[dest]};
                }

                // Weighted without negative edges: use Dijkstra
                auto [dist, path] = dijkstra_all(src, dest, true);
                if (dist[dest] == INF)
                {
                    return {std::vector<size_t>{}, INF};
                }
                return {path, dist[dest]};
            }

            /**
             * @brief Get all-pairs shortest paths using Floyd-Warshall.
             *
             * @return 2D vector where result[i][j] is the shortest distance from i to j.
             *         INF indicates unreachable.
             */
            std::vector<std::vector<double>> get_all_shortest_paths() const
            {
                return floyd_warshall();
            }

            /**
             * @brief Depth-first search traversal.
             * @param start Starting node index.
             * @param reccursion If true use recursive DFS, otherwise iterative.
             * @return Vector with nodes in visited order.
             */
            std::vector<size_t> dfs(size_t start = 0, bool reccursion = true)
            {
                if (start >= nodes)
                {
                    throw pythonic::PythonicGraphError::invalid_node(start);
                }
                std::vector<size_t> result;
                std::vector<bool> visited(nodes, false);
                if (reccursion)
                {
                    dfs_helper_rec(visited, start, result);
                }
                else
                {
                    dfs_helper_iter(visited, start, result);
                }
                return result;
            }

            /**
             * @brief Breadth-first search traversal starting at `node`.
             * @param node Starting node index.
             * @return Vector with nodes in BFS order.
             */
            std::vector<size_t> bfs(size_t node = 0)
            {
                if (node >= nodes)
                {
                    throw pythonic::PythonicGraphError::invalid_node(node);
                }
                std::queue<size_t> q;
                std::vector<size_t> result;
                std::vector<bool> visited(nodes, false);
                visited[node] = true;
                q.push(node);

                while (!q.empty())
                {
                    node = q.front();
                    q.pop();
                    result.push_back(node);
                    for (auto &neighbour : edges[node])
                    {
                        if (!visited[neighbour.id])
                        {
                            q.push(neighbour.id);
                            visited[neighbour.id] = true;
                        }
                    }
                }
                return result;
            }
            // Reserve the same capacity for each node's adjacency list.
            // Useful before bulk-inserting edges to avoid reallocations.
            void reserve_edges_per_node(size_t per_node)
            {
                for (auto &vec : edges)
                    vec.reserve(per_node);
            }

            // Reserve per-node capacities from a precomputed counts vector.
            // counts.size() must equal the number of nodes.
            void reserve_edges_by_counts(const std::vector<size_t> &counts)
            {
                if (counts.size() != edges.size())
                    throw pythonic::PythonicGraphError("reserve_edges_by_counts: counts size mismatch");
                for (size_t i = 0; i < edges.size(); ++i)
                    edges[i].reserve(counts[i]);
            }

            // ==================== BELLMAN-FORD ALGORITHM ====================
            /**
             * @brief Bellman-Ford algorithm for single-source shortest paths.
             *
             * Handles graphs with negative edge weights. Detects negative cycles.
             * Time complexity: O(V * E)
             *
             * @param src Source node index.
             * @return Pair of (distances vector, predecessors vector).
             *         Returns empty vectors if negative cycle is detected.
             */
            std::pair<std::vector<double>, std::vector<size_t>> bellman_ford(size_t src) const
            {
                if (src >= nodes)
                    throw pythonic::PythonicGraphError("bellman_ford: invalid source node");

                std::vector<double> dist(nodes, INF);
                std::vector<size_t> prev(nodes, (size_t)-1);
                dist[src] = 0.0;

                // Relax all edges (V-1) times
                for (size_t i = 0; i < nodes - 1; ++i)
                {
                    bool changed = false;
                    for (size_t u = 0; u < nodes; ++u)
                    {
                        if (dist[u] == INF)
                            continue;
                        for (const auto &e : edges[u])
                        {
                            double nd = dist[u] + e.weight;
                            if (nd < dist[e.id])
                            {
                                dist[e.id] = nd;
                                prev[e.id] = u;
                                changed = true;
                            }
                        }
                    }
                    // Early termination if no changes
                    if (!changed)
                        break;
                }

                // Check for negative cycles
                for (size_t u = 0; u < nodes; ++u)
                {
                    if (dist[u] == INF)
                        continue;
                    for (const auto &e : edges[u])
                    {
                        if (dist[u] + e.weight < dist[e.id])
                        {
                            // Negative cycle detected
                            return {{}, {}};
                        }
                    }
                }

                return {dist, prev};
            }

            // ==================== FLOYD-WARSHALL ALGORITHM ====================
            /**
             * @brief Floyd-Warshall algorithm for all-pairs shortest paths.
             *
             * Time complexity: O(V^3)
             * Space complexity: O(V^2)
             *
             * @return 2D matrix where result[i][j] is the shortest distance from i to j.
             *         INF means unreachable.
             */
            std::vector<std::vector<double>> floyd_warshall() const
            {
                std::vector<std::vector<double>> dist(nodes, std::vector<double>(nodes, INF));

                // Initialize with direct edges
                for (size_t u = 0; u < nodes; ++u)
                {
                    dist[u][u] = 0.0;
                    for (const auto &e : edges[u])
                    {
                        dist[u][e.id] = std::min(dist[u][e.id], e.weight);
                    }
                }

                // Dynamic programming - consider each intermediate node
                for (size_t k = 0; k < nodes; ++k)
                {
                    for (size_t i = 0; i < nodes; ++i)
                    {
                        if (dist[i][k] == INF)
                            continue;
                        for (size_t j = 0; j < nodes; ++j)
                        {
                            if (dist[k][j] == INF)
                                continue;
                            double through_k = dist[i][k] + dist[k][j];
                            if (through_k < dist[i][j])
                            {
                                dist[i][j] = through_k;
                            }
                        }
                    }
                }

                return dist;
            }

            // ==================== CYCLE DETECTION ====================
            /**
             * @brief Detect if the graph contains a cycle.
             *
             * For directed graphs, uses DFS with three-color marking.
             * For undirected graphs, uses DFS checking for back edges.
             *
             * @return true if a cycle exists, false otherwise.
             */
            bool has_cycle() const
            {
                // Use three-color DFS: 0=white(unvisited), 1=gray(in progress), 2=black(done)
                std::vector<int> color(nodes, 0);

                std::function<bool(size_t, size_t)> dfs = [&](size_t u, size_t parent) -> bool
                {
                    color[u] = 1; // Mark as in-progress

                    for (const auto &e : edges[u])
                    {
                        if (color[e.id] == 1)
                        {
                            // Found a back edge - cycle detected
                            // For undirected graphs, skip the edge back to parent
                            if (!e.directed && e.id == parent)
                                continue;
                            return true;
                        }
                        if (color[e.id] == 0)
                        {
                            if (dfs(e.id, u))
                                return true;
                        }
                    }

                    color[u] = 2; // Mark as done
                    return false;
                };

                // Check all components
                for (size_t i = 0; i < nodes; ++i)
                {
                    if (color[i] == 0)
                    {
                        if (dfs(i, (size_t)-1))
                            return true;
                    }
                }

                return false;
            }

            // ==================== TOPOLOGICAL SORT ====================
            /**
             * @brief Topological sort for Directed Acyclic Graphs (DAGs).
             *
             * Returns nodes in topologically sorted order (if node A depends on B,
             * B appears before A in the result).
             *
             * @return Vector of node indices in topological order.
             * @throws PythonicError if graph contains a cycle.
             */
            std::vector<size_t> topological_sort() const
            {
                std::vector<int> in_degree(nodes, 0);

                // Calculate in-degree for each node
                for (size_t u = 0; u < nodes; ++u)
                {
                    for (const auto &e : edges[u])
                    {
                        if (e.directed)
                            in_degree[e.id]++;
                    }
                }

                // Initialize queue with nodes having in-degree 0
                std::queue<size_t> q;
                for (size_t i = 0; i < nodes; ++i)
                {
                    if (in_degree[i] == 0)
                        q.push(i);
                }

                std::vector<size_t> result;
                result.reserve(nodes);

                while (!q.empty())
                {
                    size_t u = q.front();
                    q.pop();
                    result.push_back(u);

                    for (const auto &e : edges[u])
                    {
                        if (e.directed)
                        {
                            in_degree[e.id]--;
                            if (in_degree[e.id] == 0)
                                q.push(e.id);
                        }
                    }
                }

                if (result.size() != nodes)
                    throw pythonic::PythonicGraphError::has_cycle();

                return result;
            }

            // ==================== CONNECTED COMPONENTS ====================
            /**
             * @brief Find all connected components in an undirected graph.
             *
             * @return Vector of vectors, where each inner vector contains node indices
             *         belonging to the same connected component.
             */
            std::vector<std::vector<size_t>> connected_components() const
            {
                std::vector<bool> visited(nodes, false);
                std::vector<std::vector<size_t>> components;

                for (size_t i = 0; i < nodes; ++i)
                {
                    if (!visited[i])
                    {
                        std::vector<size_t> component;
                        std::queue<size_t> q;
                        q.push(i);
                        visited[i] = true;

                        while (!q.empty())
                        {
                            size_t u = q.front();
                            q.pop();
                            component.push_back(u);

                            for (const auto &e : edges[u])
                            {
                                if (!visited[e.id])
                                {
                                    visited[e.id] = true;
                                    q.push(e.id);
                                }
                            }
                        }

                        components.push_back(std::move(component));
                    }
                }

                return components;
            }

            /**
             * @brief Find strongly connected components in a directed graph (Kosaraju's algorithm).
             *
             * @return Vector of vectors, where each inner vector contains node indices
             *         belonging to the same strongly connected component.
             */
            std::vector<std::vector<size_t>> strongly_connected_components() const
            {
                // Step 1: Fill stack with nodes in order of finish time
                std::vector<bool> visited(nodes, false);
                std::stack<size_t> finish_order;

                std::function<void(size_t)> dfs1 = [&](size_t u)
                {
                    visited[u] = true;
                    for (const auto &e : edges[u])
                    {
                        if (!visited[e.id])
                            dfs1(e.id);
                    }
                    finish_order.push(u);
                };

                for (size_t i = 0; i < nodes; ++i)
                {
                    if (!visited[i])
                        dfs1(i);
                }

                // Step 2: Build transpose graph
                std::vector<std::vector<size_t>> transpose(nodes);
                for (size_t u = 0; u < nodes; ++u)
                {
                    for (const auto &e : edges[u])
                    {
                        transpose[e.id].push_back(u);
                    }
                }

                // Step 3: DFS on transpose in reverse finish order
                std::fill(visited.begin(), visited.end(), false);
                std::vector<std::vector<size_t>> sccs;

                std::function<void(size_t, std::vector<size_t> &)> dfs2 = [&](size_t u, std::vector<size_t> &component)
                {
                    visited[u] = true;
                    component.push_back(u);
                    for (size_t v : transpose[u])
                    {
                        if (!visited[v])
                            dfs2(v, component);
                    }
                };

                while (!finish_order.empty())
                {
                    size_t u = finish_order.top();
                    finish_order.pop();
                    if (!visited[u])
                    {
                        std::vector<size_t> component;
                        dfs2(u, component);
                        sccs.push_back(std::move(component));
                    }
                }

                return sccs;
            }

            // ==================== MINIMUM SPANNING TREE ====================
            /**
             * @brief Prim's algorithm for Minimum Spanning Tree.
             *
             * Finds the MST starting from node 0. Works on undirected weighted graphs.
             *
             * @return Pair of (total MST weight, vector of edges in MST as (from, to, weight) tuples).
             */
            std::pair<double, std::vector<std::tuple<size_t, size_t, double>>> prim_mst() const
            {
                if (nodes == 0)
                    return {0.0, {}};

                std::vector<bool> in_mst(nodes, false);
                std::vector<double> key(nodes, INF);
                std::vector<size_t> parent(nodes, (size_t)-1);

                // Min-heap: (weight, node)
                MinHeap pq;
                key[0] = 0.0;
                pq.push({0.0, 0});

                double total_weight = 0.0;
                std::vector<std::tuple<size_t, size_t, double>> mst_edges;

                while (!pq.empty())
                {
                    auto [w, u] = pq.top();
                    pq.pop();

                    if (in_mst[u])
                        continue;

                    in_mst[u] = true;
                    total_weight += w;

                    if (parent[u] != (size_t)-1)
                    {
                        mst_edges.emplace_back(parent[u], u, w);
                    }

                    for (const auto &e : edges[u])
                    {
                        if (!in_mst[e.id] && e.weight < key[e.id])
                        {
                            key[e.id] = e.weight;
                            parent[e.id] = u;
                            pq.push({e.weight, e.id});
                        }
                    }
                }

                return {total_weight, mst_edges};
            }

            // ==================== EDGE REMOVAL ====================
            /**
             * @brief Remove an edge from the graph.
             *
             * @param from Source node.
             * @param to Destination node.
             * @param remove_reverse If true, also removes the reverse edge (for undirected graphs).
             * @return true if edge was found and removed, false otherwise.
             */
            bool remove_edge(size_t from, size_t to, bool remove_reverse = true)
            {
                if (from >= nodes || to >= nodes)
                    return false;

                bool found = false;
                auto &from_edges = edges[from];
                for (auto it = from_edges.begin(); it != from_edges.end();)
                {
                    if (it->id == to)
                    {
                        // Update counters
                        if (it->weight != 0.0)
                        {
                            non_zero_edge--;
                            if (it->weight < 0)
                                negative_edges--;
                        }

                        bool was_undirected = !it->directed;
                        it = from_edges.erase(it);
                        found = true;

                        // Remove reverse edge for undirected edges
                        if (remove_reverse && was_undirected)
                        {
                            auto &to_edges = edges[to];
                            for (auto it2 = to_edges.begin(); it2 != to_edges.end(); ++it2)
                            {
                                if (it2->id == from)
                                {
                                    if (it2->weight != 0.0)
                                    {
                                        non_zero_edge--;
                                        if (it2->weight < 0)
                                            negative_edges--;
                                    }
                                    to_edges.erase(it2);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else
                    {
                        ++it;
                    }
                }

                // Update flags
                is_weighted = (non_zero_edge > 0);
                has_negative_weight = (negative_edges > 0);

                return found;
            }

            // ==================== NODE DEGREE ====================
            /**
             * @brief Get the out-degree of a node (number of outgoing edges).
             */
            size_t out_degree(size_t node) const
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("out_degree: invalid node");
                return edges[node].size();
            }

            /**
             * @brief Get the in-degree of a node (number of incoming edges).
             *
             * Note: O(V + E) as we need to scan all edges.
             */
            size_t in_degree(size_t node) const
            {
                if (node >= nodes)
                    throw pythonic::PythonicGraphError("in_degree: invalid node");

                size_t count = 0;
                for (size_t u = 0; u < nodes; ++u)
                {
                    for (const auto &e : edges[u])
                    {
                        if (e.id == node)
                            count++;
                    }
                }
                return count;
            }

            // ==================== GRAPH PROPERTIES ====================
            /**
             * @brief Get the number of nodes in the graph.
             */
            size_t node_count() const { return nodes; }

            /**
             * @brief Get the total number of edges in the graph.
             */
            size_t edge_count() const
            {
                size_t count = 0;
                for (const auto &adj : edges)
                    count += adj.size();
                return count;
            }

            /**
             * @brief Check if the graph is connected (for undirected graphs).
             */
            bool is_connected() const
            {
                if (nodes == 0)
                    return true;

                std::vector<bool> visited(nodes, false);
                std::queue<size_t> q;
                q.push(0);
                visited[0] = true;
                size_t count = 1;

                while (!q.empty())
                {
                    size_t u = q.front();
                    q.pop();
                    for (const auto &e : edges[u])
                    {
                        if (!visited[e.id])
                        {
                            visited[e.id] = true;
                            count++;
                            q.push(e.id);
                        }
                    }
                }

                return count == nodes;
            }

            /**
             * @brief Check if edge exists between two nodes.
             */
            bool has_edge(size_t from, size_t to) const
            {
                if (from >= nodes || to >= nodes)
                    return false;
                for (const auto &e : edges[from])
                {
                    if (e.id == to)
                        return true;
                }
                return false;
            }

            /**
             * @brief Get the weight of an edge.
             * @return The edge weight, or std::nullopt if edge doesn't exist.
             */
            std::optional<double> get_edge_weight(size_t from, size_t to) const
            {
                if (from >= nodes || to >= nodes)
                    return std::nullopt;
                for (const auto &e : edges[from])
                {
                    if (e.id == to)
                        return e.weight;
                }
                return std::nullopt;
            }

            // ==================== SERIALIZATION ====================
            /**
             * @brief Save graph structure to a file.
             *
             * Format: First line is node count. Following lines are edges as:
             * "from to weight directed"
             *
             * @param filename Path to save the file.
             */
            void save(const std::string &filename) const
            {
                std::ofstream out(filename);
                if (!out)
                    throw pythonic::PythonicFileError("unable to open file for saving");

                out << nodes << "\n";
                for (size_t u = 0; u < nodes; ++u)
                {
                    for (const auto &e : edges[u])
                    {
                        // For undirected edges, only save once (when u < e.id)
                        if (!e.directed && u > e.id)
                            continue;
                        out << u << " " << e.id << " " << e.weight << " " << (e.directed ? 1 : 0) << "\n";
                    }
                }
                out.close();
            }

            /**
             * @brief Load graph structure from a file.
             *
             * @param filename Path to load the file from.
             * @return A new Graph instance.
             */
            static Graph<T> load(const std::string &filename)
            {
                std::ifstream in(filename);
                if (!in)
                    throw pythonic::PythonicFileError("unable to open file for loading");

                size_t n;
                in >> n;
                Graph<T> g(n);

                size_t from, to;
                double weight;
                int directed;
                while (in >> from >> to >> weight >> directed)
                {
                    g.add_edge(from, to, weight, weight, directed == 1);
                }

                return g;
            }
        };

        /*
        Graph 1:


            0
           / \
          1   2
         / \
        3   4

        graph 2:

            A(0)
           /   \
         B(1)   C(2)
           \    /
            D(3)


        graph 3:

            X(0)
           /   \
         Y(1)   Z(2)
           |
           W(3)


        */

        /*
        Later I will implement more algorithms:

        1.
        * Shortest path (Dijkstra’s, Bellman-Ford)
        * Minimum spanning tree (Kruskal’s, Prim’s)
        * Cycle detection (DFS for directed/undirected)
        * Topological sort (for DAGs)
        * Connected components (BFS/DFS)
        * Add more features:

        2.
        * Remove edge/node methods
        * Support for weighted undirected/directed graphs in algorithms
        * Serialization (save/load graph to file)
        * [x] Visualization (output to Graphviz format)
        * Performance and robustness:

        3.
        * Stress test with large graphs
        * Benchmark DFS/BFS performance
        * Add more error handling and edge cases
          Unit tests:

          4.
        * Write tests for new algorithms and features
        * Test edge cases (self-loops, disconnected graphs, etc.)
        * Try different graph representations:

        5.
        * Adjacency matrix
        * Edge list
        */

    } // namespace graph
} // namespace pythonic

#endif // PYTHONIC_GRAPH_HPP