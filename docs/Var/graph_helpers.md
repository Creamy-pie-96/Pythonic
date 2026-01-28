[⬅ Back to Table of Contents](var.md)

# Graph Helpers (if using graph type)

Graph-related helpers for `var` (if enabled in your build).

- `size_t node_count()` / `size_t edge_count()` / `bool is_connected()` / `bool has_cycle()`
- `bool has_edge(size_t from, size_t to)` / `size_t out_degree(size_t node)` / `size_t in_degree(size_t node)`
- `size_t add_node()` / `size_t add_node(const var &data)` / `void remove_node(size_t node)`
- `bool remove_edge(size_t from, size_t to, bool remove_reverse = true)`
- `void set_edge_weight(size_t from, size_t to, double weight)` / `void set_node_data(size_t node, const var &data)` / `var &get_node_data(size_t node)`
