#!/usr/bin/env python3
"""
Comprehensive Python Benchmark for Pythonic C++ Library Comparison

This benchmark covers all major operations that the Pythonic library supports,
allowing direct comparison with the C++ implementation.

NOTE: Benchmark names MUST match exactly with C++ benchmark names for comparison.
"""

import time
import json
import functools
import random

# Benchmark configuration - must match C++ values
ITERATIONS = 1000000
SMALL_ITERATIONS = 10000
TINY_ITERATIONS = 1000
CONTAINER_SIZE = 1000
GRAPH_ITERATIONS = 1000

results = {}

def benchmark(name, iterations=1):
    """Decorator to measure function execution time."""
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            start = time.time()
            result = func(*args, **kwargs)
            end = time.time()
            py_time = (end - start) * 1000
            results[name] = py_time
            print(f"  {name}: {py_time:.3f}ms")
            return result
        return wrapper
    return decorator

# ============== ARITHMETIC OPERATIONS ==============

def benchmark_arithmetic_operations():
    print("\n=== Benchmarking Arithmetic Operations ===")
    
    @benchmark("Integer Addition")
    def int_add():
        sum_val = 0
        for i in range(ITERATIONS):
            sum_val = sum_val + 1
        return sum_val
    int_add()
    
    @benchmark("Integer Multiplication")
    def int_mul():
        prod = 1
        for i in range(ITERATIONS):
            prod = prod * 2
            if prod > 1000000:
                prod = 1
        return prod
    int_mul()
    
    @benchmark("Double Addition")
    def double_add():
        sum_val = 0.0
        for i in range(ITERATIONS):
            sum_val = sum_val + 1.5
        return sum_val
    double_add()
    
    @benchmark("Integer Division")
    def int_div():
        result = 0
        for i in range(1, ITERATIONS + 1):
            result = 100000 // i
        return result
    int_div()
    
    @benchmark("Integer Modulo")
    def int_mod():
        result = 0
        for i in range(1, ITERATIONS + 1):
            result = i % 17
        return result
    int_mod()
    
    @benchmark("Integer Comparison")
    def int_cmp():
        result = False
        for i in range(ITERATIONS):
            result = (i % 2 == 0)
        return result
    int_cmp()

# ============== STRING OPERATIONS ==============

def benchmark_string_operations():
    print("\n=== Benchmarking String Operations ===")
    
    @benchmark("String Concatenation")
    def str_concat():
        for i in range(SMALL_ITERATIONS):
            result = "Hello" + " " + "World"
        return result
    str_concat()
    
    @benchmark("String Comparison")
    def str_cmp():
        for i in range(ITERATIONS):
            result = ("hello" == "hello")
        return result
    str_cmp()
    
    s = "hello world"
    @benchmark("String upper()")
    def str_upper():
        for i in range(SMALL_ITERATIONS):
            result = s.upper()
        return result
    str_upper()
    
    s2 = "HELLO WORLD"
    @benchmark("String lower()")
    def str_lower():
        for i in range(SMALL_ITERATIONS):
            result = s2.lower()
        return result
    str_lower()
    
    s3 = "   hello world   "
    @benchmark("String strip()")
    def str_strip():
        for i in range(SMALL_ITERATIONS):
            result = s3.strip()
        return result
    str_strip()
    
    s4 = "hello world hello"
    @benchmark("String replace()")
    def str_replace():
        for i in range(SMALL_ITERATIONS):
            result = s4.replace("hello", "hi")
        return result
    str_replace()
    
    @benchmark("String find()")
    def str_find():
        for i in range(SMALL_ITERATIONS):
            result = s4.find("world")
        return result
    str_find()
    
    s5 = "one,two,three,four,five"
    @benchmark("String split()")
    def str_split():
        for i in range(SMALL_ITERATIONS):
            result = s5.split(",")
        return result
    str_split()
    
    @benchmark("String startswith()")
    def str_startswith():
        for i in range(SMALL_ITERATIONS):
            result = s.startswith("hello")
        return result
    str_startswith()
    
    @benchmark("String endswith()")
    def str_endswith():
        for i in range(SMALL_ITERATIONS):
            result = s.endswith("world")
        return result
    str_endswith()
    
    @benchmark("String isdigit()")
    def str_isdigit():
        s6 = "12345"
        for i in range(SMALL_ITERATIONS):
            result = s6.isdigit()
        return result
    str_isdigit()
    
    @benchmark("String center()")
    def str_center():
        s7 = "hello"
        for i in range(SMALL_ITERATIONS):
            result = s7.center(20, '-')
        return result
    str_center()
    
    @benchmark("String zfill()")
    def str_zfill():
        s8 = "42"
        for i in range(SMALL_ITERATIONS):
            result = s8.zfill(5)
        return result
    str_zfill()

