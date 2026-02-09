#pragma once
/**
 * @file Types.hpp
 * @brief Core data types for Lumora GUI
 * 
 * Backend-agnostic representations of ML concepts.
 * These types are designed for fast serialization and minimal copying.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <variant>
#include <optional>
#include <chrono>

namespace lumora::gui {

// ============================================================================
// Identifiers
// ============================================================================

using LayerId = uint32_t;
using TensorId = uint32_t;
using NodeId = uint32_t;
using EdgeId = uint32_t;

constexpr LayerId INVALID_LAYER = UINT32_MAX;
constexpr TensorId INVALID_TENSOR = UINT32_MAX;

// ============================================================================
// System State
// ============================================================================

/**
 * @brief Overall system state for status indication
 */
enum class SystemState : uint8_t {
    Idle,           // Not training
    Training,       // Actively training
    Paused,         // Training paused
    Evaluating,     // Running evaluation/inference
    Diverging,      // Training appears unstable
    NaNDetected,    // NaN/Inf detected - training halted
    Error           // System error
};

/**
 * @brief Trend direction for metrics
 */
enum class Trend : uint8_t {
    Stable,
    Improving,
    Degrading,
    Noisy,
    Divergent
};

// ============================================================================
// Training State
// ============================================================================

/**
 * @brief Real-time training metrics
 * 
 * Updated every step for smooth visualization.
 * Size-optimized for cache efficiency.
 */
struct TrainingState {
    // Core state
    SystemState state = SystemState::Idle;
    Trend trendConfidence = Trend::Stable;
    
    // Progress
    uint64_t epoch = 0;
    uint64_t totalEpochs = 0;
    uint64_t step = 0;
    uint64_t stepsPerEpoch = 0;
    
    // Metrics
    double loss = 0.0;
    double accuracy = 0.0;
    double gradientNorm = 0.0;
    double learningRate = 0.0;
    
    // Performance
    double samplesPerSecond = 0.0;
    double gpuMemoryUsedGB = 0.0;
    double gpuMemoryTotalGB = 0.0;
    double gpuUtilization = 0.0;
    
    // Timing
    std::chrono::steady_clock::time_point timestamp;
    double etaSeconds = 0.0;
    
    // Status message (for ticker)
    std::string statusMessage;
};

// ============================================================================
// Tensor Types
// ============================================================================

/**
 * @brief Tensor data type
 */
enum class DType : uint8_t {
    Float16,
    Float32,
    Float64,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    Bool
};

/**
 * @brief Tensor shape descriptor
 */
struct TensorShape {
    std::vector<int64_t> dims;
    DType dtype = DType::Float32;
    
    int64_t numel() const {
        int64_t n = 1;
        for (auto d : dims) n *= d;
        return n;
    }
    
    std::string toString() const {
        std::string s = "[";
        for (size_t i = 0; i < dims.size(); ++i) {
            if (i > 0) s += ", ";
            s += std::to_string(dims[i]);
        }
        s += "]";
        return s;
    }
};

/**
 * @brief Lightweight view into tensor data (no ownership)
 */
struct TensorView {
    TensorId id;
    TensorShape shape;
    const void* data = nullptr;  // Points to backend memory
    size_t sizeBytes = 0;
    
    bool isValid() const { return data != nullptr; }
};

// ============================================================================
// Layer Statistics
// ============================================================================

constexpr size_t HISTOGRAM_BINS = 256;

/**
 * @brief Histogram data for distribution visualization
 */
struct Histogram {
    std::array<float, HISTOGRAM_BINS> bins{};
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float mean = 0.0f;
    float stddev = 0.0f;
    uint64_t sampleCount = 0;
};

/**
 * @brief Per-layer statistics for Cortex Inspector
 */
struct LayerStats {
    LayerId id = INVALID_LAYER;
    std::string name;
    std::string type;  // "Conv2d", "Linear", "Attention", etc.
    
    // Performance
    double executionTimeMs = 0.0;
    size_t memoryBytes = 0;
    size_t parameterCount = 0;
    size_t flops = 0;
    
