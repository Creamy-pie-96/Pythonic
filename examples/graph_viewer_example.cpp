/**
 * @file graph_viewer_example.cpp
 * @brief Example demonstrating the interactive graph viewer
 *
 * Build:
 *   cmake -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON ..
 *   make graph_viewer_example
 *
 * Run:
 *   ./graph_viewer_example
 */

#include <pythonic/pythonic.hpp>
#include <iostream>

using namespace py;

int main(int argc, char** argv)
{
    // Check for test mode
    bool test_mode = (argc > 1 && std::string(argv[1]) == "--test");

    std::cout << "=== Pythonic Graph Viewer Example ===" << std::endl;

    // Create a directed acyclic graph
    var g = graph(6);

    // Add some edges to create a DAG structure
    g.add_edge(0, 1, 1.0, 0.0, true);  // 0 -> 1 (directed)
    g.add_edge(0, 2, 1.5, 0.0, true);  // 0 -> 2 (directed)
    g.add_edge(1, 3, 2.0, 0.0, true);  // 1 -> 3 (directed)
    g.add_edge(2, 3, 1.0, 0.0, true);  // 2 -> 3 (directed)
    g.add_edge(2, 4, 0.5, 0.0, true);  // 2 -> 4 (directed)
    g.add_edge(3, 5, 1.0, 0.0, true);  // 3 -> 5 (directed)
    g.add_edge(4, 5, 2.0, 0.0, true);  // 4 -> 5 (directed)

    // Add some node metadata
    g.set_node_data(0, "Input");
    g.set_node_data(1, "Layer1-A");
    g.set_node_data(2, "Layer1-B");
    g.set_node_data(3, "Layer2");
    g.set_node_data(4, "Skip");
    g.set_node_data(5, "Output");

    // Print graph info
    std::cout << "Graph created:" << std::endl;
    std::cout << "  Nodes: " << g.node_count() << std::endl;
    std::cout << "  Edges: " << g.edge_count() << std::endl;
    std::cout << "  Has cycle: " << (g.has_cycle() ? "yes" : "no") << std::endl;
    std::cout << "  Is connected: " << (g.is_connected() ? "yes" : "no") << std::endl;

    // Print topological order
    std::cout << "  Topological order: ";
    var topo = g.topological_sort();
    for (size_t i = 0; i < topo.len(); ++i)
    {
        std::cout << topo[i].toInt();
        if (i < topo.len() - 1)
            std::cout << " -> ";
    }
    std::cout << std::endl;

    if (test_mode)
    {
        // Non-visual test: just verify graph operations work
        std::cout << "\n[TEST MODE] Skipping visual test" << std::endl;

        // Test basic operations
        if (g.node_count() != 6)
        {
            std::cerr << "FAIL: node_count mismatch" << std::endl;
            return 1;
        }

        if (g.has_cycle())
        {
            std::cerr << "FAIL: DAG should not have cycle" << std::endl;
            return 1;
        }

        // Test that topological sort worked
        if (topo.len() != 6)
        {
            std::cerr << "FAIL: topological sort length mismatch" << std::endl;
            return 1;
        }

        std::cout << "PASS: All tests passed" << std::endl;
        return 0;
    }

#ifdef PYTHONIC_ENABLE_GRAPH_VIEWER
    std::cout << "\nOpening interactive viewer..." << std::endl;
    std::cout << "  Click on nodes to trigger signal flow" << std::endl;
    std::cout << "  Drag nodes to move them" << std::endl;
    std::cout << "  Click lock icon to switch to Edit mode" << std::endl;
    std::cout << "  Press ESC to close" << std::endl;

    // Open the interactive graph viewer
    g.show();

    // After closing, the graph may have been modified
    std::cout << "\nViewer closed. Final graph state:" << std::endl;
    std::cout << "  Nodes: " << g.node_count() << std::endl;
    std::cout << "  Edges: " << g.edge_count() << std::endl;
#else
    std::cout << "\nGraph viewer not enabled." << std::endl;
    std::cout << "Rebuild with: cmake -DPYTHONIC_ENABLE_GRAPH_VIEWER=ON .." << std::endl;
#endif

    return 0;
}
