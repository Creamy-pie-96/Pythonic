#!/usr/bin/env python3
import time
import sys
import json

# Benchmark configuration
ITERATIONS = 1000000
CONTAINER_SIZE = 1000
SMALL_ITERATIONS = 10000

results = {}

def benchmark_arithmetic_operations():
    print("\n=== Benchmarking Arithmetic Operations ===")
    
    # Integer addition
    start = time.time()
    sum_val = 0
    for i in range(ITERATIONS):
        sum_val = sum_val + 1
    end = time.time()
    py_time = (end - start) * 1000
    results["Integer Addition"] = py_time
    print(f"  Integer Addition: {py_time:.3f}ms")
    
    # Integer multiplication
    start = time.time()
    prod = 1
    for i in range(ITERATIONS):
        prod = prod * 2
        if prod > 1000000:
            prod = 1
    end = time.time()
    py_time = (end - start) * 1000
    results["Integer Multiplication"] = py_time
    print(f"  Integer Multiplication: {py_time:.3f}ms")
    
    # Double operations
    start = time.time()
    sum_val = 0.0
    for i in range(ITERATIONS):
        sum_val = sum_val + 1.5
    end = time.time()
    py_time = (end - start) * 1000
    results["Double Addition"] = py_time
    print(f"  Double Addition: {py_time:.3f}ms")
    
    # Comparisons
    start = time.time()
    result = False
    for i in range(ITERATIONS):
        result = (i % 2 == 0)
    end = time.time()
    py_time = (end - start) * 1000
    results["Integer Comparison"] = py_time
    print(f"  Integer Comparison: {py_time:.3f}ms")

def benchmark_string_operations():
    print("\n=== Benchmarking String Operations ===")
    
    # String concatenation
    start = time.time()
    for i in range(SMALL_ITERATIONS):
        result = "Hello" + " " + "World"
    end = time.time()
    py_time = (end - start) * 1000
    results["String Concatenation"] = py_time
    print(f"  String Concatenation: {py_time:.3f}ms")
    
    # String comparison
    start = time.time()
    for i in range(ITERATIONS):
        result = ("hello" == "hello")
    end = time.time()
    py_time = (end - start) * 1000
    results["String Comparison"] = py_time
    print(f"  String Comparison: {py_time:.3f}ms")

def benchmark_container_creation():
    print("\n=== Benchmarking Container Creation ===")
    
    # List creation
    start = time.time()
    for i in range(SMALL_ITERATIONS):
        lst = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    end = time.time()
    py_time = (end - start) * 1000
    results["List Creation (10 elements)"] = py_time
    print(f"  List Creation: {py_time:.3f}ms")
    
    # Set creation
    start = time.time()
    for i in range(SMALL_ITERATIONS):
        s = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    end = time.time()
    py_time = (end - start) * 1000
    results["Set Creation (10 elements)"] = py_time
    print(f"  Set Creation: {py_time:.3f}ms")

def benchmark_container_operations():
    print("\n=== Benchmarking Container Operations ===")
    
    # List append
    start = time.time()
    lst = []
    for i in range(CONTAINER_SIZE):
        lst.append(i)
    end = time.time()
    py_time = (end - start) * 1000
    results["List Append"] = py_time
    print(f"  List Append: {py_time:.3f}ms")
    
    # List access
    lst = list(range(CONTAINER_SIZE))
    start = time.time()
    sum_val = 0
    for i in range(CONTAINER_SIZE):
        sum_val += lst[i]
    end = time.time()
    py_time = (end - start) * 1000
    results["List Access (indexed)"] = py_time
    print(f"  List Access: {py_time:.3f}ms")
    
    # Set insertion
    start = time.time()
    s = set()
    for i in range(CONTAINER_SIZE):
        s.add(i)
    end = time.time()
    py_time = (end - start) * 1000
    results["Set Insertion"] = py_time
    print(f"  Set Insertion: {py_time:.3f}ms")
    
    # Dict insertion
    start = time.time()
    d = {}
    for i in range(CONTAINER_SIZE):
        d[f"key{i}"] = i
    end = time.time()
    py_time = (end - start) * 1000
    results["Dict Insertion"] = py_time
    print(f"  Dict Insertion: {py_time:.3f}ms")

def benchmark_container_operators():
    print("\n=== Benchmarking Container Operators ===")
    
    # Set union
    s1 = set(range(100))
    s2 = set(range(50, 150))
    start = time.time()
    for i in range(SMALL_ITERATIONS):
        result = s1 | s2
    end = time.time()
    py_time = (end - start) * 1000
    results["Set Union (|)"] = py_time
    print(f"  Set Union: {py_time:.3f}ms")
    
    # List concatenation
    l1 = list(range(100))
    l2 = list(range(100))
    start = time.time()
    for i in range(SMALL_ITERATIONS):
        result = l1 + l2
    end = time.time()
    py_time = (end - start) * 1000
    results["List Concatenation (|)"] = py_time
    print(f"  List Concatenation: {py_time:.3f}ms")

def benchmark_loops():
    print("\n=== Benchmarking Loop Constructs ===")
    
    # For loop with range
    start = time.time()
    sum_val = 0
    for i in range(ITERATIONS):
        sum_val += i
    end = time.time()
    py_time = (end - start) * 1000
    results["Loop Iteration (for_in + range)"] = py_time
    print(f"  Loop (for + range): {py_time:.3f}ms")
    
    # Loop over container
    lst = list(range(CONTAINER_SIZE))
    start = time.time()
    sum_val = 0
    for x in lst:
        sum_val += x
    end = time.time()
    py_time = (end - start) * 1000
    results["Loop over Container (for_in)"] = py_time
    print(f"  Loop over Container: {py_time:.3f}ms")

def benchmark_functional():
    print("\n=== Benchmarking Functional Operations ===")
    
    # Map operation
    lst = list(range(CONTAINER_SIZE))
    start = time.time()
    result = list(map(lambda x: x * 2, lst))
    end = time.time()
    py_time = (end - start) * 1000
    results["Map (transform)"] = py_time
    print(f"  Map: {py_time:.3f}ms")
    
    # Filter operation
    start = time.time()
    result = list(filter(lambda x: x % 2 == 0, lst))
    end = time.time()
    py_time = (end - start) * 1000
    results["Filter"] = py_time
    print(f"  Filter: {py_time:.3f}ms")

def main():
    print("==================================================")
    print("      PYTHON PERFORMANCE BENCHMARK               ")
    print("==================================================")
    print(f"\nConfiguration:")
    print(f"  Iterations: {ITERATIONS}")
    print(f"  Small Iterations: {SMALL_ITERATIONS}")
    print(f"  Container Size: {CONTAINER_SIZE}")
    
    benchmark_arithmetic_operations()
    benchmark_string_operations()
    benchmark_container_creation()
    benchmark_container_operations()
    benchmark_container_operators()
    benchmark_loops()
    benchmark_functional()
    
    print("\n==================================================")
    print("            BENCHMARK COMPLETE                    ")
    print("==================================================")
    
    # Write results to JSON for C++ to read
    with open("python_results.json", "w") as f:
        json.dump(results, f, indent=2)
    
    print("\n✓ Results saved to python_results.json")

if __name__ == "__main__":
    main()
