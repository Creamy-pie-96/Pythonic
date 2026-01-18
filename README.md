# Pythonic C++ Library

Hey there! So you love Python's clean syntax but need C++'s performance? You're in the right place.

This library brings Python's most beloved features to C++ - dynamic typing, easy containers, slicing, string methods, comprehensions, and tons more. No weird hacks, just modern C++20 that feels surprisingly Pythonic.

## Table of Contents

- [Installation & Build](#installation--build)

- [Quick Start](#quick-start)
  - [The Easy Way (Recommended)](#the-easy-way-recommended)
  - [The Manual Way](#the-manual-way)
- [The `var` Type](#the-var-type)
  - [Type Checking & Conversion](#type-checking--conversion)
- [Containers](#containers)
  - [Lists](#lists)
  - [Slicing](#slicing-finally)
  - [Dicts](#dicts)
  - [Sets](#sets)
  - [OrderedSet (Sorted Set)](#orderedset-sorted-set)
  - [OrderedDict (Sorted Dictionary)](#ordereddict-sorted-dictionary)
  - [Truthiness (Python-style Boolean Context)](#truthiness-python-style-boolean-context)
  - [None Values](#none-values)
- [Graph Data Structure](#graph-data-structure)
  - [Creating Graphs](#creating-graphs)
  - [Adding Edges](#adding-edges)
  - [Node Data](#node-data)
  - [Graph Properties](#graph-properties)
  - [Graph Traversals](#graph-traversals)
  - [Shortest Paths](#shortest-paths)
  - [Graph Algorithms](#graph-algorithms)
  - [Graph Serialization](#graph-serialization)
  - [Graph Performance Optimization](#graph-performance-optimization)
- [String Methods](#string-methods)
- [Comprehensive Math Library](#comprehensive-math-library)
  - [Basic Math Operations](#basic-math-operations)
  - [Trigonometric Functions](#trigonometric-functions)
  - [Hyperbolic Functions](#hyperbolic-functions)
  - [Angle Conversion](#angle-conversion)
  - [Mathematical Constants](#mathematical-constants)
  - [Random Functions](#random-functions)
  - [Product](#product-multiply-all-elements)
  - [Advanced Math Functions](#advanced-math-functions)
  - [Aggregation Functions](#aggregation-functions-from-pythonicvars)
- [List Comprehensions](#list-comprehensions)
- [Lambda Helpers](#lambda-helpers)
- [Iteration](#iteration)
  - [Range](#range)
  - [Enumerate](#enumerate)
  - [Zip](#zip)
  - [Reversed](#reversed)
  - [Loop Macros](#loop-macros)
- [Functional Programming](#functional-programming)
  - [Map, Filter, Reduce](#map-filter-reduce)
  - [Sorting](#sorting)
  - [Advanced Functional Utilities](#advanced-functional-utilities)
  - [All/Any Predicates](#allany-predicates)
- [Global Variable Table (let macro)](#global-variable-table-let-macro)
- [Printing & Formatting](#printing--formatting)
- [User Input](#user-input)
- [File I/O](#file-io)
- [Operator Overloading](#operator-overloading)
- [C++20 Features](#c20-features)
  - [Concepts](#concepts)
  - [Ranges Integration](#ranges-integration)
  - [Fast Path Cache](#fast-path-cache-hot-loop-optimization)
- [Error Handling](#error-handling)
  - [Exception Hierarchy](#exception-hierarchy)
  - [Using Exceptions](#using-exceptions)
- [Checked Arithmetic](#checked-arithmetic)
- [What's Under the Hood?](#whats-under-the-hood)
- [Examples](#examples)
- [Tips & Tricks](#tips--tricks)
- [Common Pitfalls](#common-pitfalls)

## Installation & Build

### Build and Install with CMake

For a complete step-by-step guide on cloning, building, installing, and using this library in your own project (with commands for all platforms), see the detailed [Getting Started Guide](examples/README.md).

**Note:** If you don't want to install, you can simply add the `include/pythonic` directory to your project's include path and use `target_include_directories` in your CMakeLists.txt.

## Quick Start

### The Easy Way (Recommended)

Just include the main header - it pulls in everything under the `Pythonic` (or `py`) namespace:

```cpp
#include "pythonic/pythonic.hpp"

using namespace Pythonic;
// or
using namespace py;
```

### The Manual Way

Or pick exactly what you need:

```cpp
#include "pythonicVars.hpp"      // var, list, dict, set
#include "pythonicPrint.hpp"     // print(), pprint()
#include "pythonicLoop.hpp"      // range(), enumerate(), zip() + macros
#include "pythonicFunction.hpp"  // map, filter, comprehensions
#include "pythonicFile.hpp"      // File I/O
#include "pythonicMath.hpp"      // Math functions (trig, logarithms, etc.)

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::math;
using namespace pythonic::file;
using namespace pythonic::func;
using namespace pythonic::graph;
using namespace pythonic::fast;
using namespace pythonic::overflow;

using namespace pythonic::error;
```

**Caution:** If you use namespaces and import them globally, be careful with using std names and other namespaces, as they might clash or cause ambiguity and multiple definition errors. We recommend using std:: explicitly if you make the pythonic namespace global.

**About Namespaces:**

- `pythonic::vars` - Core `var` type, containers, type conversion
- `pythonic::print` - Printing and formatting (`print()`, `pprint()`)
- `pythonic::loop` - Iteration helpers (`range()`, `enumerate()`, `zip()`, `reversed()`)
- `pythonic::func` - Functional programming (`map()`, `filter()`, comprehensions)
- `pythonic::file` - File I/O
- `pythonic::math` - Comprehensive math library (`trig`, `logarithms`, `random`, etc.)
- `pythonic::graph` - Graph data structure and algorithms
- `pythonic::error` - Exception hierarchy and error handling
- `pythonic::fast` - Hot-loop optimizations
- `pythonic::overflow` - Checked arithmetic helpers

You can use them all at once with `using namespace Pythonic;` or `using namespace py;`.

## The `var` Type

This is the heart of the library - a dynamic variable that can hold anything.

```cpp
var x = 42;           // int
var y = 3.14;         // double
var name = "Alice";   // string
var flag = true;      // bool

// Changes type whenever you want
x = "now I'm a string";
x = list(1, 2, 3);    // now I'm a list!
```

### Type Checking & Conversion

```cpp
var x = 42;
print(x.type());              // prints: int
print(isinstance(x, "int"));  // prints: True

// Template version
if (isinstance<int>(x)) {
    int n = x.get<int>();
}

// Type conversion helpers
var a = Int("123");      // "123" -> 123
var b = Float("3.14");   // "3.14" -> 3.14
var c = Str(42);         // 42 -> "42"
var d = Bool(0);         // 0 -> false. Any non 0 value for var will give true
```

## Containers

### Lists

```cpp
var nums = list(1, 2, 3, 4, 5);
var mixed = list("hello", 42, 3.14, true);  // yep, mixed types
var empty = list();

// Access by index
print(nums[0]);         // 1
nums[0] = 100;          // modify

// Methods you'd expect
nums.append(6);
nums.extend(list(7, 8, 9));  // Add multiple elements
nums.insert(0, -1);
nums.pop();
nums.remove(100);
nums.clear();
print(nums.len());      // or len(nums)

// List operators
var l1 = list(1, 2, 3);
var l2 = list(2, 3, 4);
var concat = l1 | l2;     // [1, 2, 3, 2, 3, 4]  - Concatenation
var common = l1 & l2;     // [2, 3]              - Intersection
var diff = l1 - l2;       // [1]                 - Difference
```

### Slicing (finally!)

Works just like Python. Negative indices, steps, the whole deal.

```cpp
var lst = list(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

var a = lst.slice(2, 5);      // [2, 3, 4]
var b = lst.slice(0, 10, 2);  // [0, 2, 4, 6, 8]  (every 2nd)
var c = lst.slice(-3);        // [7, 8, 9]  (last 3)
var d = lst(1, 4);            // [1, 2, 3]  (operator() works too)

// Use None for Python-style slicing!
using pythonic::vars::None;
var rev = lst.slice(None, None, var(-1));  // [9, 8, 7, 6, 5, 4, 3, 2, 1, 0] - reverse!
var first_half = lst.slice(None, var(5));       // [0, 1, 2, 3, 4]
var last_half = lst.slice(var(5), None);        // [5, 6, 7, 8, 9]

// Strings too!
var s = "Hello, World!";
print(s.slice(0, 5));         // Hello
print(s.slice(None, None, var(-1)));  // !dlroW ,olleH (reversed)
```

### Dicts

```cpp
var person = dict();
person["name"] = "Bob";
person["age"] = 25;
person["city"] = "NYC";

// Check if key exists
if (person.contains("name")) {
    print(person["name"]);
}

// Iterate
for_each(key, person.keys()) {
    print(key, ":", person[key]);
}

// Get keys, values, items
var keys = person.keys();      // list of keys
var values = person.values();  // list of values
var items = person.items();    // list of [key, value] pairs
```

### Sets

```cpp
var s = set(1, 2, 3, 4, 5);
s.add(6);
s.remove(1);

// Check if element exists (like "in" in Python)
if (s.has(3)) {
    print("3 is in the set");
}

// Set operations using Python-style operators
var a = set(1, 2, 3);
var b = set(3, 4, 5);

var union_set = a | b;          // {1, 2, 3, 4, 5}  - Union
var intersect = a & b;          // {3}              - Intersection
var diff = a - b;               // {1, 2}           - Difference
var sym_diff = a ^ b;           // {1, 2, 4, 5}     - Symmetric difference

// extend() to add multiple elements (like Python's update())
s.update(set(7, 8, 9));  // Add multiple elements from another set
```

### OrderedSet (Sorted Set)

If you need a set that maintains sorted order (like C++'s `std::set`), use `ordered_set()`:

```cpp
// OrderedSet maintains sorted order (O(log n) operations)
var os = ordered_set(5, 3, 1, 4, 2);
print("OrderedSet:", os);  // OrderedSet{1, 2, 3, 4, 5} - sorted!

// All set operations work the same
os.add(0);  // Added in sorted position
print("Contains 3:", os.contains(var(3)));  // True

// Set operations
var os2 = ordered_set(4, 5, 6, 7, 8);
var union_os = os | os2;   // Union (sorted)
var inter_os = os & os2;   // Intersection
```

### OrderedDict (Sorted Dictionary)

Similarly, `ordered_dict()` provides a dictionary sorted by keys:

```cpp
// OrderedDict maintains key-sorted order (O(log n) access)
var od = ordered_dict({{"zebra", 1}, {"apple", 2}, {"mango", 3}});
print("OrderedDict:");
pprint(od);  // Keys in alphabetical order: apple, mango, zebra

// Access and modify like regular dict
od["banana"] = 4;
print("Value:", od["apple"].get<int>());

// Dict operations
var od2 = ordered_dict({{"banana", 40}, {"cherry", 50}});
var merged = od | od2;  // Merge (later values override)
```

**Performance Note:**

- `set()` and `dict()` use hash tables (O(1) average operations) - fastest for most use cases
- `ordered_set()` and `ordered_dict()` use balanced trees (O(log n) operations) - use when you need sorted order

### Truthiness (Python-style Boolean Context)

Just like Python, containers and values have "truthiness":

```cpp
var empty_list = list();
var full_list = list(1, 2, 3);
var zero = 0;
var number = 42;
var empty_str = "";
var text = "hello";

// Works exactly like Python's if statement
if (full_list) {
    print("List has items!");      // This prints
}

if (!empty_list) {
    print("List is empty!");       // This prints
}

if (number) {
    print("Number is non-zero!");  // This prints
}

if (!zero) {
    print("Zero is falsy!");       // This prints
}

if (text) {
    print("String is not empty!"); // This prints
}

// Truthiness rules (just like Python):
// - Empty containers (list, dict, set) are False
// - Zero (0, 0.0) is False
// - Empty string "" is False
// - None is False
// - Everything else is True
```

### None Values

Check for None (undefined/null values):

```cpp
var x;  // Default is None

// Check if None
if (x.type() == "None") {
    print("x is None");
}

// Or use is_none()
if (x.is_none()) {
    print("x is None");
}

// Functions can return None
var result = some_dict.get("nonexistent_key");  // Returns None if key doesn't exist
if (result.is_none()) {
    print("Key not found");
}

// None in truthiness context
if (!x) {
    print("x is None or empty or zero");
}
```

## Graph Data Structure

Graphs are everywhere - social networks, maps, dependencies, you name it. This library gives you a full-featured graph implementation that integrates seamlessly with the `var` type. Create graphs, run algorithms, all with that Pythonic feel.

### Creating Graphs

```cpp
// Create a graph with n nodes (numbered 0 to n-1)
var g = graph(5);  // 5 nodes: 0, 1, 2, 3, 4

print(g.type());       // "graph"
print(g.isGraph());    // true
print(g.str());        // "Graph(nodes=5, edges=0)"

// Graphs are truthy when they have nodes
if (g) {
    print("Graph has nodes!");
}

var empty = graph(0);
if (!empty) {
    print("Empty graph is falsy");
}

// Add nodes dynamically
var g2 = graph(3);           // Start with 3 nodes
size_t new_node = g2.add_node();        // Add a node, returns its index (3)
size_t labeled = g2.add_node("Label");  // Add a node with data

// Get graph size (number of nodes)
print(g2.node_count());  // 5
```

### Adding Edges

Edges connect nodes. You can make them directed or undirected, and give them weights.

```cpp
var g = graph(4);

// add_edge(from, to, weight, reverse_weight, directed)
//
// Parameters:
//   from          - Source node index (size_t)
//   to            - Destination node index (size_t)
//   weight        - Edge weight (default: 0.0)
//   reverse_weight- Weight for reverse edge in undirected graphs (default: 0.0)
//   directed      - If true, creates one-way edge. If false (default), creates both directions.

// Undirected edge (default) - creates edges in both directions
g.add_edge(0, 1, 5.0);        // 0 <-> 1 with weight 5.0

// Directed edge - only one direction
g.add_edge(1, 2, 3.0, 0.0, true);  // 1 -> 2 only

// Check if edges exist
print(g.has_edge(0, 1));  // true
print(g.has_edge(1, 0));  // true (undirected)
print(g.has_edge(1, 2));  // true
print(g.has_edge(2, 1));  // false (directed)

// Get edge weight (returns std::optional<double>)
auto weight = g.get_edge_weight(0, 1);
if (weight.has_value()) {
    print("Weight:", weight.value());  // 5.0
}

// Set edge weight
// set_edge_weight(from, to, new_weight)
g.set_edge_weight(0, 1, 10.0);

// Remove an edge
// remove_edge(from, to, remove_reverse)
//   remove_reverse - If true (default), also removes the reverse edge
g.remove_edge(0, 1);  // Removes both 0->1 and 1->0
```

### Node Data

Each node can store arbitrary data using the `var` type. Super useful for labeling nodes, storing metadata, etc.

```cpp
var g = graph(3);

// set_node_data(node_index, data)
g.set_node_data(0, "Alice");
g.set_node_data(1, "Bob");
g.set_node_data(2, dict({{"name", "Carol"}, {"age", 30}}));

// get_node_data(node_index) - returns var&
var name = g.get_node_data(0);
print(name);  // "Alice"

var carol_info = g.get_node_data(2);
print(carol_info["name"]);  // "Carol"
```

### Graph Properties

Quick info about your graph:

```cpp
var g = graph(5);
g.add_edge(0, 1);
g.add_edge(1, 2);
g.add_edge(2, 3);
g.add_edge(3, 4);

// node_count() - Total number of nodes
print(g.node_count());  // 5

// edge_count() - Total number of edges (counts both directions for undirected)
print(g.edge_count());  // 8 (4 undirected = 8 directed)

// is_connected() - Check if all nodes are reachable from any node
print(g.is_connected());  // true

// has_cycle() - Check if the graph contains any cycles
print(g.has_cycle());  // false (linear chain)

// Add a cycle
g.add_edge(4, 0);
print(g.has_cycle());  // true

// out_degree(node) - Number of outgoing edges from a node
print(g.out_degree(0));  // Number of edges leaving node 0

// in_degree(node) - Number of incoming edges to a node
print(g.in_degree(0));   // Number of edges pointing to node 0

// neighbors(node) - Get neighbor node indices as a list
var nbrs = g.neighbors(0);
print("Neighbors of 0:", nbrs);  // [1, 4] (connected nodes)

// get_edges(node) - Get all edges from a node as a list of dicts
var edges = g.get_edges(0);
for_each(edge, edges) {
    print("To:", edge["to"], "Weight:", edge["weight"]);
}
```

### Graph Traversals

Classic traversal algorithms that return lists of visited nodes:

```cpp
var g = graph(6);
g.add_edge(0, 1);
g.add_edge(0, 2);
g.add_edge(1, 3);
g.add_edge(1, 4);
g.add_edge(2, 5);

// dfs(start, recursive)
// Depth-First Search - goes deep before going wide
//   start     - Starting node (default: 0)
//   recursive - Use recursive implementation if true (default: true)
// Returns: var (list of node indices in visit order)

var dfs_order = g.dfs(0);
print("DFS:", dfs_order);  // [0, 1, 3, 4, 2, 5] (order may vary)

// bfs(start)
// Breadth-First Search - visits all neighbors before going deeper
//   start - Starting node (default: 0)
// Returns: var (list of node indices in visit order)

var bfs_order = g.bfs(0);
print("BFS:", bfs_order);  // [0, 1, 2, 3, 4, 5] (level by level)
```

### Shortest Paths

Three algorithms for finding shortest paths, each with different strengths:

```cpp
var g = graph(5);
g.add_edge(0, 1, 4.0);
g.add_edge(0, 2, 1.0);
g.add_edge(1, 3, 1.0);
g.add_edge(2, 1, 2.0);
g.add_edge(2, 3, 5.0);
g.add_edge(3, 4, 3.0);

// get_shortest_path(source, destination)
// Uses Dijkstra's algorithm (fast, but no negative weights)
// Returns: dict with "path" (list of nodes) and "distance" (total weight)

var result = g.get_shortest_path(0, 4);
print("Path:", result["path"]);         // [0, 2, 1, 3, 4]
print("Distance:", result["distance"]); // 7.0

// bellman_ford(source)
// Handles negative weights, detects negative cycles
// Slower than Dijkstra but more general
// Returns: dict with "distances" (list) and "predecessors" (list)

var bf = g.bellman_ford(0);
print("Distances from 0:", bf["distances"]);      // [0, 3, 1, 4, 7]
print("Predecessors:", bf["predecessors"]);       // For path reconstruction

// floyd_warshall()
// All-pairs shortest paths - finds shortest path between ALL node pairs
// Returns: 2D list (matrix) where result[i][j] = shortest distance from i to j
// Uses infinity (very large number) for unreachable pairs

var fw = g.floyd_warshall();
print("Distance matrix:");
for_each(row, fw) {
    print(row);
}
// Row i gives distances from node i to all other nodes
```

### Graph Algorithms

The heavy hitters - sorting, components, and minimum spanning trees:

```cpp
// ===== Topological Sort =====
// topological_sort()
// Orders nodes so that for every edge u->v, u comes before v
// Only works on Directed Acyclic Graphs (DAGs)
// Returns: var (list of nodes in topological order)

var tasks = graph(4);
tasks.add_edge(0, 1, 1.0, 0.0, true);  // directed
tasks.add_edge(0, 2, 1.0, 0.0, true);
tasks.add_edge(1, 3, 1.0, 0.0, true);
tasks.add_edge(2, 3, 1.0, 0.0, true);

var order = tasks.topological_sort();
print("Build order:", order);  // [0, 1, 2, 3] or [0, 2, 1, 3]


// ===== Connected Components =====
// connected_components()
// Finds groups of nodes that are connected to each other (for undirected graphs)
// Returns: var (list of lists, each inner list is a component)

var network = graph(6);
network.add_edge(0, 1);
network.add_edge(1, 2);
// nodes 3, 4, 5 form another group
network.add_edge(3, 4);
network.add_edge(4, 5);

var components = network.connected_components();
print("Components:", components);  // [[0, 1, 2], [3, 4, 5]]


// ===== Strongly Connected Components =====
// strongly_connected_components()
// For directed graphs: finds groups where every node can reach every other node
// Uses Kosaraju's algorithm
// Returns: var (list of lists)

var directed = graph(5);
directed.add_edge(0, 1, 1.0, 0.0, true);
directed.add_edge(1, 2, 1.0, 0.0, true);
directed.add_edge(2, 0, 1.0, 0.0, true);  // Cycle: 0->1->2->0
directed.add_edge(2, 3, 1.0, 0.0, true);
directed.add_edge(3, 4, 1.0, 0.0, true);
directed.add_edge(4, 3, 1.0, 0.0, true);  // Cycle: 3<->4

var sccs = directed.strongly_connected_components();
print("SCCs:", sccs);  // Contains [0, 1, 2] and [3, 4]


// ===== Minimum Spanning Tree =====
// prim_mst()
// Finds the minimum weight set of edges that connects all nodes
// Uses Prim's algorithm
// Returns: dict with "weight" (total MST weight) and "edges" (list of [from, to, weight])

var city_network = graph(4);
city_network.add_edge(0, 1, 1.0);
city_network.add_edge(0, 2, 4.0);
city_network.add_edge(1, 2, 2.0);
city_network.add_edge(1, 3, 5.0);
city_network.add_edge(2, 3, 3.0);

var mst = city_network.prim_mst();
print("MST total weight:", mst["weight"]);  // 6.0 (1 + 2 + 3)
print("MST edges:", mst["edges"]);          // [[0, 1, 1.0], [1, 2, 2.0], [2, 3, 3.0]]
```

### Graph Serialization

Save your graphs to files and load them back:

```cpp
var g = graph(3);
g.add_edge(0, 1, 1.5);
g.add_edge(1, 2, 2.5);
g.set_node_data(0, "Start");

// save_graph(filename)
// Saves the graph structure to a file
g.save_graph("my_graph.txt");

// load_graph(filename)
// Loads a graph from a file - this is a free function
var loaded = load_graph("my_graph.txt");
print("Loaded graph:", loaded.str());

// to_dot(filename, show_weights)
// Exports the graph in DOT format for visualization with Graphviz
//   filename     - Output file path
//   show_weights - Include edge weights in the output (default: true)

g.to_dot("my_graph.dot");
// Then run: dot -Tpng my_graph.dot -o my_graph.png
```

### Graph Performance Optimization

When you know roughly how many edges you'll be adding, pre-reserving capacity can speed things up significantly:

```cpp
var g = graph(1000);

// reserve_edges_per_node(per_node)
// Pre-allocates space for edges on each node
// Use when all nodes have roughly the same number of edges
g.reserve_edges_per_node(10);  // Each node will have ~10 edges

// Now adding edges is faster (fewer reallocations)
for (size_t i = 0; i < 1000; i++) {
    for (int j = 0; j < 10; j++) {
        g.add_edge(i, (i + j + 1) % 1000, 1.0);
    }
}

// reserve_edges_by_counts(counts_list)
// Pre-allocates different capacities per node
// Use when you know the exact edge counts for each node

var counts = list(5, 10, 3, 8, 2);  // Node 0 gets 5 edges, node 1 gets 10, etc.
var custom = graph(5);
custom.reserve_edges_by_counts(counts);
```

**Tip:** These methods are optional optimizations. If you don't call them, the graph will grow dynamically (just a bit slower for large graphs).

### Complete Graph Example

Here's a real-world example - modeling a small road network:

```cpp
// Cities: 0=NYC, 1=Boston, 2=Philadelphia, 3=DC, 4=Baltimore
var roads = graph(5);

// Set city names
roads.set_node_data(0, "New York");
roads.set_node_data(1, "Boston");
roads.set_node_data(2, "Philadelphia");
roads.set_node_data(3, "Washington DC");
roads.set_node_data(4, "Baltimore");

// Add roads with distances (in miles, approximate)
roads.add_edge(0, 1, 215);   // NYC <-> Boston
roads.add_edge(0, 2, 95);    // NYC <-> Philly
roads.add_edge(2, 4, 100);   // Philly <-> Baltimore
roads.add_edge(4, 3, 40);    // Baltimore <-> DC
roads.add_edge(0, 3, 225);   // NYC <-> DC (direct but longer)

print("Road network:", roads.str());
print("Connected:", roads.is_connected());

// Find shortest route from NYC to DC
var route = roads.get_shortest_path(0, 3);
print("\nShortest NYC to DC:");
print("  Distance:", route["distance"], "miles");
print("  Route:", route["path"]);

// Print city names along the route
var path = route["path"];
print("  Cities:");
for_each(node, path) {
    size_t idx = static_cast<size_t>(node.toInt());
    print("   ->", roads.get_node_data(idx));
}

// Find the minimum road network to connect all cities
var min_roads = roads.prim_mst();
print("\nMinimum road network:");
print("  Total miles:", min_roads["weight"]);
```

## String Methods

All the string goodness you know and love:

```cpp
var s = "  Hello, World!  ";

// Transformations
print(s.upper());              // "  HELLO, WORLD!  "
print(s.lower());              // "  hello, world!  "
print(s.strip());              // "Hello, World!"
print(s.lstrip());             // "Hello, World!  "
print(s.rstrip());             // "  Hello, World!"
print(s.replace("World", "Python"));  // "  Hello, Python!  "
print(s.capitalize());         // "  hello, world!  "
print(s.title());              // "  Hello, World!  "

// Splitting and joining
var words = s.split(",");      // ["  Hello", " World!  "]
var joined = var("-").join(list("a", "b", "c"));  // "a-b-c"

// Searching
print(s.find("World"));        // 9
print(s.count("l"));           // 3
print(s.startswith("  Hello")); // True
print(s.endswith("!  "));      // True

// Checking character types
var digit_str = "12345";
print(digit_str.isdigit());    // True
print(digit_str.isalpha());    // False
print(digit_str.isalnum());    // True

var alpha_str = "Hello";
print(alpha_str.isalpha());    // True

var mixed = "Hello123";
print(mixed.isalnum());        // True

var space_str = "   ";
print(space_str.isspace());    // True

// Padding
var text = "Hi";
print(text.center(10));        // "    Hi    "
print(text.center(10, "*"));   // "****Hi****"
print(text.zfill(5));          // "000Hi"

// Other useful methods
print(s.reverse());            // "  !dlroW ,olleH  "
```

## Comprehensive Math Library

The pythonic math module provides extensive mathematical functions - from basic operations to advanced trigonometry!

```cpp
using namespace pythonic::math;  // Don't forget this!
```

### Basic Math Operations

```cpp
// Rounding
print(round(var(3.7)));        // 4
print(floor(var(3.7)));        // 3
print(ceil(var(3.2)));         // 4
print(trunc(var(-3.7)));       // -3

// Powers and roots
print(pow(var(2), var(10)));   // 1024
print(sqrt(var(16)));          // 4
print(nthroot(var(27), var(3))); // 3 (cube root)

// Exponential and logarithms
print(exp(var(1)));            // 2.718... (e^1)
print(log(var(2.718)));        // ~1 (natural log)
print(log10(var(100)));        // 2
print(log2(var(8)));           // 3

// Absolute value
var x = abs(var(-5));          // 5 (also in pythonic::vars)
```

### Trigonometric Functions

All angles in radians:

```cpp
var pi_val = pi();             // Get π constant

// Basic trig
print(sin(pi_val / 2));        // 1
print(cos(var(0)));            // 1
print(tan(pi_val / 4));        // 1

// Reciprocal trig functions
print(cot(pi_val / 4));        // 1 (cotangent)
print(sec(var(0)));            // 1 (secant)
print(cosec(pi_val / 2));      // 1 (cosecant, also: csc())

// Inverse trig
print(asin(var(1)));           // π/2
print(acos(var(1)));           // 0
print(atan(var(1)));           // π/4
print(atan2(var(1), var(1)));  // π/4 (two-argument)

// And their reciprocals
print(acot(var(1)));           // Inverse cotangent
print(asec(var(1)));           // Inverse secant
print(acosec(var(1)));         // Inverse cosecant (also: acsc())
```

### Hyperbolic Functions

```cpp
print(sinh(var(0)));           // 0
print(cosh(var(0)));           // 1
print(tanh(var(0)));           // 0

// Inverse hyperbolic
print(asinh(var(0)));          // 0
print(acosh(var(1)));          // 0
print(atanh(var(0)));          // 0
```

### Angle Conversion

```cpp
var deg = degrees(pi());       // 180 (radians to degrees)
var rad = radians(var(180));   // π (degrees to radians)
```

### Mathematical Constants

```cpp
var pi_val = pi();             // 3.14159...
var e_val = e();               // 2.71828...
```

### Random Functions

```cpp
// Random integer in range [min, max]
var dice = random_int(var(1), var(6));

// Random float in range [min, max)
var rand_percent = random_float(var(0.0), var(100.0));

// Random element from list or set
var items = list("apple", "banana", "cherry");
var choice = random_choice(items);

var nums_set = set(10, 20, 30, 40);
var num_choice = random_choice_set(nums_set);

// Fill list with N random integers (like NumPy/PyTorch)
var rand_ints = fill_random(100, var(1), var(100));  // 100 random ints in [1, 100]

// Fill list with N random floats (uniform distribution)
var rand_floats = fill_randomf(100, var(0.0), var(1.0));  // 100 random floats in [0, 1)

// Fill list with N random floats (Gaussian/normal distribution)
var gauss_data = fill_randomn(100, var(0.0), var(1.0));  // mean=0, stddev=1

// Fill set with N unique random integers
var rand_set = fill_random_set(10, var(1), var(50));  // 10 unique ints in [1, 50]

// Fill set with N random floats (uniform distribution)
var rand_set_f = fill_randomf_set(10, var(0.0), var(1.0));

// Fill set with N random floats (Gaussian distribution)
var gauss_set = fill_randomn_set(10, var(0.0), var(1.0));  // mean=0, stddev=1
```

### Product (Multiply All Elements)

```cpp
var factors = list(2, 3, 4);
print(product(factors));        // 24

// With starting value
print(product(factors, var(10))); // 240

// Works with sets too
var s = set(2, 3, 4);
print(product(s));              // 24
```

### Advanced Math Functions

```cpp
// Greatest common divisor
print(gcd(var(48), var(18)));  // 6

// Least common multiple
print(lcm(var(12), var(18)));  // 36

// Factorial
print(factorial(var(5)));      // 120

// Hypotenuse (Pythagorean theorem)
print(hypot(var(3), var(4)));  // 5

// Other useful functions
print(fmod(var(5.5), var(2.0))); // 1.5 (floating-point remainder)
print(copysign(var(5), var(-1))); // -5 (copy sign)
print(fabs(var(-3.14)));       // 3.14 (floating-point absolute)
```

### Aggregation Functions (from pythonic::vars)

```cpp
using namespace pythonic::vars;  // These are in vars namespace

var nums = list(3, 1, 4, 1, 5);
print(min(nums));              // 1
print(max(nums));              // 5
print(sum(nums));              // 14

// **Warning**: min(), max(), and sum() on empty lists will throw runtime_error
// Always check if list is not empty:
var data = list();
if (data) {  // Truthiness check
    print(max(data));
}
```

## List Comprehensions

Yes, we have those too! Three flavors:

### List Comprehensions

```cpp
using namespace pythonic::func;

// Basic: [x*2 for x in range(5)]
auto doubled = list_comp(
    range(5),
    lambda_(x, x * 2)
);
// Result: [0, 2, 4, 6, 8]

// With filter: [x*2 for x in range(10) if x % 2 == 0]
auto even_doubled = list_comp(
    range(10),
    lambda_(x, x * 2),
    lambda_(x, x % 2 == 0)
);
// Result: [0, 4, 8, 12, 16]
```

### Set Comprehensions

```cpp
// {x*x for x in range(5)}
auto squares = set_comp(
    range(5),
    lambda_(x, x * x)
);
// Result: {0, 1, 4, 9, 16}
```

### Dict Comprehensions

```cpp
// {x: x*x for x in range(5)}
auto square_dict = dict_comp(
    range(5),
    lambda_(x, x),        // key function
    lambda_(x, x * x)     // value function
);
// Result: {0:0, 1:1, 2:4, 3:9, 4:16}
```

## Lambda Helpers

Writing lambdas in C++ can be verbose. We've got macros to help:

```cpp
// One parameter
auto double_it = lambda_(x, x * 2);

// Two parameters
auto add = lambda2_(x, y, x + y);

// Three parameters
auto sum3 = lambda3_(x, y, z, x + y + z);

// Capturing lambda (captures by value)
int factor = 10;
auto scale = clambda_(x, x * factor);
```

## Iteration

### Range

```cpp
using namespace pythonic::loop;

// range(n) - 0 to n-1
for_each(i, range(5)) {
    print(i);  // 0, 1, 2, 3, 4
}

// range(start, stop)
for_each(i, range(2, 5)) {
    print(i);  // 2, 3, 4
}

// range(start, stop, step)
for_each(i, range(0, 10, 2)) {
    print(i);  // 0, 2, 4, 6, 8
}

// Negative step (reverse iteration) - very useful!
for_each(i, range(10, 0, -1)) {
    print(i);  // 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
}

// **Note**: step cannot be 0 - will throw invalid_argument
```

### Enumerate

```cpp
var fruits = list("apple", "banana", "cherry");

for_enumerate(i, fruit, fruits) {
    print(i, ":", fruit);
    // 0 : apple
    // 1 : banana
    // 2 : cherry
}
```

### Zip

```cpp
var names = list("Alice", "Bob", "Charlie");
var ages = list(25, 30, 35);

for (auto pair : zip(names, ages)) {
    print(pair[0], "is", pair[1], "years old");
}
```

### Reversed

Iterate in reverse order - super common in Python!

```cpp
var nums = list(1, 2, 3, 4, 5);
var rev = reversed_var(nums);  // [5, 4, 3, 2, 1]

for_each(x, reversed_var(nums)) {
    print(x);  // 5, 4, 3, 2, 1
}

// Works with range() too
for_each(i, reversed_var(range(10))) {
    print(i);  // 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
}
```

### Loop Macros

Tired of typing `for (auto x : ...)`? Use these:

```cpp
var nums = list(1, 2, 3, 4, 5);

// for_in - simple iteration
for_each(x, nums) {
    print(x);
}

// for_index - index-based loop
for_index(i, nums) {
    print(i, nums[i]);
}

// for_enumerate - both index and value
for_enumerate(i, x, nums) {
    print(i, ":", x);
}

// for_range - like Python's range()
for_range(i, 0, 10, 2) {
    print(i);  // 0, 2, 4, 6, 8
}

// while_true - infinite loop
int count = 0;
while_true {
    print(count++);
    if (count > 5) break;
}
```

## Functional Programming

### Map, Filter, Reduce

```cpp
using namespace pythonic::func;

var nums = list(1, 2, 3, 4, 5);

// Map
var doubled = map(nums, lambda_(x, x * 2));
// [2, 4, 6, 8, 10]

// Map with index (hybrid of map and enumerate)
// Perfect for position-dependent transformations!
auto with_idx = map_indexed(nums, lambda2_(idx, x,
    dict().set("index", idx).set("value", x)
));
// [{index:0, value:1}, {index:1, value:2}, ...]

// Practical example: Apply weights based on position
var values = list(10, 20, 30, 40);
var weights = list(1.0, 1.5, 2.0, 2.5);
auto weighted = map_indexed(values, lambda2_(i, x,
    x * weights[i]
));
// [10, 30, 60, 100]

// Filter
var evens = filter(nums, lambda_(x, x % 2 == 0));
// [2, 4]

// Reduce (sum example)
var total = reduce(nums, 0, lambda2_(acc, x, acc + x));
// 15
```

### Sorting

```cpp
var nums = list(3, 1, 4, 1, 5, 9);

// Basic sort
var sorted_nums = sorted(nums);
// [1, 1, 3, 4, 5, 9]

// Reverse sort
var desc = sorted(nums, true);
// [9, 5, 4, 3, 1, 1]

// Sort with custom key
var words = list("apple", "pie", "zoo", "a");
auto by_length = sorted(words, lambda_(x, x.len()));
// ["a", "pie", "zoo", "apple"]
```

### Advanced Functional Utilities

```cpp
using namespace pythonic::func;

var nums = list(1, 2, 3, 4, 5, 6, 7, 8, 9);

// Take first N elements
var first_three = take(nums, 3);  // [1, 2, 3]

// Drop first N elements
var skip_three = drop(nums, 3);   // [4, 5, 6, 7, 8, 9]

// Take while condition is true
auto less_than_5 = take_while(nums, lambda_(x, x < 5));
// [1, 2, 3, 4]

// Drop while condition is true
auto from_5 = drop_while(nums, lambda_(x, x < 5));
// [5, 6, 7, 8, 9]

// Find first matching element
auto found = find_if(nums, lambda_(x, x > 5));
// 6

// Get index of element
var idx = index(nums, 5);  // 4

// Count matching elements
auto count_evens = count_if(nums, lambda_(x, x % 2 == 0));
// 4

// Flatten nested lists
var nested = list(list(1, 2), list(3, 4), list(5));
auto flat = flatten(nested);  // [1, 2, 3, 4, 5]

// Get unique elements
var dups = list(1, 2, 2, 3, 3, 3);
auto uniq = unique(dups);  // [1, 2, 3]

// Group by key
var items = list(1, 2, 3, 4, 5, 6);
auto grouped = group_by(items, lambda_(x, x % 2));
// {0: [2, 4, 6], 1: [1, 3, 5]}

// Cartesian product
auto prod = product(list(1, 2), list("a", "b"));
// [[1, "a"], [1, "b"], [2, "a"], [2, "b"]]

// Partial application
auto add = lambda2_(x, y, x + y);
auto add5 = partial(add, 5);
var result = add5(3);  // 8

// Function composition
auto double_it = lambda_(x, x * 2);
auto add_one = lambda_(x, x + 1);
auto double_then_add = compose(add_one, double_it);
var res = double_then_add(5);  // 11 (5*2 + 1)
```

### All/Any Predicates

```cpp
var all_true = list(true, true, true);
var some_true = list(false, true, false);
var none_true = list(false, false, false);

print(all_var(all_true));    // True
print(all_var(some_true));   // False
print(any_var(some_true));   // True
print(any_var(none_true));   // False
```

## Global Variable Table (let macro)

Need a Python-like global variable table? We've got you covered:

```cpp
using namespace pythonic::vars;

// Store variables by name
let(x) = 42;
let(name) = "Alice";
let(data) = list(1, 2, 3);

// Access them
print(let(x));        // 42
print(let(name));     // Alice

// Modify them
let(x) = let(x) + 10;
print(let(x));        // 52

// Check if exists
if (DynamicVar::has("x")) {
    print("x exists!");
}

// List all variables
auto all_vars = DynamicVar::list_all();
for_each(name, all_vars) {
    print(name, "=", DynamicVar::get_ref(name.template get<std::string>()));
}
```

This is handy for: - Scripting-like code

- Dynamic variable lookups
- Runtime configuration
- DSLs and interpreters

## Printing & Formatting

### Basic Print

Python-style print with space-separated arguments:

```cpp
using namespace pythonic::print;

// Works with var types
print("Hello", "World", 42);
// Output: Hello World 42

// Works with mixed native C++ types and var
var x = 100;
print("The result is:", x, "and the flag is:", true);
// Output: The result is: 100 and the flag is: true

// Automatic spacing between arguments
print(1, 2, 3, 4, 5);
// Output: 1 2 3 4 5
```

**String Formatting Behavior:**
Just like Python, strings behave differently based on context:

- Top-level strings: No quotes (e.g., `print("hello")` → `hello`)
- Strings in containers: Quoted (e.g., `print(list("hello"))` → `[hello]`)

This makes the output feel natural and Pythonic!

### Pretty Print

For complex nested structures with automatic indentation:

```cpp
var data = dict()
    .set("name", "Alice")
    .set("age", 30)
    .set("hobbies", list("reading", "coding", "gaming"))
    .set("scores", dict().set("math", 95).set("english", 88));

pprint(data);
// Output (nicely formatted with 2-space indentation):
// {
//   age: 30
//   hobbies: [reading, coding, gaming]
//   name: Alice
//   scores: {
//     english: 88
//     math: 95
//   }
// }

// Custom indentation (4 spaces instead of 2)
pprint(data, 4);
// {
//     age: 30
//     hobbies: [reading, coding, gaming]
//     ...
// }
```

**Smart Formatting:**

- **Simple/short lists**: Kept on one line for readability
- **Nested/complex structures**: Automatically indented with proper nesting
- **Recursive handling**: Deep nesting is automatically formatted correctly

Example of automatic nesting:

```cpp
var complex = dict()
    .set("users", list(
        dict().set("name", "Alice").set("scores", list(85, 90, 95)),
        dict().set("name", "Bob").set("scores", list(78, 82, 88))
    ))
    .set("meta", dict().set("version", 1).set("updated", "2024-01-01"));

pprint(complex);
// Beautifully formatted with proper indentation at each level!
// {
//   meta: {
//     updated: 2024-01-01
//     version: 1
//   }
//   users: [
//     {
//       name: Alice
//       scores: [85, 90, 95]
//     }
//     {
//       name: Bob
//       scores: [78, 82, 88]
//     }
//   ]
// }
```

## User Input

```cpp
using namespace pythonic::vars;

// Get string input
var name = input("Enter your name: ");
print("Hello,", name);

// Get and convert to int
var age_str = input("Enter your age: ");
var age = Int(age_str);

// Get and convert to float
var price_str = input("Enter price: ");
var price = Float(price_str);
```

## File I/O

### One-Line File Utilities

In standard C++, file I/O requires opening, writing, and closing. We make it simple:

```cpp
using namespace pythonic::file;

// Read entire file in one line
auto content = read_file("data.txt");
if (content) {  // Check if file was read (returns None if missing)
    print(content);
}

// Read lines into a list
auto lines = read_lines("data.txt");
if (lines) {  // Returns None if file doesn't exist
    for_each(line, lines) {
        print(line);
    }
}
```

**Important**: File read operations return `None` if the file doesn't exist or can't be opened. Always check:

```cpp
var data = read_file("config.txt");
if (!data) {
    print("File not found or couldn't be read!");
}
```

### Writing Files

```cpp
// Write string to file (one line!)
write_file("output.txt", "Hello, World!");

// Append to file
append_file("log.txt", "New log entry\n");

// Write list of lines
var lines = list("Line 1", "Line 2", "Line 3");
write_lines("output.txt", lines);
```

### Binary Mode Support

For raw binary data (e.g., neural network weights, images):

```cpp
using namespace pythonic::file;

// Binary write
File bin_file("weights.bin", FileMode::WRITE_BINARY);
bin_file.write("raw binary data");
bin_file.close();

// Binary read
File reader("weights.bin", FileMode::READ_BINARY);
auto binary_data = reader.read();
reader.close();

// Binary append
File appender("data.bin", FileMode::APPEND_BINARY);
appender.write("more binary data");
appender.close();
```

### Python-Style Context Manager (with_open)

The most Pythonic feature - automatic resource management:

```cpp
// Just like Python's "with open(...) as f:"
with_open("data.txt", "r", [](File &f) {
    auto content = f.read();
    print(content);
});  // File automatically closed!

// Write mode
with_open("output.txt", "w", [](File &f) {
    f.write("Hello, World!");
    f.write("Second line\n");
});  // File automatically closed!

// Append mode
with_open("log.txt", "a", [](File &f) {
    f.write("Log entry at ");
    f.write("2024-01-01\n");
});

// No need to remember to close() - it's automatic!
```

**Modes**: "r" (read), "w" (write), "a" (append), "rb" (read binary), "wb" (write binary), "ab" (append binary)

### File Existence Check

```cpp
// Check before reading (pragmatic approach)
if (file_exists("config.txt")) {
    auto config = read_file("config.txt");
    print(config);
} else {
    print("Config file not found, using defaults");
}

// Delete file
if (file_exists("temp.txt")) {
    file_remove("temp.txt");
}
```

## Operator Overloading

The `var` type supports all the operators you'd expect:

```cpp
var a = 10, b = 5;

// Arithmetic (implicit conversion works!)
print(a + b);    // 15
print(a + 5);    // 15 - no need for var(5)!
print(a - b);    // 5
print(a * b);    // 50
print(a / b);    // 2
print(a % b);    // 0

// Comparison (implicit conversion works!)
print(a == b);   // False
print(a == 10);  // True - no need for var(10)!
print(a != b);   // True
print(a > b);    // True
print(a > 5);    // True - works directly with literals!
print(a < b);    // False
print(a >= b);   // True
print(a <= b);   // False

// Logical
var x = true, y = false;
print(x && y);   // False
print(x || y);   // True
print(!x);       // False

// String concatenation
var s1 = "Hello", s2 = "World";
print(s1 + " " + s2);  // "Hello World"

// List concatenation (+ operator)
var l1 = list(1, 2), l2 = list(3, 4);
print(l1 + l2);  // [1, 2, 3, 4]

// Container operators (|, &, -, ^)
var set1 = set(1, 2, 3);
var set2 = set(2, 3, 4);
print(set1 | set2);  // {1, 2, 3, 4}  - Union
print(set1 & set2);  // {2, 3}        - Intersection
print(set1 - set2);  // {1}           - Difference
print(set1 ^ set2);  // {1, 4}        - Symmetric difference

// Works for lists and dicts too!
var list1 = list(1, 2, 3);
var list2 = list(2, 3, 4);
print(list1 | list2);  // [1, 2, 3, 2, 3, 4]  - Concatenation
print(list1 & list2);  // [2, 3]              - Intersection
print(list1 - list2);  // [1]                 - Difference

var d1 = dict(); d1["a"] = 1; d1["b"] = 2;
var d2 = dict(); d2["b"] = 3; d2["c"] = 4;
print(d1 | d2);  // {"a": 1, "b": 3, "c": 4}  - Merge (right overwrites)
print(d1 & d2);  // {"b": 2}                  - Common keys
print(d1 - d2);  // {"a": 1}                  - Keys only in d1
```

## C++20 Features

This library leverages modern C++20 features to provide type-safe, flexible, and performant abstractions.

### Concepts

The library defines a rich set of concepts in `pythonic::loop::traits` for generic programming:

```cpp
#include "pythonic/pythonic.hpp"

using namespace pythonic::loop::traits;

// Core container concepts
static_assert(Iterable<std::vector<int>>);    // Has begin()/end()
static_assert(Container<std::vector<int>>);   // Iterable + size()
static_assert(Sized<std::vector<int>>);       // Has size() returning integral
static_assert(Reversible<std::vector<int>>);  // Has rbegin()/rend()
static_assert(RandomAccess<std::vector<int>>);// Has operator[]

// Numeric concepts
static_assert(Numeric<int>);
static_assert(Numeric<double>);
static_assert(SignedNumeric<int>);
static_assert(UnsignedNumeric<unsigned int>);
static_assert(!Numeric<std::string>);

// Callable concepts
auto square = [](int x) { return x * x; };
static_assert(Callable<decltype(square), int>);
static_assert(UnaryCallable<decltype(square), int>);

auto add = [](int a, int b) { return a + b; };
static_assert(BinaryCallable<decltype(add), int, int>);

auto is_even = [](int x) { return x % 2 == 0; };
static_assert(Predicate<decltype(is_even), int>);
```

#### Using Concepts in Your Code

```cpp
// Write generic functions that work with any iterable
template <Iterable T>
void process_items(T& container) {
    for (auto& item : container) {
        // Process each item
    }
}

// Require containers with size
template <Container T>
auto get_middle(T& container) {
    return container[container.size() / 2];
}

// Use FullContainer for maximum capabilities
template <FullContainer T>
void full_access(T& container) {
    // Forward iteration
    for (auto& item : container) { }
    // Reverse iteration
    for (auto it = container.rbegin(); it != container.rend(); ++it) { }
    // Random access
    auto first = container[0];
}
```

### Ranges Integration

The library integrates with C++20 `std::ranges` through the `pythonic::loop::views` namespace:

```cpp
#include "pythonic/pythonic.hpp"

using namespace pythonic::loop;
using namespace pythonic::loop::views;

// Take first N elements
auto first_five = take_n(range(1, 100), 5);  // [1, 2, 3, 4, 5]

// Drop first N elements
auto after_five = drop_n(range(1, 10), 5);   // [6, 7, 8, 9]

// Filter with predicate
auto evens = filter_view(range(1, 20), [](int x) { return x % 2 == 0; });

// Transform elements
auto squares = transform_view(range(1, 10), [](int x) { return x * x; });

// Reverse iteration
std::vector<int> v = {1, 2, 3, 4, 5};
for (int x : reverse_view(v)) {
    print(x);  // 5, 4, 3, 2, 1
}

// Generate sequences
for (int x : iota_view(1, 10)) {
    print(x);  // 1, 2, 3, ..., 9
}
```

### Fast Path Cache (Hot-Loop Optimization)

For performance-critical code with repeated operations on the same types, use the cached operation system:

```cpp
#include "pythonic/pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::fastpath;

// Create cached operations (caches function pointers by type pair)
CachedAdd add_op;
CachedMul mul_op;

var a = 10, b = 20;

// First call: looks up and caches the operation
var sum = add_op(a, b);  // 30

// Subsequent calls with same types: O(1) dispatch
for (int i = 0; i < 1000000; ++i) {
    sum = add_op(sum, b);  // No type checking overhead!
}

// Works with mixed types too (double + int, etc.)
var x = 3.14, y = 2;
var result = mul_op(x, y);  // 6.28

// Convenient helper functions for common patterns
std::vector<var> numbers = {1, 2, 3, 4, 5};
var total = fast_sum(numbers);      // 15
var product = fast_product(numbers); // 120

// Dot product
std::vector<var> a_vec = {1, 2, 3};
std::vector<var> b_vec = {4, 5, 6};
var dot = fast_dot(a_vec, b_vec);   // 1*4 + 2*5 + 3*6 = 32

// Cached accumulator for reduction operations
CachedAccumulator<CachedAdd> summer(var(0));
for (auto& n : numbers) {
    summer.accumulate(n);
}
var result = summer.get();  // 15
```

#### When to Use Fast Path Cache

- **Hot loops** with repeated arithmetic on `var` types
- **Numerical algorithms** where type stability can be assumed
- **Large data processing** where function dispatch overhead matters

For simple one-off operations, regular `var` arithmetic is perfectly fine.

## Error Handling

The library provides a Python-style exception hierarchy for graceful error handling.

### Exception Hierarchy

```
PythonicError (base class)
├── PythonicTypeError      - Type mismatch errors
├── PythonicValueError     - Invalid values
├── PythonicIndexError     - Index out of bounds
├── PythonicKeyError       - Key not found in dict
├── PythonicAttributeError - Invalid attribute access
├── PythonicOverflowError  - Arithmetic overflow
├── PythonicZeroDivisionError - Division by zero
├── PythonicFileError      - File I/O errors
├── PythonicGraphError     - Graph operation errors
├── PythonicIterationError - Iteration errors
├── PythonicRuntimeError   - General runtime errors
├── PythonicNotImplementedError - Unimplemented features
└── PythonicStopIteration  - Iterator exhaustion
```

### Using Exceptions

```cpp
#include "pythonic/pythonic.hpp"

using namespace pythonic;
using namespace pythonic::vars;

try {
    var mylist = list(1, 2, 3);
    var item = mylist[10];  // Throws PythonicIndexError
}
catch (const PythonicIndexError& e) {
    print("Index error:", e.what());
    // "list index 10 out of range for size 3"
}

try {
    var d = dict();
    d["key"] = "value";
    var missing = d.at("nonexistent");  // Throws PythonicKeyError
}
catch (const PythonicKeyError& e) {
    print("Key not found:", e.what());
}

try {
    var s = "hello";
    var num = s.toInt();  // Throws PythonicTypeError
}
catch (const PythonicTypeError& e) {
    print("Type error:", e.what());
}

// Catch any pythonic error
try {
    // ... operations that might fail
}
catch (const PythonicError& e) {
    print("Something went wrong:", e.what());
}
```

## Checked Arithmetic

The library provides comprehensive overflow-checked arithmetic operations with configurable policies for handling overflow scenarios.

### Overflow Policies

The `Overflow` enum provides three modes for handling arithmetic overflow:

```cpp
#include "pythonic/pythonic.hpp"

using namespace pythonic::overflow;
using namespace pythonic::math;
using namespace pythonic::vars;

// Overflow::Throw (default) - Throws PythonicOverflowError on overflow
// Overflow::Promote         - Auto-promotes to larger type on overflow
// Overflow::Wrap            - Allows wrapping (C++ default behavior)
```

### Basic Usage with var Types

```cpp
var a = 10;
var b = 20;

// Default behavior (Throw) - throws on overflow
var result = add(a, b);                      // 30
var result2 = add(a, b, Overflow::Throw);    // Same as above

// Promote mode - auto-promotes to larger type
var max_int = std::numeric_limits<int>::max();
var one = 1;
var promoted = add(max_int, one, Overflow::Promote);  // Promotes to long long or double

// Wrap mode - allows overflow wrapping
var wrapped = add(max_int, one, Overflow::Wrap);  // Wraps around (implementation-defined)
```

### Available Operations

All operations support the three overflow policies:

```cpp
// Addition
var sum = add(a, b, Overflow::Throw);      // Throws on overflow
var sum = add(a, b, Overflow::Promote);    // Promotes on overflow
var sum = add(a, b, Overflow::Wrap);       // Wraps on overflow

// Subtraction
var diff = sub(a, b, Overflow::Throw);
var diff = sub(a, b, Overflow::Promote);
var diff = sub(a, b, Overflow::Wrap);

// Multiplication
var prod = mul(a, b, Overflow::Throw);
var prod = mul(a, b, Overflow::Promote);
var prod = mul(a, b, Overflow::Wrap);

// Division (always throws on zero, overflow check for INT_MIN/-1)
var quot = div(a, b, Overflow::Throw);
var quot = div(a, b, Overflow::Promote);
var quot = div(a, b, Overflow::Wrap);

// Modulo (always throws on zero)
var rem = mod(a, b, Overflow::Throw);
var rem = mod(a, b, Overflow::Promote);
var rem = mod(a, b, Overflow::Wrap);
```

### Mixed Type Support

Operations support `var + var`, `var + primitive`, and `primitive + var`:

```cpp
var x = 100;

// var + var
var r1 = add(x, var(50));

// var + primitive
var r2 = add(x, 50, Overflow::Throw);

// primitive + var
var r3 = add(50, x, Overflow::Throw);

// All numeric types supported
var ll = var(1000000000LL);
var d = var(3.14);
var result = add(ll, d, Overflow::Promote);  // Returns double
```

### Direct Primitive Operations

For performance-critical code, use direct primitive operations:

```cpp
using namespace pythonic::overflow;

int a = std::numeric_limits<int>::max();
int b = 1;

// Throw variants
try {
    int result = add_throw(a, b);  // Throws PythonicOverflowError
}
catch (const PythonicOverflowError& e) {
    print("Overflow detected!");
}

// Promote variants - auto-promotes return type
auto promoted = add_promote(a, b);  // Returns long long (or double if needed)

// Wrap variants
int wrapped = add_wrap(a, b);  // Wraps around

// Policy-based API
auto r1 = add(a, b, Overflow::Throw);
auto r2 = add(a, b, Overflow::Promote);
auto r3 = add(a, b, Overflow::Wrap);
```

### Overflow Detection Helpers

```cpp
using namespace pythonic::overflow;

int a = std::numeric_limits<int>::max();
int b = 1;

// Check if operation would overflow (without performing it)
if (would_add_overflow(a, b)) {
    print("Addition would overflow!");
}

if (would_sub_overflow(a, b)) {
    print("Subtraction would overflow!");
}

if (would_mul_overflow(a, b)) {
    print("Multiplication would overflow!");
}
```

### Type Promotion Chain

When using `Overflow::Promote`, the library uses a **smart promotion** strategy:

1. **Compute the result in `long double`** (the widest type available)
2. **Find the smallest container that fits the result**

#### Integer Inputs (no floating point operands)

If both operands are integers, try these containers in order:

```
int → long long → float → double → long double → (throw if inf)
```

#### Floating Point or Division

If any operand is floating point, or the operation is division, try:

```
float → double → long double → (throw if inf)
```

> **Note:** This strategy is O(1), simple, and always finds the most compact representation.
> If even `long double` overflows (becomes infinity), a `PythonicOverflowError` is thrown.

```cpp
// Example: int addition that fits in int
var a = 10, b = 20;
auto result = add(a, b, Overflow::Promote);  // Returns int (30)

// Example: int overflow promotes to long long
var max_int = std::numeric_limits<int>::max();
auto result2 = add(max_int, var(1), Overflow::Promote);  // Returns long long

// Example: long long overflow promotes to double
var max_ll = std::numeric_limits<long long>::max();
auto result3 = add(max_ll, var(1LL), Overflow::Promote);  // Returns double

// Example: int + float → smallest floating container
var x = 10;
var y = 1.5f;
auto result4 = add(x, y, Overflow::Promote);  // Returns float (11.5)
```

### Legacy API (Backward Compatibility)

The old `checked_*` functions still work and use `Overflow::Throw`:

```cpp
// These are equivalent to using Overflow::Throw
var a = 10, b = 20;
var sum = checked_add(a, b);    // Same as add(a, b, Overflow::Throw)
var diff = checked_sub(a, b);   // Same as sub(a, b, Overflow::Throw)
var prod = checked_mul(a, b);   // Same as mul(a, b, Overflow::Throw)
var quot = checked_div(a, b);   // Same as div(a, b, Overflow::Throw)
var rem = checked_mod(a, b);    // Same as mod(a, b, Overflow::Throw)
```

### Direct Operators

Direct operators (`+`, `-`, `*`, `/`, `%`) on `var` types use the default throw behavior:

```cpp
var a = 10, b = 20;

// Direct operators - use internal overflow checking with Throw policy
var sum = a + b;   // Uses operator+, throws on overflow
var diff = a - b;  // Uses operator-
var prod = a * b;  // Uses operator*
var quot = a / b;  // Uses operator/, throws on zero
var rem = a % b;   // Uses operator%, throws on zero

// These throw the appropriate errors
try {
    var zero = 0;
    var bad = a / zero;  // Throws PythonicZeroDivisionError
}
catch (const PythonicZeroDivisionError& e) {
    print("Division by zero!");
}
```

### Error Types

- **`PythonicOverflowError`** - Thrown when arithmetic overflows (with Throw policy)
- **`PythonicZeroDivisionError`** - Thrown when dividing or taking modulo by zero

### Detailed Overflow Policy Behavior

#### Policy Overview

| Policy      | Behavior                                                                                     |
| ----------- | -------------------------------------------------------------------------------------------- |
| **Throw**   | Check for overflow, throw `PythonicOverflowError` if overflow occurs                         |
| **Wrap**    | Fast path, no checking, allow wrapping (undefined behavior for signed, defined for unsigned) |
| **Promote** | Compute in `long double`, find smallest container that fits the result                       |

#### Smart Promotion Strategy

The `Overflow::Promote` policy uses a simple O(1) algorithm:

1. Convert all operands to `long double`
2. Perform the operation in `long double`
3. Find the smallest type that can hold the result:
   - **Integer inputs**: try `int` → `long long` → `float` → `double` → `long double`
   - **Floating inputs or division**: try `float` → `double` → `long double`
4. If the result is infinity/NaN, throw `PythonicOverflowError`

#### Operation-Specific Behavior

| Operation | Throw                         | Wrap                             | Promote                                    |
| --------- | ----------------------------- | -------------------------------- | ------------------------------------------ |
| **ADD**   | Check and throw on overflow   | Fast path, no check              | Compute in `long double` → fit to smallest |
| **SUB**   | Check and throw on overflow   | Fast path, no check              | Compute in `long double` → fit to smallest |
| **MUL**   | Check and throw on overflow   | Fast path, no check              | Compute in `long double` → fit to smallest |
| **DIV**   | Throws on zero and INT_MIN/-1 | Throws on zero, INT_MIN/-1 wraps | Always floating result → fit to smallest   |
| **MOD**   | Throws on zero divisor        | Throws on zero divisor           | Use `fmod` → fit to smallest               |
| **POW**   | Check and throw on overflow   | Fast path, no check              | Compute in `long double` → fit to smallest |

#### Example: Smart Promotion in Action

```cpp
var a = 10;
var b = 20;

// No overflow → fits in int
var result = add(a, b, Overflow::Promote);  // Returns int (30)

// Overflow → promoted to smallest container that fits
var max_int = std::numeric_limits<int>::max();
var one = 1;
var result2 = add(max_int, one, Overflow::Promote);  // Returns long long

// Large overflow → promoted to double
var max_ll = std::numeric_limits<long long>::max();
var result3 = add(max_ll, var(1LL), Overflow::Promote);  // Returns double

// Division always returns floating point
var five = 5;
var two = 2;
var result4 = div(five, two, Overflow::Promote);  // Returns float (2.5)

// Mixed int + float → smallest floating container
var x = 1000;
var y = 1.5f;
var result5 = add(x, y, Overflow::Promote);  // Returns float (1001.5)
```

## What's Under the Hood?

Just so you know what you're working with:

- **`var`** - A `std::variant` wrapper that can hold `int`, `double`, `std::string`, `bool`, `List`, `Dict`, `Set`, `None`
- **Containers** - Internally use `std::vector`, `std::map`, `std::set`
- **Performance** - Comparable to standard containers with minimal overhead
- **C++20 Required** - Uses `std::variant`, `std::visit`, `std::span`, `concepts`
- **No exceptions** - Returns sensible defaults on errors (empty var, None, etc.)
- **Move semantics** - Optimized for modern C++ performance

## Examples

### Quick Script Example

```cpp
#include "pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::loop;
using namespace pythonic::func;

int main() {
    // Get user input
    var name = input("What's your name? ");
    var age_str = input("How old are you? ");
    var age = Int(age_str);

    // Calculate
    var years_to_100 = 100 - age;

    // Print
    print("Hey", name + "!");
    print("You'll be 100 in", years_to_100, "years.");

    // Show some numbers
    var nums = range(1, age + var(1));
    var squares = list_comp(lambda_(x, x * x), nums);

    print("Squares from 1 to", age, ":");
    pprint(squares);

    return 0;
}
```

### Data Processing Example

```cpp
#include "pythonic.hpp"

using namespace pythonic::vars;
using namespace pythonic::print;
using namespace pythonic::func;

int main() {
    // Sample data - creating dicts with [] operator
    var alice = dict();
    alice["name"] = "Alice";
    alice["score"] = 85;

    var bob = dict();
    bob["name"] = "Bob";
    bob["score"] = 92;

    var charlie = dict();
    charlie["name"] = "Charlie";
    charlie["score"] = 78;

    var diana = dict();
    diana["name"] = "Diana";
    diana["score"] = 95;

    var students = list(alice, bob, charlie, diana);

    // Filter high scorers (function first, iterable second)
    auto high_scorers = filter(
        lambda_(s, s["score"] > var(80)),
        students
    );

    // Get names (function first, iterable second)
    auto names = map(
        lambda_(s, s["name"]),
        high_scorers
    );

    print("Students with score > 80:");
    pprint(names);

    // Calculate average
    auto scores = map(lambda_(s, s["score"]), students);
    auto total = reduce(lambda2_(a, b, a + b), scores, var(0));
    var avg = total / var(len(scores));

    print("Average score:", avg);

    return 0;
}
```

## Tips & Tricks

1. **Use `using namespace`** - Makes code cleaner, less typing
2. **`pprint()` for debugging** - Much easier to read than `print()` for complex structures
3. **Lambda macros** - `lambda_()`, `lambda2_()` save tons of typing
4. **Loop macros** - `for_in`, `for_enumerate` are your friends
5. **Type conversions** - Use `Int()`, `Float()`, `Str()`, `Bool()` for explicit conversions
6. **Slicing** - Works on both lists and strings, supports negative indices
7. **Comprehensions** - More concise than manual loops for transforming data
8. **The central header** - `#include "pythonic.hpp"` gives you everything

## API Notes & Gotchas

### Working with var and Literals

Implicit conversion now works for most arithmetic and comparison operators:

```cpp
// ✓ These all work now!
var x = var(10);
var y = x + 2;      // Works! Implicit conversion
var z = x * 3;      // Works!
var cmp = x > 5;    // Works!
var eq = x == 10;   // Works!

// String concatenation with literals also works:
var s = "Hello" + var(" ") + "World";  // Works!

// Note: When ambiguity occurs (rare), wrap in var():
var result = var(2) + x;  // If left operand is literal
```

### List Operations

```cpp
// ✓ append() works
nums.append(6);

// ✓ extend() now works!
nums.extend(list(7, 8, 9));  // Add multiple elements
nums.extend("abc");          // Add characters from string
```

### Slicing with None

Use `None` for Python-style full-range slicing:

```cpp
using pythonic::vars::None;
var lst = list(1, 2, 3, 4, 5);

// ✓ None now works!
var reversed = lst.slice(None, None, var(-1));  // Reverse!
var first_three = lst.slice(None, 3);           // [1, 2, 3]
var last_three = lst.slice(-3, None);           // [3, 4, 5]

// Note: Wrap step in var() to avoid overload ambiguity
lst.slice(None, None, var(-1));  // Correct
```

### Tuple Access

Use `get()` helper for runtime tuple element access, or `unpack()` to convert to list:

```cpp
for (auto pair : zip(names, ages)) {
    // ✓ Use get<>() helper for runtime access
    print(get<0>(pair), get<1>(pair));

    // ✓ Or use std::get<>
    print(std::get<0>(pair), std::get<1>(pair));
}

// ✓ Or use unpack() to convert tuple to list
for (auto pair : zip(names, ages)) {
    var lst = unpack(pair);
    print(lst[size_t(0)], lst[size_t(1)]);
}
```

### DynamicVar (let macro) Printing

`DynamicVar` from `let()` **can be printed directly**:

```cpp
let(x) = 100;
let(name) = "Alice";

// ✓ Direct printing now works!
print("Value:", let(x));       // Output: Value: 100
print("Name:", let(name));     // Output: Name: Alice

// ✓ You can also convert to var if needed
var val = let(x);
print(val);  // This also works
```

**Note:** For arithmetic operations, convert to `var` first:

```cpp
let(x) = 100;
var x_val = let(x);
let(x) = x_val + 50;  // Increment by 50
print("New value:", let(x));  // Output: New value: 150
```

### File I/O

For simple file operations, prefer `read_file()` and `write_file()`:

```cpp
// ✓ Simple and clear
write_file("data.txt", "Hello\\nWorld\\n");
var content = read_file("data.txt");
print(content);

// Note: with_open macro can have issues with structured bindings
// in some contexts - use the simple functions above instead
```

### Function Parameter Orders

Be aware that parameter orders follow the library's conventions:

```cpp
// Functional programming: function first, iterable second
auto evens = filter(lambda_(x, x % var(2) == var(0)), nums);
auto doubled = map(lambda_(x, x * var(2)), nums);

// Comprehensions: expression first, iterable second, optional filter third
auto result = list_comp(
    lambda_(x, x * var(2)),     // Expression
    range(10),                   // Iterable
    lambda_(x, x % var(2) == var(0))  // Filter (optional)
);

// Reduce: function first, iterable second, initial value third
auto sum = reduce(lambda2_(acc, x, acc + x), nums, var(0));
```

## Common Pitfalls

- **C++20 required** - Won't compile with older standards
- **No automatic conversions from strings** - Use `Int()`, `Float()`, etc. when converting from strings
- **Dictionary keys** - Currently only support string keys
- **Index checking** - Out of bounds returns None instead of throwing
- **Comparison** - Different types compare as not equal (no implicit conversion)
- **Operator ambiguity with slicing** - When using `slice()` with `None`, wrap numeric literals in `var()`: `slice(None, None, var(-1))`
- **Index ambiguity** - Use `size_t` for list indices when 0 might be ambiguous: `lst[size_t(0)]`
- **DynamicVar arithmetic** - Convert `let()` variables to `var` first for arithmetic: `var v = let(x); let(x) = v + 50;`
- **Type Conversions** - The `var` type provides explicit conversion operators, so `static_cast<int>()`, `static_cast<double>()` work correctly. However, for clarity and best practice, prefer using the explicit conversion methods `.toInt()`, `.toDouble()`, `.toString()`:

  ```cpp
  var x = 42;
  int i1 = static_cast<int>(x);    // Works ✓ (returns 42)
  int i2 = x.toInt();              // Recommended ✓ (clearer intent)

  // For list/path access, both work:
  var path = list(0, 2, 4, 3);
  size_t idx1 = static_cast<size_t>(static_cast<int>(path[1]));  // Works ✓
  size_t idx2 = static_cast<size_t>(path[1].toInt());            // Clearer ✓
  ```

## Safety Features

- **Overflow Detection** - Integer arithmetic operations (+, -, \*, /) are checked for overflow and throw `PythonicOverflowError`
- **Zero Division** - Division/Modulo by zero throws `PythonicZeroDivisionError`
- **Safe Iteration** - `as_span()` provides safe views of list data

## That's It! Now getout

**AUTHOR:** MD. NASIF SADIK PRITHU  
**EMAIL:** nasifsadik9@gmail.com  
**GITHUB:** [Creamy-pie-96](https://github.com/Creamy-pie-96)
