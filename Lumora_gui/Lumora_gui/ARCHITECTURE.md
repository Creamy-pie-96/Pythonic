# LUMORA GUI Architecture

## Overview

A modular, Qt6-based neural IDE for real-time deep learning introspection.
Designed for integration with any ML backend via a simple, fast API.

## Directory Structure

```
Lumora_gui/
├── CMakeLists.txt
├── main.cpp
│
├── core/                    # Backend-agnostic data types & API
│   ├── LumoraAPI.hpp        # Integration interface (IPC/shared memory)
│   ├── ModelState.hpp       # Model graph, layer, tensor representations
│   ├── TrainingState.hpp    # Epoch, step, loss, metrics
│   ├── CommandQueue.hpp     # Commands to backend (pause, update config)
│   └── Types.hpp            # Common types, enums
│
├── widgets/                 # Reusable Qt widgets
│   ├── PulseDashboard/      # Global state view
│   │   ├── PulseDashboard.hpp/cpp
│   │   ├── MetricsChart.hpp/cpp
│   │   └── StatusTicker.hpp/cpp
│   │
│   ├── XRayGraph/           # Model graph canvas
│   │   ├── XRayCanvas.hpp/cpp
│   │   ├── GraphNode.hpp/cpp
│   │   ├── GraphEdge.hpp/cpp
│   │   └── TimelineScrubber.hpp/cpp
│   │
│   ├── CortexInspector/     # Layer inspection panel
│   │   ├── CortexInspector.hpp/cpp
│   │   ├── DistributionHistogram.hpp/cpp
│   │   └── DeltaView.hpp/cpp
│   │
│   ├── HyperparameterValve/ # Control plane
│   │   ├── HyperparameterValve.hpp/cpp
│   │   ├── LiveSlider.hpp/cpp
│   │   └── ScriptConsole.hpp/cpp
│   │
│   ├── CortexLibrary/       # Block assembly sidebar
│   │   ├── CortexLibrary.hpp/cpp
│   │   ├── DraggableBlock.hpp/cpp
│   │   └── ConstraintViewer.hpp/cpp
│   │
│   └── Observer/            # Ambient presence indicator
│       └── Observer.hpp/cpp
│
├── panels/                  # Dock panel containers
│   ├── DashboardPanel.hpp/cpp
│   ├── InspectorPanel.hpp/cpp
│   └── ControlPanel.hpp/cpp
│
├── theme/                   # Dark theme & styling
│   ├── DarkTheme.hpp/cpp
│   └── NeonPalette.hpp
│
└── MainWindow.hpp/cpp       # Main window orchestration
```

## Core API Design

The API is designed for:
1. **Speed**: Lock-free ring buffers for metrics, shared memory for tensors
2. **Modularity**: GUI knows nothing about Lumora internals
3. **Simplicity**: Subscribe to data streams, send commands

### Integration Points

```cpp
namespace lumora::gui {

// Backend pushes data to GUI via these interfaces
struct IDataProvider {
    virtual ~IDataProvider() = default;
    
    // Training State (called ~60Hz for smooth updates)
    virtual TrainingState getTrainingState() = 0;
    
    // Model Structure (called on model change)
    virtual ModelGraph getModelGraph() = 0;
    
    // Layer Details (on-demand)
    virtual LayerStats getLayerStats(LayerId id) = 0;
    
    // Tensor Data (for visualization)
    virtual TensorView getTensorView(TensorId id) = 0;
    
    // Anomaly Events
    virtual std::vector<AnomalyEvent> getAnomalies() = 0;
};

// GUI sends commands to backend via this interface
struct ICommandHandler {
    virtual ~ICommandHandler() = default;
    
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void setLearningRate(float lr) = 0;
    virtual void setHyperparameter(const std::string& name, double value) = 0;
    virtual void executeScript(const std::string& script) = 0;
    virtual void saveCheckpoint(const std::string& path) = 0;
    virtual void loadCheckpoint(const std::string& path) = 0;
};

// Main entry point for integration
class LumoraGUI {
public:
    explicit LumoraGUI(IDataProvider* provider, ICommandHandler* handler);
    
    void show();
    void update(); // Call from main loop or timer
    
    // Optional: Run standalone with internal Qt event loop
    int exec(int argc, char* argv[]);
    
private:
    std::unique_ptr<MainWindow> m_window;
};

} // namespace lumora::gui
```

## Data Types

```cpp
namespace lumora::gui {

enum class SystemState {
    Idle,
    Training,
    Paused,
    Diverging,
    NaNDetected,
    Error
};

struct TrainingState {
    SystemState state;
    uint64_t epoch;
    uint64_t step;
    double loss;
    double accuracy;
    double gradientNorm;
    double learningRate;
    double samplesPerSecond;
    double gpuMemoryGB;
    std::string statusMessage;
};

struct LayerStats {
    LayerId id;
    std::string name;
    std::string type;
    double executionTimeMs;
    size_t memoryBytes;
    size_t parameterCount;
    
    // Distributions (256 bins)
    std::array<float, 256> weightHistogram;
    std::array<float, 256> gradientHistogram;
    std::array<float, 256> activationHistogram;
    
    // Delta from N steps ago
    std::array<float, 256> weightDelta;
    std::array<float, 256> gradientDelta;
    
    // Anomaly indicators
    float deadNeuronRatio;
    bool hasNaN;
    bool hasSaturatedActivations;
};

struct GraphNode {
    LayerId id;
    std::string name;
    std::string type;
    float x, y;           // Position
    float width, height;  // Size
    
    // Visual state
    float activationIntensity;  // 0-1, for glow
    float gradientMagnitude;    // For edge coloring
    bool isBottleneck;
    bool hasAnomaly;
};

struct GraphEdge {
    LayerId from;
    LayerId to;
    float gradientNorm;  // For thickness/color
    TensorShape shape;   // For hover info
};

struct ModelGraph {
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    uint64_t version;  // Increments on structure change
};

struct AnomalyEvent {
    uint64_t step;
    LayerId sourceLayer;
    std::string type;  // "NaN", "Inf", "Exploding", "Vanishing"
    std::string message;
    std::vector<LayerId> propagationPath;
};

} // namespace lumora::gui
```

## Widget Design Principles

1. **Self-Contained**: Each widget is a standalone component
2. **Data-Driven**: Widgets receive data, they don't fetch it
3. **Reactive**: Minimal CPU when data hasn't changed
4. **Theme-Aware**: All colors from NeonPalette

## Performance Considerations

- **Ring Buffers**: Metrics use lock-free ring buffers (1024 entries)
- **Dirty Flags**: Only repaint when data actually changes
- **LOD**: Graph simplifies when zoomed out
- **Lazy Loading**: Tensor views loaded on-demand
- **GPU Rendering**: Use QOpenGLWidget for graph canvas
