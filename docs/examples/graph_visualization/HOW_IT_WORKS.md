# Neural Network Visualization - How It Works

This is an interactive neural network visualization built with the **Pythonic C++ library** and **Dear ImGui**. It demonstrates real-time signal propagation, backpropagation, and physics-based network layout.

## 🎯 Key Features

### 1. **Interactive Physics Simulation**
- **Force-directed graph layout** with repulsion and spring forces
- **Layer-based gravity** keeps networks aligned left-to-right (input → output)
- **Distance-based forces**: nodes repel when close, attract when far
- **Drag support**: drag individual nodes or grab the center handle to move the entire graph

### 2. **Signal Propagation Visualization**
- **Forward signals** (yellow/gold): Information flowing from input to output
- **Backward signals** (red): Error gradients during backpropagation
- **Signal trails**: Comet-like trails show the path signals have traveled
- **Adaptive edge tension**: Edges pulse and brighten when signals flow through them

### 3. **Network Topologies**
- **Simple** (4-6-2): Basic feedforward network
- **Deep** (4-8-8-6-3): Multi-layer network
- **Dense** (6-12-12-4): Fully-connected layers
- **Random Spaghetti**: Chaotic connections for testing
- **Residual** (4-8-8-8-8-4): Deep network with skip connections (cyan edges)
- **Custom**: Define your own topology (e.g., `4-8-6-8-12-2`)

### 4. **Skip Connections (Residual Networks)**
- **Cyan colored edges** jump 1-2 layers ahead
- Enable via checkbox: "Use Skip Connections (Residual)"
- Allows gradients to flow more easily through deep networks
- Similar to ResNet architecture

### 5. **Visual Feedback**
- **Node colors**:
  - Ash grey: Inactive neurons
  - Light up: Activation from signals
  - Purple: Hovered node (with all connected edges highlighted)
- **Edge colors**:
  - Grey: Regular forward edges
  - Cyan: Skip connections (residual paths)
  - Purple: Edges connected to hovered node
- **Edge thickness**: Proportional to weight + activity boost

## 🔧 How to Use

### Basic Controls
1. **Select topology**: Use the dropdown to choose network architecture
2. **Custom topology**: Enter format like `4-8-6-2` (layers separated by dashes)
3. **Enable skip connections**: Check the residual checkbox for custom topologies
4. **Drag nodes**: Click and drag any neuron to move it
5. **Drag graph**: Hover near the center to reveal the drag handle (blue pulsing circle)
6. **Hover effects**: Move mouse over nodes to highlight them and their connections

### Physics Parameters
- **Repulsion** (10-1000): How strongly nodes push each other away
- **Ideal Distance** (50-500): Target separation between nodes
- **Signal Speed** (0.1-10.0): How fast signals travel along edges

### Understanding the Simulation

#### Phase States
The network cycles through three phases:
1. **INPUT**: Initial signals injected into input layer (layer 0)
2. **FORWARD**: Signals propagate toward output layer
3. **BACKWARD**: Error signals flow backward, updating weights

#### Learning Process
- Forward signals reach output → trigger backward signals
- Backward signals (red) update edge weights as they travel
- Edge weights change randomly (simulating gradient descent)
- 70% of backward signals are dropped to reduce clutter

## 🎨 Visual Effects Explained

### 1. **Adaptive Edge Tension**
When a signal travels over an edge:
- The edge temporarily increases in brightness and thickness
- Creates a "tightening" effect showing active connections
- Decays gradually (0.92× per frame)

### 2. **Signal Trails (Comet Effect)**
Signals leave behind dimmer copies as they move:
- Forward trails: Dimmer yellow dots
- Backward trails: Dimmer red dots
- Trails fade out over ~1 second
- Makes signal direction easier to track in dense networks

### 3. **Layer-Based Gravity**
Instead of pulling toward a single center point:
- Each layer has its own target X position
- Input layer (0) → X = 100
- Each next layer → X += 200
- Keeps information flow visually clear (left → right)

## 🧠 Technical Implementation

### Built With
- **Pythonic C++ Library**: Python-like syntax for C++
  - `var` type: Dynamic typing
  - `list()`, `dict()`, `graph()`: Python-style containers
  - Fast path caching for performance
- **Dear ImGui**: Immediate mode GUI
- **GLFW + OpenGL3**: Graphics rendering

### Key Optimizations
- **Atomic signal counter**: O(1) instead of O(N) counting
- **Adjacency lists**: Fast edge lookup (O(edges_per_node))
- **Multithreaded physics**: Repulsion calculated across 6 threads
- **Native double arithmetic**: Hot loops use C++ primitives
- **Batch signal processing**: Reduces pythonic overhead

### Graph API Usage
The simulation uses the Pythonic library's graph features:
```cpp
var g = graph(40);  // Create graph with 40 nodes
g.add_edge(u, v, weight, 0.0, true);  // Add directed edge
g.remove_edge(u, v);  // Remove connection
```

## 📊 Performance Notes

- Handles **100+ nodes** smoothly
- **11,000+ edges** on large networks (e.g., `14-72-72-64-18`)
- Signal processing: ~100-150μs per backward trigger
- Physics simulation runs at 60 FPS on modern hardware

## 🚀 Try These Experiments

1. **Dense Network**: Set topology to "Dense" and watch signal saturation
2. **Residual Flow**: Create `4-8-8-8-8-4` with skip connections enabled
3. **Chaos Mode**: Random Spaghetti topology shows emergent behavior
4. **Custom Deep**: Try `8-16-32-64-32-16-8` for a symmetric deep network
5. **Gravity Test**: Change ideal distance to 100 (tight) vs 500 (spread out)

## 🐛 Debugging Features

- **Edge count display**: Shows total connections created
- **Phase indicator**: Current simulation state
- **Signal count**: Active signals in the network
- **Debug output**: Terminal shows edge creation and timing data

## 📝 Code Structure

- **SynapseSim class**: Main simulation logic
- **reset_network()**: Builds topology and initializes nodes/edges
- **trigger_node()**: Creates signals when neuron activates
- **update_signals()**: Moves signals along edges, handles arrivals
- **update_physics()**: Force-directed layout with multithreading
- **Main loop**: ImGui rendering with visual effects

---

**Built by**: Neural network visualization enthusiast  
**License**: Use freely, learn endlessly  
**Powered by**: [Pythonic C++ Library](https://github.com/Creamy-pie-96/Pythonic)