# ============== SLICING OPERATIONS ==============

def benchmark_slicing():
    print("\n=== Benchmarking Slicing Operations ===")
    
    s = "Hello, World! This is a test."
    lst = list(range(100))
    
    @benchmark("String Slice [2:8]")
    def string_slice():
        for i in range(SMALL_ITERATIONS):
            result = s[2:8]
        return result
    string_slice()
    
    @benchmark("List Slice [2:8]")
    def list_slice():
        for i in range(SMALL_ITERATIONS):
            result = lst[2:8]
        return result
    list_slice()
    
    @benchmark("String Slice [::2]")
    def string_slice_step():
        for i in range(SMALL_ITERATIONS):
            result = s[::2]
        return result
    string_slice_step()
    
    @benchmark("List Slice [::2]")
    def list_slice_step():
        for i in range(SMALL_ITERATIONS):
            result = lst[::2]
        return result
    list_slice_step()
    
    @benchmark("List Slice [-5:-1]")
    def list_slice_neg():
        for i in range(SMALL_ITERATIONS):
            result = lst[-5:-1]
        return result
    list_slice_neg()
    
    @benchmark("List Slice [::-1] (Reverse)")
    def list_slice_rev():
        for i in range(SMALL_ITERATIONS):
            result = lst[::-1]
        return result
    list_slice_rev()

# ============== CONTAINER OPERATIONS ==============

def benchmark_container_operations():
    print("\n=== Benchmarking Container Operations ===")
    
    @benchmark("List Creation")
    def list_create():
        for i in range(SMALL_ITERATIONS):
            lst = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        return lst
    list_create()
    
    @benchmark("Dict Creation")
    def dict_create():
        for i in range(SMALL_ITERATIONS):
            d = {"a": 1, "b": 2, "c": 3, "d": 4, "e": 5}
        return d
    dict_create()
    
    @benchmark("Set Creation")
    def set_create():
        for i in range(SMALL_ITERATIONS):
            s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        return s
    set_create()
    
    @benchmark("List append()")
    def list_append():
        lst = []
        for i in range(CONTAINER_SIZE):
            lst.append(i)
        return lst
    list_append()
    
    @benchmark("List extend()")
    def list_extend():
        for i in range(SMALL_ITERATIONS):
            lst = [1, 2, 3]
            lst.extend([4, 5, 6])
        return lst
    list_extend()
    
    lst = list(range(CONTAINER_SIZE))
    @benchmark("List Index Access")
    def list_access():
        sum_val = 0
        for i in range(CONTAINER_SIZE):
            sum_val += lst[i]
        return sum_val
    list_access()
    
    d = {f"key{i}": i for i in range(CONTAINER_SIZE)}
    @benchmark("Dict Access")
    def dict_access():
        sum_val = 0
        for i in range(CONTAINER_SIZE):
            sum_val += d[f"key{i}"]
        return sum_val
    dict_access()
    
    @benchmark("Dict keys()")
    def dict_keys():
        for i in range(SMALL_ITERATIONS):
            result = list(d.keys())
        return result
    dict_keys()
    
    @benchmark("Dict values()")
    def dict_values():
        for i in range(SMALL_ITERATIONS):
            result = list(d.values())
        return result
    dict_values()
    
    @benchmark("Dict items()")
    def dict_items():
        for i in range(SMALL_ITERATIONS):
            result = list(d.items())
        return result
    dict_items()
    
    @benchmark("Set add()")
    def set_add():
        s = set()
        for i in range(CONTAINER_SIZE):
            s.add(i)
        return s
    set_add()
    
    @benchmark("'in' Operator (List)")
    def list_in():
        result = False
        for i in range(CONTAINER_SIZE):
            result = (500 in lst)
        return result
    list_in()
    
    s = set(range(CONTAINER_SIZE))
    @benchmark("'in' Operator (Set)")
    def set_in():
        result = False
        for i in range(CONTAINER_SIZE):
            result = (500 in s)
        return result
    set_in()
    
    @benchmark("'in' Operator (Dict)")
    def dict_in():
        result = False
        for i in range(CONTAINER_SIZE):
            result = ("key500" in d)
        return result
    dict_in()
    
    l1 = list(range(100))
    l2 = list(range(100))
    @benchmark("List + Operator")
    def list_concat():
        for i in range(SMALL_ITERATIONS):
            result = l1 + l2
        return result
    list_concat()
    
    @benchmark("List * Operator")
    def list_mul():
        for i in range(SMALL_ITERATIONS):
            result = l1 * 3
        return result
    list_mul()

