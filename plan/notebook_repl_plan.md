# High-Level Plan: Pythonic C++ Notebook/REPL System

## Vision

- Provide two ways to use the library:
  1. As a regular C++ header library for high-performance, production code (classic usage).
  2. As a dynamic, interactive, Python-like notebook system (Jupyter-style), where users can write and execute code in cells, with persistent state and immediate feedback.

## Key Design Points

- **No macro-based magic for main() or entry points.**
- **REPL/Notebook system** will:
  - Let users write code in cells (like Jupyter).
  - Parse code, auto-inject semicolons, and handle indentation.
  - Transpile user code to C++.
  - Compile each cell as a shared library (.so) and hot-load it into a running host process.
  - Maintain a global state map (e.g., `std::map<std::string, var>`) for persistent variables across cells.
  - Each cell is wrapped as a function (e.g., `extern "C" void run_cell(State&)`).
  - The host calls the cell function, passing the state map.
  - Precompiled headers and shared libraries are used for fast compilation and execution.
- **Performance:**
  - All heavy computation (loops, tensor ops) runs at C++/machine code speed.
  - Only the cell compilation step is slower than Python, but with PCH and .so hot-loading, it is fast enough for interactive use.
- **Flexibility:**
  - Users can use the library as a normal C++ header or as a dynamic scripting environment.
  - No loss of control for power users; no toy macros.

## Why This Approach

- Avoids the pitfalls of macro-based entry points.
- Provides a real, professional workflow for both research and production.
- Enables live coding, debugging, and rapid prototyping with C++ performance.
- Bridges the gap between scripting and compiled code in the deep learning domain.

## Next Steps

- Discuss and refine the architecture.
- Prototype the transpiler and host/snippet system.
- Design the persistent state map and cell execution logic.
- Plan for error handling, debugging, and user experience.

---

This plan is based on discussions about the limitations of macro-based scripting, the power of REPLs, and the technical feasibility of a C++ notebook system using dynamic compilation and shared state.
