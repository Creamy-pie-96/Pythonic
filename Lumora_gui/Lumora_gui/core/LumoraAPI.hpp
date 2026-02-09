#pragma once
/**
 * @file LumoraAPI.hpp
 * @brief Integration API for Lumora GUI
 * 
 * This is the ONLY interface your ML backend needs to implement.
 * The GUI is completely backend-agnostic - works with any framework.
 * 
 * Usage:
 *   1. Implement IDataProvider to expose your model state
 *   2. Implement ICommandHandler to receive GUI commands
 *   3. Create LumoraGUI with your implementations
 *   4. Either call exec() for standalone or update() in your loop
 */

#include "Types.hpp"
#include <memory>
#include <functional>
#include <optional>
#include <future>

namespace lumora::gui {

// Forward declarations
class MainWindow;

// ============================================================================
// Data Provider Interface
// ============================================================================

/**
 * @brief Interface for backend to provide data to GUI
 * 
 * Implement this interface to expose your ML backend's state.
 * All methods should be thread-safe and return quickly (<1ms).
 * 
 * The GUI polls these at ~60Hz, so cache expensive computations.
 */
class IDataProvider {
public:
    virtual ~IDataProvider() = default;
    
    // ========== Core Training State ==========
    
    /**
     * @brief Get current training state
     * 
     * Called every frame (~60Hz). Should be very fast.
     * Consider caching and returning cached values.
     */
    virtual TrainingState getTrainingState() = 0;
    
    // ========== Model Structure ==========
    
    /**
     * @brief Get the model graph structure
     * 
     * Called when version changes or on initial load.
     * Return cached if structure hasn't changed.
     */
    virtual ModelGraph getModelGraph() = 0;
    
    /**
     * @brief Check if model structure has changed
     * 
     * Returns a version number that increments on change.
     * GUI uses this to avoid unnecessary getModelGraph() calls.
     */
    virtual uint64_t getModelVersion() = 0;
    
    // ========== Layer Details (On-Demand) ==========
    
    /**
     * @brief Get detailed statistics for a specific layer
     * 
     * Called when user inspects a layer. Can be slower.
     * @param layerId Layer to inspect
     * @param historySteps How many steps back for delta view (0 = current only)
     */
    virtual LayerStats getLayerStats(LayerId layerId, size_t historySteps = 10) = 0;
    
    // ========== Tensor Data (Heavy, On-Demand) ==========
    
    /**
     * @brief Get a view into tensor data
     * 
     * Used for detailed weight/activation visualization.
     * Can be slow - GUI shows loading indicator.
     * 
     * @param tensorId Tensor to view
     * @param downsample Downsample factor (1 = full res, 4 = 1/4 res)
     */
    virtual TensorView getTensorView(TensorId tensorId, int downsample = 1) = 0;
    
    // ========== Anomaly Events ==========
    
    /**
     * @brief Get recent anomaly events
     * 
     * Return events since last call or last N events.
     * @param sinceStep Only return events after this step (0 = all recent)
     */
    virtual std::vector<AnomalyEvent> getAnomalies(uint64_t sinceStep = 0) = 0;
    
    // ========== Hyperparameters ==========
    
    /**
     * @brief Get list of adjustable hyperparameters
     */
    virtual std::vector<Hyperparameter> getHyperparameters() = 0;
    
    // ========== History (for Time Scrubbing) ==========
    
    /**
     * @brief Get historical training state at a specific step
     * 
     * Used for training replay. Return nullopt if step not available.
     */
    virtual std::optional<TrainingState> getHistoricalState(uint64_t step) = 0;
    
    /**
     * @brief Get range of available historical steps
     * @return {earliestStep, latestStep}
     */
    virtual std::pair<uint64_t, uint64_t> getHistoryRange() = 0;
};

// ============================================================================
// Command Handler Interface
// ============================================================================

/**
 * @brief Interface for GUI to send commands to backend
 * 
 * Implement this to handle user actions.
 * Commands should be queued and handled asynchronously.
 * Return immediately and let backend process when ready.
 */
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    
    // ========== Training Control ==========
    
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
    
    // ========== Hyperparameter Updates ==========
    
    /**
     * @brief Update a hyperparameter
     * 
     * Changes are staged until committed for safety.
     * @param name Parameter name
     * @param value New value
     * @param immediate Apply immediately (true) or stage (false)
     */
    virtual void setHyperparameter(const std::string& name, 
                                   HyperparamValue value,
                                   bool immediate = false) = 0;
    
    /**
     * @brief Commit all staged hyperparameter changes
     */
    virtual void commitHyperparameters() = 0;
    
    /**
     * @brief Revert staged changes
     */
    virtual void revertHyperparameters() = 0;
    
    // ========== Scripting ==========
    
    /**
     * @brief Execute a script (Lua/Python)
     * 
     * @param script Script source code
     * @param language "lua" or "python"
     * @return Future with result or error message
     */
    virtual std::future<std::string> executeScript(const std::string& script,
                                                   const std::string& language = "lua") = 0;
    
    // ========== Checkpointing ==========
    
    virtual void saveCheckpoint(const std::string& path) = 0;
    virtual void loadCheckpoint(const std::string& path) = 0;
    
    // ========== Model Modification (Advanced) ==========
    
    /**
     * @brief Freeze/unfreeze layer parameters
     */
    virtual void setLayerFrozen(LayerId layerId, bool frozen) = 0;
    
    /**
     * @brief Add a new layer to the graph
     * 
     * Used by Cortex Library drag-and-drop.
     * @param afterLayer Insert after this layer (INVALID_LAYER = at end)
     * @param blockType Block template ID
     * @param parameters Initial parameters
     */
    virtual void addLayer(LayerId afterLayer,
                          const std::string& blockType,
                          const std::vector<Hyperparameter>& parameters) = 0;
    