    // Distributions
    Histogram weightDistribution;
    Histogram gradientDistribution;
    Histogram activationDistribution;
    
    // Delta from N steps ago (for change tracking)
    Histogram weightDelta;
    Histogram gradientDelta;
    
    // Health indicators
    float deadNeuronRatio = 0.0f;  // Neurons with ~0 activation
    float saturationRatio = 0.0f; // Neurons stuck at min/max
    bool hasNaN = false;
    bool hasInf = false;
    bool isBottleneck = false;
};

// ============================================================================
// Graph Representation
// ============================================================================

/**
 * @brief Visual state for graph nodes
 */
struct NodeVisualState {
    float activationIntensity = 0.0f;  // 0-1, for glow effect
    float gradientMagnitude = 0.0f;    // For coloring
    bool isSelected = false;
    bool isHighlighted = false;
    bool hasAnomaly = false;
    bool isFrozen = false;  // Parameters frozen
};

/**
 * @brief Graph node (layer representation)
 */
struct GraphNode {
    NodeId id;
    LayerId layerId;
    std::string name;
    std::string type;
    
    // Layout
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 50.0f;
    
    // Tensor contracts
    std::vector<TensorShape> inputs;
    std::vector<TensorShape> outputs;
    
    // Visual state
    NodeVisualState visual;
};

/**
 * @brief Graph edge (tensor flow)
 */
struct GraphEdge {
    EdgeId id;
    NodeId from;
    NodeId to;
    int fromPort = 0;  // Output port index
    int toPort = 0;    // Input port index
    
    TensorShape shape;
    float gradientNorm = 0.0f;
    bool isActive = true;
};

/**
 * @brief Complete model graph
 */
struct ModelGraph {
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    uint64_t version = 0;  // Increments on structure change
    
    std::optional<GraphNode*> findNode(NodeId id) {
        for (auto& n : nodes) {
            if (n.id == id) return &n;
        }
        return std::nullopt;
    }
};

// ============================================================================
// Anomaly Events
// ============================================================================

/**
 * @brief Type of detected anomaly
 */
enum class AnomalyType : uint8_t {
    NaN,
    Inf,
    ExplodingGradient,
    VanishingGradient,
    DeadNeurons,
    SaturatedActivations,
    LossSpike,
    Custom
};

/**
 * @brief Anomaly event for diagnostics
 */
struct AnomalyEvent {
    uint64_t step;
    std::chrono::steady_clock::time_point timestamp;
    AnomalyType type;
    LayerId sourceLayer;
    std::string message;
    std::vector<LayerId> propagationPath;  // How the anomaly spread
    
    // Suspected causes (heuristic)
    std::vector<std::string> suspectedCauses;
};

// ============================================================================
// Hyperparameters
// ============================================================================

using HyperparamValue = std::variant<double, int64_t, bool, std::string>;

/**
 * @brief Hyperparameter definition
 */
struct Hyperparameter {
    std::string name;
    std::string displayName;
    std::string description;
    HyperparamValue value;
    HyperparamValue minValue;
    HyperparamValue maxValue;
    HyperparamValue defaultValue;
    bool isLogarithmic = false;  // For sliders
    bool requiresRestart = false;
    
    // Blast radius - which layers/components affected
    std::vector<std::string> affectedComponents;
};

// ============================================================================
// Block Library (for drag-and-drop assembly)
// ============================================================================

/**
 * @brief Block template for Cortex Library
 */
struct BlockTemplate {
    std::string id;          // "conv2d", "attention", etc.
    std::string displayName;
    std::string category;    // "Convolution", "Attention", "Normalization"
    std::string iconPath;
    
    // Tensor contracts
    std::vector<TensorShape> inputShapes;   // With wildcards for dynamic dims
    std::vector<TensorShape> outputShapes;
    
    // Configurable parameters
    std::vector<Hyperparameter> parameters;
};

} // namespace lumora::gui
