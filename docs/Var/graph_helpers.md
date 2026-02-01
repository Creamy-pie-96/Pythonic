[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Var Table of Contents](var.md)
[⬅ Back to Iterators, Mapping & Functional Helpers](iterators_mapping_functional.md)

# Graph Helpers

This page documents all user-facing graph APIs for `var` when holding a graph, in a clear, tabular format with concise, non-redundant examples. Multi-step and real-world examples are at the end.

---

## Graph API Reference

### Properties & Queries

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

### Node & Edge Manipulation

| Method                                                                                 | Description                             | Example                         |
| -------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------- |
| `size_t add_node()`                                                                    | Add empty node, return id               | `g.add_node()`                  |
| `size_t add_node(const var &data)`                                                     | Add node with data, return id           | `g.add_node("foo")`             |
| `void remove_node(size_t node)`                                                        | Remove node by id                       | `g.remove_node(0)`              |
| `void add_edge(size_t u, size_t v, bool directed=false, double w1=0.0, double w2=NaN)` | Add edge (optionally directed/weighted) | `g.add_edge(0, 1, true)`        |
| `bool remove_edge(size_t from, size_t to, bool remove_reverse=true)`                   | Remove edge (optionally reverse)        | `g.remove_edge(0, 1)`           |
| `void set_edge_weight(size_t from, size_t to, double weight)`                          | Set edge weight                         | `g.set_edge_weight(0, 1, 3.14)` |
| `void set_node_data(size_t node, const var &data)`                                     | Set node data                           | `g.set_node_data(0, "foo")`     |
| `var &get_node_data(size_t node)`                                                      | Get node data (reference)               | `g.get_node_data(0)`            |

### Algorithms

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

### File/Export & Visualization

| Method                                                             | Description                                | Example                       |
| ------------------------------------------------------------------ | ------------------------------------------ | ----------------------------- |
| `void save_graph(const std::string &filename)`                     | Save graph to file                         | `g.save_graph("g.txt")`       |
| `void to_dot(const std::string &filename, bool show_weights=true)` | Export to Graphviz DOT                     | `g.to_dot("g.dot")`           |
| `void show(bool layout=true)`                                      | Show interactive graph viewer (if enabled) | `g.show()` or `g.show(false)` |

---

## Interactive Graph Viewer

The interactive viewer lets you visualize, edit, and explore graphs in real time (requires build with `-DPYTHONIC_ENABLE_GRAPH_VIEWER=ON`).

**Usage:**

```cpp
g.show();           // auto-layout
g.show(false);      // preserve node positions
```

**Features:**

- View: drag nodes, hover for metadata, click for animation
- Edit: double-click to add nodes, drag to add edges, select+delete to remove
- All changes are saved back to your `var` graph

---

## Examples

### Basic Graph Creation & Manipulation

```cpp
var g = graph();
auto n0 = g.add_node("A");
auto n1 = g.add_node("B");
g.add_edge(n0, n1, 1.5);
g.set_node_data(n0, "Alpha");
print(g.node_count()); // 2
print(g.edge_count()); // 1
print(g.has_edge(n0, n1)); // true
print(g.get_edge_weight(n0, n1)); // 1.5
print(g.get_node_data(n0)); // "Alpha"
print(g.dfs(n0)); // DFS traversal from node n0
g.save_graph("g.txt");
g.to_dot("g.dot");
```

### Graph Properties & Traversals

```cpp
var g = graph(5);
g.add_edge(0, 1);
g.add_edge(1, 2);
g.add_edge(2, 3);
g.add_edge(3, 4);
print(g.is_connected());  // true
print(g.has_cycle());     // false
g.add_edge(4, 0);
print(g.has_cycle());     // true
print(g.out_degree(0));   // outgoing edges from node 0
print(g.in_degree(0));    // incoming edges to node 0
print(g.neighbors(0));    // neighbors of node 0
print(g.get_edges(0));    // edges from node 0
```

### Shortest Paths & Algorithms

```cpp
var g = graph(5);
g.add_edge(0, 1, 4.0);
g.add_edge(0, 2, 1.0);
g.add_edge(1, 3, 1.0);
g.add_edge(2, 1, 2.0);
g.add_edge(2, 3, 5.0);
g.add_edge(3, 4, 3.0);
var result = g.get_shortest_path(0, 4);
print(result["path"]);      // [0, 2, 1, 3, 4]
print(result["distance"]);  // 7.0
var bf = g.bellman_ford(0);
print(bf["distances"]);     // [0, 3, 1, 4, 7]
var fw = g.floyd_warshall();
for_each(row, fw) print(row);
```

### Components, Topological Sort, MST

```cpp
var dag = graph(4);
dag.add_edge(0, 1, true, 1.0);  // directed edge
dag.add_edge(0, 2, true, 1.0);  // directed edge
dag.add_edge(1, 3, true, 1.0);  // directed edge
dag.add_edge(2, 3, true, 1.0);  // directed edge
print(dag.topological_sort());

var network = graph(6);
network.add_edge(0, 1);
network.add_edge(1, 2);
network.add_edge(3, 4);
network.add_edge(4, 5);
print(network.connected_components());

var city_network = graph(4);
city_network.add_edge(0, 1, 1.0);
city_network.add_edge(0, 2, 4.0);
city_network.add_edge(1, 2, 2.0);
city_network.add_edge(1, 3, 5.0);
city_network.add_edge(2, 3, 3.0);
var mst = city_network.prim_mst();
print(mst["weight"]);  // 6.0
print(mst["edges"]);   // MST edge list
```