    /**
     * @brief Remove a layer from the graph
     */
    virtual void removeLayer(LayerId layerId) = 0;
    
    // ========== Debug ==========
    
    /**
     * @brief Request a gradient/activation snapshot at next step
     */
    virtual void requestSnapshot() = 0;
    
    /**
     * @brief Set a breakpoint on NaN/Inf detection
     */
    virtual void setAnomalyBreakpoint(AnomalyType type, bool enabled) = 0;
};

// ============================================================================
// Event Callbacks (Optional)
// ============================================================================

/**
 * @brief Callback for real-time events from backend
 * 
 * Optional push-based events instead of polling.
 * More efficient for sparse events.
 */
struct IEventCallback {
    virtual ~IEventCallback() = default;
    
    virtual void onTrainingStateChanged(const TrainingState& state) {}
    virtual void onAnomalyDetected(const AnomalyEvent& event) {}
    virtual void onModelStructureChanged(uint64_t newVersion) {}
    virtual void onLayerSelected(LayerId layerId) {}
};

// ============================================================================
// Main GUI Class
// ============================================================================

/**
 * @brief Main Lumora GUI application
 * 
 * Create with your IDataProvider and ICommandHandler implementations.
 * 
 * Two usage modes:
 * 1. Standalone: Call exec() which runs its own Qt event loop
 * 2. Embedded: Call update() from your existing event loop
 * 
 * Example (Standalone):
 * ```cpp
 * MyDataProvider provider;
 * MyCommandHandler handler;
 * 
 * LumoraGUI gui(&provider, &handler);
 * return gui.exec(argc, argv);
 * ```
 * 
 * Example (Embedded):
 * ```cpp
 * LumoraGUI gui(&provider, &handler);
 * gui.show();
 * 
 * while (training) {
 *     trainStep();
 *     gui.update();  // Call ~60Hz
 * }
 * ```
 */
class LumoraGUI {
public:
    /**
     * @brief Create GUI with backend interfaces
     * 
     * @param provider Data source (required)
     * @param handler Command receiver (required)
     * @param eventCallback Optional push-based event handler
     */
    LumoraGUI(IDataProvider* provider, 
              ICommandHandler* handler,
              IEventCallback* eventCallback = nullptr);
    
    ~LumoraGUI();
    
    // Prevent copying
    LumoraGUI(const LumoraGUI&) = delete;
    LumoraGUI& operator=(const LumoraGUI&) = delete;
    
    /**
     * @brief Show the main window
     */
    void show();
    
    /**
     * @brief Hide the main window
     */
    void hide();
    
    /**
     * @brief Check if window is visible
     */
    bool isVisible() const;
    
    /**
     * @brief Process pending events and update UI
     * 
     * Call this ~60Hz from your main loop.
     * Handles Qt events and refreshes displays.
     */
    void update();
    
    /**
     * @brief Run standalone with internal Qt event loop
     * 
     * This blocks until the window is closed.
     * @return Exit code (0 = success)
     */
    int exec(int argc, char* argv[]);
    
    /**
     * @brief Get the main window (for advanced customization)
     */
    MainWindow* getMainWindow();
    
    // ========== Configuration ==========
    
    /**
     * @brief Set refresh rate for real-time updates
     * @param hz Updates per second (1-120, default 60)
     */
    void setRefreshRate(int hz);
    
    /**
     * @brief Enable/disable the Observer (ambient presence)
     */
    void setObserverEnabled(bool enabled);
    
    /**
     * @brief Set dark/light theme
     */
    void setDarkMode(bool dark);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ============================================================================
// Utility: Mock Provider for Testing
// ============================================================================

/**
 * @brief Mock data provider for testing the GUI without a backend
 * 
 * Generates fake but realistic training data.
 */
class MockDataProvider : public IDataProvider {
public:
    MockDataProvider();
    ~MockDataProvider() override;
    
    // Simulation controls
    void startTraining();
    void pauseTraining();
    void simulateNaN();
    void simulateDivergence();
    
    // IDataProvider implementation
    TrainingState getTrainingState() override;
    ModelGraph getModelGraph() override;
    uint64_t getModelVersion() override;
    LayerStats getLayerStats(LayerId layerId, size_t historySteps) override;
    TensorView getTensorView(TensorId tensorId, int downsample) override;
    std::vector<AnomalyEvent> getAnomalies(uint64_t sinceStep) override;
    std::vector<Hyperparameter> getHyperparameters() override;
    std::optional<TrainingState> getHistoricalState(uint64_t step) override;
    std::pair<uint64_t, uint64_t> getHistoryRange() override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Mock command handler that logs commands
 */
class MockCommandHandler : public ICommandHandler {
public:
    void pause() override;
    void resume() override;
    void stop() override;
    void setHyperparameter(const std::string& name, HyperparamValue value, bool immediate) override;
    void commitHyperparameters() override;
    void revertHyperparameters() override;
    std::future<std::string> executeScript(const std::string& script, const std::string& language) override;
    void saveCheckpoint(const std::string& path) override;
    void loadCheckpoint(const std::string& path) override;
    void setLayerFrozen(LayerId layerId, bool frozen) override;
    void addLayer(LayerId afterLayer, const std::string& blockType, 
                  const std::vector<Hyperparameter>& parameters) override;
    void removeLayer(LayerId layerId) override;
    void requestSnapshot() override;
    void setAnomalyBreakpoint(AnomalyType type, bool enabled) override;
    
    // Access command log
    const std::vector<std::string>& getCommandLog() const;
    
private:
    std::vector<std::string> m_log;
};

} // namespace lumora::gui
