[⬅ Back to Table of Contents](var.md)

# Graph Helpers

This page documents all user-facing graph APIs for `var` when holding a graph, in a clear tabular format with concise examples.

---

## Graph Properties & Queries

| Method                                                          | Description                  | Example                   |
| --------------------------------------------------------------- | ---------------------------- | ------------------------- |
| `size_t node_count()`                                           | Number of nodes              | `g.node_count()`          |
| `size_t edge_count()`                                           | Number of edges              | `g.edge_count()`          |
| `bool is_connected()`                                           | Is the graph connected?      | `g.is_connected()`        |
| `bool has_cycle()`                                              | Does the graph have a cycle? | `g.has_cycle()`           |
| `bool has_edge(size_t from, size_t to)`                         | Is there an edge from→to?    | `g.has_edge(0, 1)`        |
| `std::optional<double> get_edge_weight(size_t from, size_t to)` | Get edge weight (if any)     | `g.get_edge_weight(0, 1)` |
| `size_t out_degree(size_t node)`                                | Out-degree of node           | `g.out_degree(0)`         |
| `size_t in_degree(size_t node)`                                 | In-degree of node            | `g.in_degree(0)`          |
| `var neighbors(size_t node)`                                    | List of neighbors for node   | `g.neighbors(0)`          |
| `var get_edges(size_t node)`                                    | List of edges for node       | `g.get_edges(0)`          |

---

## Node & Edge Manipulation

| Method                                                                                 | Description                             | Example                         |
| -------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------- |
| `size_t add_node()`                                                                    | Add empty node, return id               | `g.add_node()`                  |
| `size_t add_node(const var &data)`                                                     | Add node with data, return id           | `g.add_node("foo")`             |
| `void remove_node(size_t node)`                                                        | Remove node by id                       | `g.remove_node(0)`              |
| `void add_edge(size_t u, size_t v, double w1=0.0, double w2=NaN, bool directed=false)` | Add edge (optionally weighted/directed) | `g.add_edge(0, 1, 2.5)`         |
| `bool remove_edge(size_t from, size_t to, bool remove_reverse=true)`                   | Remove edge (optionally reverse)        | `g.remove_edge(0, 1)`           |
| `void set_edge_weight(size_t from, size_t to, double weight)`                          | Set edge weight                         | `g.set_edge_weight(0, 1, 3.14)` |
| `void set_node_data(size_t node, const var &data)`                                     | Set node data                           | `g.set_node_data(0, "foo")`     |
| `var &get_node_data(size_t node)`                                                      | Get node data (reference)               | `g.get_node_data(0)`            |

---

## Graph Algorithms

| Method                                           | Description                             | Example                             |
| ------------------------------------------------ | --------------------------------------- | ----------------------------------- |
| `var dfs(size_t start=0, bool recursive=true)`   | Depth-first search                      | `g.dfs(0)`                          |
| `var bfs(size_t start=0)`                        | Breadth-first search                    | `g.bfs(0)`                          |
| `var get_shortest_path(size_t src, size_t dest)` | Shortest path (Dijkstra)                | `g.get_shortest_path(0, 2)`         |
| `var bellman_ford(size_t src)`                   | Bellman-Ford shortest paths             | `g.bellman_ford(0)`                 |
| `var floyd_warshall()`                           | Floyd-Warshall all-pairs shortest paths | `g.floyd_warshall()`                |
| `var topological_sort()`                         | Topological sort (DAGs)                 | `g.topological_sort()`              |
| `var connected_components()`                     | List of connected components            | `g.connected_components()`          |
| `var strongly_connected_components()`            | List of strongly connected components   | `g.strongly_connected_components()` |
| `var prim_mst()`                                 | Minimum spanning tree (Prim's)          | `g.prim_mst()`                      |

---

## File/Export & Visualization Helpers

| Method                                                             | Description                                                                                                 | Example                       |
| ------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------- | ----------------------------- |
| `void save_graph(const std::string &filename)`                     | Save graph to file                                                                                          | `g.save_graph("g.txt")`       |
| `void to_dot(const std::string &filename, bool show_weights=true)` | Export to Graphviz DOT                                                                                      | `g.to_dot("g.dot")`           |
| `void show(bool layout=true)`                                      | Show interactive graph viewer (if enabled).<br>`layout=true` enables automatic layout, `false` disables it. | `g.show()`<br>`g.show(false)` |

---

## Examples

```cpp
#include "pythonic/pythonic.hpp"
using namespace py;

// Create a graph and add nodes/edges
var g = graph();
auto n0 = g.add_node("A");
auto n1 = g.add_node("B");
g.add_edge(n0, n1, 1.5);
print(g.node_count()); // 2
print(g.edge_count()); // 1
print(g.has_edge(n0, n1)); // true
print(g.get_edge_weight(n0, n1)); // 1.5
g.set_node_data(n0, "Alpha");
print(g.get_node_data(n0)); // "Alpha"
print(g.dfs(n0)); // DFS traversal from node n0
print(g); // prints the graph
g.save_graph("g.txt");
g.to_dot("g.dot");
g.show();
g.show(false);
```

---

## Notes

- All graph helpers are only available when `var` holds a graph.
- Most methods return `var` or standard types; see API for details.
- For advanced graph algorithms, see the full API or source.

- `show(layout)` — If `layout` is true (default), the viewer will automatically arrange the graph layout. If false, the current node positions are preserved.