### Serialization & Loading

```cpp
var g = graph(3);
g.add_edge(0, 1, 1.5);
g.add_edge(1, 2, 2.5);
g.set_node_data(0, "Start");
g.save_graph("my_graph.txt");
var loaded = load_graph("my_graph.txt");
print(loaded.str());
g.to_dot("my_graph.dot");
```

### Performance Optimization

```cpp
var g = graph(1000);
g.reserve_edges_per_node(10);
for (size_t i = 0; i < 1000; i++)
	for (int j = 0; j < 10; j++)
		g.add_edge(i, (i + j + 1) % 1000, 1.0);
var counts = list(5, 10, 3, 8, 2);
var custom = graph(5);
custom.reserve_edges_by_counts(counts);
```

---

## Notes

- All graph helpers are only available when `var` holds a graph.
- Most methods return `var` or standard types; see API for details.
- For advanced graph algorithms, see the full API or source.
- `show(layout)` — If `layout` is true (default), the viewer will automatically arrange the graph layout. If false, the current node positions are preserved.

## Example for you:

```cpp
// Test file for all graph_helpers.md documentation examples
#include <pythonic/pythonic.hpp>
#include <iostream>
using namespace py;

int main()
{
    // Basic Graph Creation & Manipulation
    {
        var g = graph(0);
        auto n0 = g.add_node("A");
        auto n1 = g.add_node("B");
        g.add_edge(n0, n1, 1.5);
        g.set_node_data(n0, "Alpha");
        std::cout << g.node_count() << std::endl;
        std::cout << g.edge_count() << std::endl;
        std::cout << g.has_edge(n0, n1) << std::endl;
        std::cout << g.get_edge_weight(n0, n1).value_or(-1) << std::endl;
        std::cout << g.get_node_data(n0) << std::endl;
        std::cout << g.dfs(n0) << std::endl;
        g.save_graph("/tmp/g.txt");
        g.to_dot("/tmp/g.dot");
    }
    // Graph Properties & Traversals
    {
        var g = graph(5);
        g.add_edge(0, 1);
        g.add_edge(1, 2);
        g.add_edge(2, 3);
        g.add_edge(3, 4);
        std::cout << g.is_connected() << std::endl;
        std::cout << g.has_cycle() << std::endl;
        g.add_edge(4, 0);
        std::cout << g.has_cycle() << std::endl;
        std::cout << g.out_degree(0) << std::endl;
        std::cout << g.in_degree(0) << std::endl;
        std::cout << g.neighbors(0) << std::endl;
        std::cout << g.get_edges(0) << std::endl;
    }
    // Shortest Paths & Algorithms
    {
        var g = graph(5);
        g.add_edge(0, 1, 4.0);
        g.add_edge(0, 2, 1.0);
        g.add_edge(1, 3, 1.0);
        g.add_edge(2, 1, 2.0);
        g.add_edge(2, 3, 5.0);
        g.add_edge(3, 4, 3.0);
        var result = g.get_shortest_path(0, 4);
        std::cout << result["path"] << std::endl;
        std::cout << result["distance"] << std::endl;
        var bf = g.bellman_ford(0);
        std::cout << bf["distances"] << std::endl;
        var fw = g.floyd_warshall();
        for (const auto &row : fw)
            std::cout << row << std::endl;
    }
    // Components, Topological Sort, MST
    {
        var dag = graph(4);
        dag.add_edge(0, 1, 1.0, 0.0, true);
        dag.add_edge(0, 2, 1.0, 0.0, true);
        dag.add_edge(1, 3, 1.0, 0.0, true);
        dag.add_edge(2, 3, 1.0, 0.0, true);
        std::cout << dag.topological_sort() << std::endl;
        var network = graph(6);
        network.add_edge(0, 1);
        network.add_edge(1, 2);
        network.add_edge(3, 4);
        network.add_edge(4, 5);
        std::cout << network.connected_components() << std::endl;
        var city_network = graph(4);
        city_network.add_edge(0, 1, 1.0);
        city_network.add_edge(0, 2, 4.0);
        city_network.add_edge(1, 2, 2.0);
        city_network.add_edge(1, 3, 5.0);
        city_network.add_edge(2, 3, 3.0);
        var mst = city_network.prim_mst();
        std::cout << mst["weight"] << std::endl;
        std::cout << mst["edges"] << std::endl;
    }
    // Serialization & Loading
    {
        var g = graph(3);
        g.add_edge(0, 1, 1.5);
        g.add_edge(1, 2, 2.5);
        g.set_node_data(0, "Start");
        g.save_graph("/tmp/my_graph.txt");
        var loaded = load_graph("/tmp/my_graph.txt");
        std::cout << loaded.str() << std::endl;
        g.to_dot("/tmp/my_graph.dot");
    }
    // Performance Optimization
    {
        var g = graph(1000);
        g.reserve_edges_per_node(10);
        for (size_t i = 0; i < 1000; i++)
            for (int j = 0; j < 10; j++)
                g.add_edge(i, (i + j + 1) % 1000, 1.0);
        var counts = list(5, 10, 3, 8, 2);
        var custom = graph(5);
        custom.reserve_edges_by_counts(counts);
    }
    return 0;
}
```

## Next check

- [Math](../Math/math.md)