# ============== TYPE CONVERSIONS ==============

def benchmark_type_conversions():
    print("\n=== Benchmarking Type Conversions ===")
    
    @benchmark("Int() from String")
    def conv_int():
        for i in range(SMALL_ITERATIONS):
            result = int("12345")
        return result
    conv_int()
    
    @benchmark("Float() from String")
    def conv_float():
        for i in range(SMALL_ITERATIONS):
            result = float("3.14159")
        return result
    conv_float()
    
    @benchmark("Str() from Int")
    def conv_str():
        for i in range(SMALL_ITERATIONS):
            result = str(12345)
        return result
    conv_str()
    
    @benchmark("Bool() from Int")
    def conv_bool():
        for i in range(ITERATIONS):
            result = bool(42)
        return result
    conv_bool()
    
    @benchmark("Int to Double")
    def int_to_double():
        for i in range(ITERATIONS):
            result = float(42)
        return result
    int_to_double()
    
    @benchmark("type()")
    def func_type():
        for i in range(SMALL_ITERATIONS):
            result = type("hello")
        return result
    func_type()
    
    @benchmark("isinstance()")
    def func_isinstance():
        for i in range(SMALL_ITERATIONS):
            result = isinstance("hello", str)
        return result
    func_isinstance()

# ============== FUNCTIONAL OPERATIONS ==============

def benchmark_functional():
    print("\n=== Benchmarking Functional Operations ===")
    
    lst = list(range(CONTAINER_SIZE))
    
    @benchmark("map()")
    def func_map():
        result = list(map(lambda x: x * 2, lst))
        return result
    func_map()
    
    @benchmark("filter()")
    def func_filter():
        result = list(filter(lambda x: x % 2 == 0, lst))
        return result
    func_filter()
    
    @benchmark("reduce()")
    def func_reduce():
        result = functools.reduce(lambda a, b: a + b, lst)
        return result
    func_reduce()
    
    @benchmark("Chained map + filter")
    def chained():
        result = list(filter(lambda x: x > 500, map(lambda x: x * 2, lst)))
        return result
    chained()
    
    @benchmark("Lambda Application")
    def lambda_app():
        f = lambda x: x * 2 + 1
        result = 0
        for i in range(SMALL_ITERATIONS):
            result = f(i)
        return result
    lambda_app()

# ============== GRAPH OPERATIONS ==============

