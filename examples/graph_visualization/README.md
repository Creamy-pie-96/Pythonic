# Graph Visualization - Neural Network Simulation

An interactive neural network visualization demonstrating the **Pythonic C++ Library**'s graph API with real-time signal propagation, physics simulation, and residual connections.

## Features

- 🧠 **Interactive Neural Networks**: Visualize forward and backward signal propagation
- 🌐 **Multiple Topologies**: Simple, Deep, Dense, Residual, Custom configurations
- ⚡ **Skip Connections**: Residual network support with cyan-colored skip edges
- 🎨 **Visual Effects**: Edge tension, signal trails, layer-based gravity
- 🎯 **Physics Simulation**: Force-directed layout with multithreaded calculations
- 🖱️ **Interactive Controls**: Drag nodes, hover to highlight, adjustable parameters

## Quick Start

### Build Instructions

```bash
mkdir build && cd build
cmake ..
make
./mainapp
```

### Dependencies

- **Pythonic C++ Library**: Installed in `../../install/`
- **GLFW3**: Window and input handling
- **OpenGL**: Graphics rendering
- **Dear ImGui**: Included in `external/imgui`

### Usage

1. **Select a topology** from the dropdown menu
2. **Adjust physics parameters** (repulsion, ideal distance, signal speed)
3. **Enable skip connections** for custom or residual networks
4. **Drag nodes** to rearrange the network
5. **Hover over nodes** to highlight connections

## How It Works

See [HOW_IT_WORKS.md](HOW_IT_WORKS.md) for detailed documentation on:
- Signal propagation mechanics
- Physics simulation algorithm
- Visual effects implementation
- Performance optimizations
- Graph API usage examples

## Example Topologies

- **Simple** (`4-6-2`): Basic feedforward network
- **Deep** (`4-8-8-6-3`): Multi-layer architecture
- **Residual** (`4-8-8-8-8-4`): With skip connections
- **Custom**: Define your own (e.g., `8-16-32-16-8`)

## Key Features

### Residual Networks
- Enable "Use Skip Connections" checkbox
- Creates cyan-colored skip edges that jump 1-2 layers
- Helps gradients flow through deep networks (like ResNet)

### Visual Feedback
- **Yellow signals**: Forward propagation (input → output)
- **Red signals**: Backpropagation (error gradients)
- **Cyan edges**: Skip connections (residual paths)
- **Pulsing edges**: Activity-based brightness and thickness

### Physics Controls
- **Repulsion**: Node separation strength (10-1000)
- **Ideal Distance**: Target node spacing (50-500)
- **Signal Speed**: Propagation velocity (0.1-10.0)

## Performance

- Handles **100+ nodes** smoothly
- **11,000+ edges** on large networks
- Multithreaded physics (6 threads)
- Optimized with native arithmetic and adjacency lists

## Pythonic API Examples

```cpp
// Create graph with nodes
var net = graph(40);

// Add directed weighted edge
net.add_edge(from_node, to_node, weight, 0.0, true);

// Remove edge
net.remove_edge(from_node, to_node);

// Check edge existence
if (net.has_edge(u, v)) {
    double w = net.get_edge_weight(u, v);
}
```

## License

Free to use and learn from. Built with the Pythonic C++ Library.

---

**For more details**, see [HOW_IT_WORKS.md](HOW_IT_WORKS.md)
