[⬅ Back to Table of Contents](../index.md)
[⬅ Back to Quick Start](quickstart.md)
# Cookbook

Practical recipes:

Practical recipes for using the Pythonic C++ library.

---

## 1. Parse CSV into `list<dict>` using `var`

**Goal:** Read a CSV file and store each row as a dictionary in a list, using the dynamic `var` type.

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic;

auto rows = list<dict<str, var>>{};
auto file = open("data.csv");
auto header = file.readline().strip().split(',');

for (auto line : file) {
	auto values = line.strip().split(',');
	dict<str, var> row;
	for (size_t i = 0; i < header.size(); ++i)
		row[header[i]] = values[i];
	rows.append(row);
}
```

**Explanation:**

- Reads the header to get column names.
- For each line, splits values and builds a dictionary.
- Appends each row dictionary to a list.

---

## 2. Build and run a shortest-path example using `graph`

**Goal:** Create a graph, add edges, and compute shortest paths.

```cpp
#include <pythonic/Graph.hpp>
using namespace pythonic;

graph g(5); // 5 nodes
g.add_edge(0, 1, 2);
g.add_edge(1, 2, 3);
g.add_edge(0, 3, 1);
g.add_edge(3, 4, 4);

auto dist = g.dijkstra(0);
print(dist); // Output: shortest distances from node 0
```

**Explanation:**

- Creates a graph with 5 nodes.
- Adds weighted edges.
- Runs Dijkstra’s algorithm from node 0.

---

## 3. High-performance loop pattern using `is_*()` + `as_*_unchecked()`

**Goal:** Efficiently process a heterogeneous list by type-checking and fast casting.

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic;

list<var> items = {1, "hello", 3.14, 42};

for (const auto& x : items) {
	if (x.is_int()) {
		int val = x.as_int_unchecked(); // Fast, no type check
		print("int:", val);
	} else if (x.is_str()) {
		str s = x.as_str_unchecked();
		print("str:", s);
	}
}
```

**Explanation:**

- Use `is_*()` to check type.
- Use `as_*_unchecked()` for fast, unchecked access after confirming type.

---

## 4. Combining comprehensions and functional helpers

**Goal:** Use list comprehensions and functional utilities for concise data processing.

```cpp
#include <pythonic/pythonic.hpp>
using namespace pythonic;

list<int> nums = {1, 2, 3, 4, 5};
auto squares = [x * x for x : nums if x % 2 == 0]; // List comprehension

auto doubled = map([](int x) { return x * 2; }, nums); // Functional map

print(squares); // [4, 16]
print(list<int>(doubled)); // [2, 4, 6, 8, 10]
```

**Explanation:**

- List comprehensions for filtering and transforming.
- Functional helpers like `map` for concise operations.

---
## That's all, Now get out and touch grass!
[⬅ Back to main readme](../../README.md)