def benchmark_graph():
    print("\n=== Benchmarking Graph Operations ===")
    
    try:
        import networkx as nx
        
        @benchmark("Graph Creation")
        def graph_create():
            for i in range(GRAPH_ITERATIONS):
                g = nx.DiGraph()
                g.add_nodes_from(range(6))
            return g
        graph_create()
        
        @benchmark("add_edge()")
        def add_edge():
            g = nx.DiGraph()
            g.add_nodes_from(range(6))
            for i in range(GRAPH_ITERATIONS):
                g.add_edge(0, 1, weight=1.0)
            return g
        add_edge()
        
        # Build test graph
        g = nx.DiGraph()
        g.add_nodes_from(range(6))
        g.add_edge(0, 1, weight=1.0)
        g.add_edge(0, 2, weight=1.0)
        g.add_edge(1, 3, weight=1.0)
        g.add_edge(1, 4, weight=1.0)
        g.add_edge(2, 5, weight=1.0)
        
        @benchmark("DFS Traversal")
        def dfs():
            for i in range(GRAPH_ITERATIONS):
                result = list(nx.dfs_preorder_nodes(g, 0))
            return result
        dfs()
        
        @benchmark("BFS Traversal")
        def bfs():
            for i in range(GRAPH_ITERATIONS):
                result = list(nx.bfs_tree(g, 0))
            return result
        bfs()
        
        @benchmark("has_edge()")
        def has_edge():
            result = False
            for i in range(GRAPH_ITERATIONS):
                result = g.has_edge(0, 1)
            return result
        has_edge()
        
        @benchmark("get_shortest_path()")
        def shortest():
            for i in range(100):
                result = nx.dijkstra_path(g, 0, 5)
            return result
        shortest()
        
        @benchmark("is_connected()")
        def is_conn():
            # For directed graph, use weakly_connected
            for i in range(GRAPH_ITERATIONS):
                result = nx.is_weakly_connected(g)
            return result
        is_conn()
        
        @benchmark("has_cycle()")
        def has_cycle():
            for i in range(GRAPH_ITERATIONS):
                try:
                    result = not nx.is_directed_acyclic_graph(g)
                except:
                    result = True
            return result
        has_cycle()
        
        @benchmark("topological_sort()")
        def topo():
            for i in range(100):
                result = list(nx.topological_sort(g))
            return result
        topo()
        
        @benchmark("connected_components()")
        def components():
            for i in range(100):
                result = list(nx.weakly_connected_components(g))
            return result
        components()
        
    except ImportError:
        print("  [Skipped - networkx not installed]")

# ============== LOOPS ==============

def benchmark_loops():
    print("\n=== Benchmarking Loop Constructs ===")
    
    @benchmark("range() Iteration")
    def range_iter():
        sum_val = 0
        for i in range(ITERATIONS):
            sum_val += i
        return sum_val
    range_iter()
    
    @benchmark("range(start, stop) Iteration")
    def range_start_stop():
        sum_val = 0
        for i in range(100, ITERATIONS):
            sum_val += i
        return sum_val
    range_start_stop()
    
    @benchmark("range(start, stop, step) Iteration")
    def range_step():
        sum_val = 0
        for i in range(0, ITERATIONS, 2):
            sum_val += i
        return sum_val
    range_step()
    
    lst = list(range(CONTAINER_SIZE))
    @benchmark("for_in with List")
    def for_in():
        sum_val = 0
        for x in lst:
            sum_val += x
        return sum_val
    for_in()
    
    @benchmark("enumerate()")
    def enum():
        sum_val = 0
        for i, x in enumerate(lst):
            sum_val += i + x
        return sum_val
    enum()
    
    lst2 = list(range(CONTAINER_SIZE))
    @benchmark("zip()")
    def zip_iter():
        sum_val = 0
        for a, b in zip(lst, lst2):
            sum_val += a + b
        return sum_val
    zip_iter()
    
    d = {f"key{i}": i for i in range(CONTAINER_SIZE)}
    @benchmark("Dict Iteration (items())")
    def dict_iter():
        sum_val = 0
        for k, v in d.items():
            sum_val += v
        return sum_val
    dict_iter()

# ============== SORTING ==============

