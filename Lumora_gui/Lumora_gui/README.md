# Lumora GUI

**Neural Network IDE for Deep Learning Introspection**

Lumora is a Qt6-based visualization and control interface for real-time machine learning training introspection. Think of it as a "game engine editor" for neural networks - a dense, information-rich IDE that lets you see inside your model while it trains.

![Lumora Screenshot](docs/screenshot.png)

## Features

### 🫀 Pulse Dashboard
Real-time training metrics with TensorBoard-style charts:
- Scrolling loss and accuracy curves
- Live learning rate, ETA, and throughput displays
- Status ticker for events and notifications
- Pulsing status indicator

### 🔬 X-Ray Model Graph
Interactive node-based model topology:
- Visual layer representation by type
- Gradient flow visualization on edges
- Pan/zoom navigation
- Click-to-inspect integration

### 🧠 Cortex Inspector
Deep layer analysis panel:
- Weight and gradient histograms
- Dead neuron detection
- Gradient norm health indicators
- Layer freeze/unfreeze controls

### ⚡ Hyperparameter Valve
Live training parameter control:
- Real-time sliders with log-scale support
- "Blast Radius" impact indicators
- Commit/revert staged changes
- Embedded Lua/Python script console

### 👁️ Observer
Ambient presence entity:
- Cursor-tracking animated eyes
- State-reactive behavior (blinks, expressions)
- Non-intrusive system health indicator
- Optional - can be disabled

## Quick Start

### Prerequisites
- Qt 6.2+
- C++17 compiler
- CMake 3.16+

### Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./LumoraGUI
```

### Integration

Lumora is designed as a **frontend only** - your ML backend provides the data.

```cpp
#include <lumora/LumoraAPI.hpp>

// 1. Implement the data provider interface
class MyTrainer : public lumora::gui::IDataProvider {
    TrainingState getTrainingState() override {
        // Return your current training state
        return m_state;
    }
    
    ModelGraph getModelGraph() override {
        // Return your model structure
        return m_graph;
    }
    
    // ... implement other methods
};

// 2. Implement the command handler interface
class MyController : public lumora::gui::ICommandHandler {
    void pause() override { m_trainer.pause(); }
    void resume() override { m_trainer.resume(); }
    void setHyperparameter(const std::string& name, 
                           HyperparamValue value, 
                           bool immediate) override {
        // Apply hyperparameter change
    }
    // ... implement other methods
};

// 3. Create and run the GUI
int main(int argc, char* argv[]) {
    MyTrainer trainer;
    MyController controller;
    
    lumora::gui::LumoraGUI gui(&trainer, &controller);
    return gui.exec(argc, argv);
}
```

## Architecture

```
Lumora_gui/
├── core/
│   ├── Types.hpp          # Data structures (TrainingState, ModelGraph, etc.)
│   └── LumoraAPI.hpp      # IDataProvider, ICommandHandler interfaces
├── theme/
│   └── NeonPalette.hpp    # Colors, fonts, styling
├── widgets/
│   ├── PulseDashboard.hpp # Metrics and charts
│   ├── XRayGraph.hpp      # Model topology view
│   ├── CortexInspector.hpp# Layer analysis
│   ├── HyperparameterValve.hpp
│   └── Observer.hpp       # Ambient presence
├── panels/
│   └── MainWindow.hpp     # Main application window
└── src/
    ├── main.cpp           # Demo application
    ├── LumoraGUI.cpp      # LumoraGUI implementation
    └── MockProvider.cpp   # Mock data for testing
```

## Design Philosophy

1. **Backend Agnostic**: Works with any ML framework (PyTorch, TensorFlow, custom)
2. **Non-Invasive**: Polling-based data retrieval, won't slow your training
3. **Information Dense**: Every pixel conveys useful information
4. **Neon Dark Theme**: Easy on the eyes during long training sessions
5. **Modular**: Use only the panels you need

## API Reference

### IDataProvider

The interface your backend must implement to provide data to the GUI:

| Method | Frequency | Purpose |
|--------|-----------|---------|
| `getTrainingState()` | ~60Hz | Current loss, step, LR, metrics |
| `getModelGraph()` | On change | Model structure |
| `getLayerStats()` | On demand | Detailed layer analysis |
| `getAnomalies()` | ~10Hz | NaN/Inf/gradient issues |
| `getHyperparameters()` | On demand | Adjustable parameters |

### ICommandHandler

The interface for receiving user commands from the GUI:

| Method | Purpose |
|--------|---------|
| `pause()` / `resume()` | Training control |
| `setHyperparameter()` | Live parameter updates |
| `executeScript()` | Lua/Python scripting |
| `setLayerFrozen()` | Freeze layer weights |
| `saveCheckpoint()` | Save model state |

## Customization

### Adding Custom Widgets

```cpp
class MyWidget : public QWidget {
    Q_OBJECT
public:
    MyWidget(IDataProvider* provider, QWidget* parent = nullptr);
    // ...
};

// Add to MainWindow::setupDocks()
QDockWidget* myDock = new QDockWidget("My Widget", this);
myDock->setWidget(new MyWidget(m_provider, this));
addDockWidget(Qt::LeftDockWidgetArea, myDock);
```

### Custom Theme Colors

Edit `theme/NeonPalette.hpp`:
```cpp
constexpr QColor NEON_CYAN {0x00, 0xE5, 0xFF};  // Change to your preference
```

## License

MIT License - See [LICENSE](LICENSE) for details.

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](docs/CONTRIBUTING.md) first.

---

*Built with ❤️ for the ML community*