def benchmark_sorting():
    print("\n=== Benchmarking Sorting Operations ===")
    
    @benchmark("sorted() Ascending")
    def sort_asc():
        for i in range(TINY_ITERATIONS):
            lst = [random.randint(0, 1000) for _ in range(100)]
            result = sorted(lst)
        return result
    sort_asc()
    
    @benchmark("sorted() Descending")
    def sort_desc():
        for i in range(TINY_ITERATIONS):
            lst = [random.randint(0, 1000) for _ in range(100)]
            result = sorted(lst, reverse=True)
        return result
    sort_desc()
    
    @benchmark("Large List Sorting (1000 elements)")
    def large_sort():
        for i in range(100):
            lst = [random.randint(0, 10000) for _ in range(1000)]
            result = sorted(lst)
        return result
    large_sort()

# ============== BUILT-IN FUNCTIONS ==============

def benchmark_builtins():
    print("\n=== Benchmarking Built-in Functions ===")
    
    lst = list(range(CONTAINER_SIZE))
    
    @benchmark("len()")
    def func_len():
        for i in range(ITERATIONS):
            result = len(lst)
        return result
    func_len()
    
    @benchmark("sum()")
    def func_sum():
        for i in range(SMALL_ITERATIONS):
            result = sum(lst)
        return result
    func_sum()
    
    @benchmark("min()")
    def func_min():
        for i in range(SMALL_ITERATIONS):
            result = min(lst)
        return result
    func_min()
    
    @benchmark("max()")
    def func_max():
        for i in range(SMALL_ITERATIONS):
            result = max(lst)
        return result
    func_max()
    
    @benchmark("abs()")
    def func_abs():
        for i in range(ITERATIONS):
            result = abs(-42)
        return result
    func_abs()
    
    all_true = [True] * 100
    @benchmark("all()")
    def func_all():
        for i in range(SMALL_ITERATIONS):
            result = all(all_true)
        return result
    func_all()
    
    some_true = [False] * 99 + [True]
    @benchmark("any()")
    def func_any():
        for i in range(SMALL_ITERATIONS):
            result = any(some_true)
        return result
    func_any()
    
    @benchmark("pow()")
    def func_pow():
        for i in range(SMALL_ITERATIONS):
            result = pow(2, 10)
        return result
    func_pow()
    
    import math
    @benchmark("sqrt()")
    def func_sqrt():
        for i in range(SMALL_ITERATIONS):
            result = math.sqrt(144)
        return result
    func_sqrt()
    
    @benchmark("floor()")
    def func_floor():
        for i in range(SMALL_ITERATIONS):
            result = math.floor(3.7)
        return result
    func_floor()
    
    @benchmark("ceil()")
    def func_ceil():
        for i in range(SMALL_ITERATIONS):
            result = math.ceil(3.2)
        return result
    func_ceil()
    
    @benchmark("round()")
    def func_round():
        for i in range(SMALL_ITERATIONS):
            result = round(3.14159, 2)
        return result
    func_round()

# ============== MAIN ==============

def main():
    print("==================================================")
    print("    PYTHON COMPREHENSIVE PERFORMANCE BENCHMARK    ")
    print("==================================================")
    print(f"\nConfiguration:")
    print(f"  Iterations: {ITERATIONS}")
    print(f"  Small Iterations: {SMALL_ITERATIONS}")
    print(f"  Tiny Iterations: {TINY_ITERATIONS}")
    print(f"  Container Size: {CONTAINER_SIZE}")
    print(f"  Graph Iterations: {GRAPH_ITERATIONS}")
    
    benchmark_arithmetic_operations()
    benchmark_string_operations()
    benchmark_slicing()
    benchmark_container_operations()
    benchmark_loops()
    benchmark_functional()
    benchmark_sorting()
    benchmark_builtins()
    benchmark_type_conversions()
    benchmark_graph()
    
    print("\n==================================================")
    print("            BENCHMARK COMPLETE                    ")
    print("==================================================")
    
    # Write results to JSON for C++ to read
    with open("python_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"\n✓ Results saved to python_results.json ({len(results)} benchmarks)")

if __name__ == "__main__":
    main()
