/**
 * @file MockProvider.cpp
 * @brief Mock data provider for testing
 */

#include "core/LumoraAPI.hpp"
#include <random>
#include <chrono>
#include <cmath>

namespace lumora::gui {

// ============================================================================
// MockDataProvider::Impl
// ============================================================================

struct MockDataProvider::Impl {
    std::mt19937 rng;
    std::normal_distribution<float> normalDist{0.0f, 1.0f};
    std::uniform_real_distribution<float> uniformDist{0.0f, 1.0f};
    
    TrainingState state;
    ModelGraph graph;
    uint64_t modelVersion = 1;
    std::vector<AnomalyEvent> anomalies;
    std::vector<Hyperparameter> hyperparams;
    
    // Simulation state
    bool isTraining = false;
    float lossTarget = 0.01f;
    
    Impl() {
        rng.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        
        // Initialize training state
        state.systemState = SystemState::Idle;
        state.step = 0;
        state.epoch = 0;
        state.loss = 2.5f;
        state.learningRate = 0.001;
        state.samplesPerSecond = 0;
        state.samplesProcessed = 0;
        state.totalSamples = 60000;
        state.metrics["accuracy"] = 0.1f;
        
        // Initialize model graph
        initModelGraph();
        
        // Initialize hyperparameters
        initHyperparameters();
    }
    
    void initModelGraph() {
        // Create a simple CNN architecture
        std::vector<std::string> layerNames = {
            "Input", "Conv1", "ReLU1", "Pool1",
            "Conv2", "ReLU2", "Pool2",
            "Flatten", "FC1", "ReLU3", "FC2", "Softmax"
        };
        
        std::vector<LayerType> layerTypes = {
            LayerType::Input, LayerType::Conv, LayerType::Activation, LayerType::Pool,
            LayerType::Conv, LayerType::Activation, LayerType::Pool,
            LayerType::Reshape, LayerType::Linear, LayerType::Activation, 
            LayerType::Linear, LayerType::Activation
        };
        
        for (size_t i = 0; i < layerNames.size(); ++i) {
            GraphNode node;
            node.id = i;
            node.name = layerNames[i];
            node.type = layerTypes[i];
            node.outputShape = {64, 32, 32};  // Simplified
            node.numParams = (i % 3 == 1) ? 1000 * (i + 1) : 0;
            node.isFrozen = false;
            graph.nodes.push_back(node);
            
            // Add edge from previous layer
            if (i > 0) {
                GraphEdge edge;
                edge.source = i - 1;
                edge.target = i;
                graph.edges.push_back(edge);
            }
        }
    }
    
    void initHyperparameters() {
        Hyperparameter lr;
        lr.name = "Learning Rate";
        lr.value = 0.001;
        lr.minValue = 1e-6;
        lr.maxValue = 1.0;
        lr.logScale = true;
        lr.blastRadius = 0.9f;
        hyperparams.push_back(lr);
        
        Hyperparameter wd;
        wd.name = "Weight Decay";
        wd.value = 1e-4;
        wd.minValue = 0.0;
        wd.maxValue = 0.1;
        wd.logScale = true;
        wd.blastRadius = 0.3f;
        hyperparams.push_back(wd);
        
        Hyperparameter dropout;
        dropout.name = "Dropout";
        dropout.value = 0.3;
        dropout.minValue = 0.0;
        dropout.maxValue = 0.9;
        dropout.logScale = false;
        dropout.blastRadius = 0.5f;
        hyperparams.push_back(dropout);
        
        Hyperparameter momentum;
        momentum.name = "Momentum";
        momentum.value = 0.9;
        momentum.minValue = 0.0;
        momentum.maxValue = 0.999;
        momentum.logScale = false;
        momentum.blastRadius = 0.6f;
        hyperparams.push_back(momentum);
    }
    
    void simulateStep() {
        if (!isTraining) return;
        
        state.step++;
        state.samplesProcessed += 64;  // Batch size
        
        // Simulate loss decrease with noise
        float progress = float(state.step) / 10000.0f;
        float targetLoss = lossTarget + (2.5f - lossTarget) * std::exp(-progress * 3);
        state.loss = targetLoss + normalDist(rng) * 0.05f * targetLoss;
        state.loss = std::max(0.001f, state.loss);
        
        // Simulate accuracy increase
        float accuracy = 1.0f - state.loss * 0.3f + uniformDist(rng) * 0.02f;
        state.metrics["accuracy"] = std::clamp(accuracy, 0.0f, 0.99f);
        
        // Simulate speed
        state.samplesPerSecond = 800 + normalDist(rng) * 50;
        
        // Epoch
        if (state.samplesProcessed >= state.totalSamples) {
            state.epoch++;
            state.samplesProcessed = 0;
        }
        
        // Random anomaly generation (rare)
        if (uniformDist(rng) < 0.001f) {
            AnomalyEvent anomaly;
            anomaly.type = AnomalyType::VanishingGradient;
            anomaly.step = state.step;
            anomaly.layerId = 3;
            anomaly.severity = 0.6f;
            anomaly.message = "Gradient near zero in Conv2";
            anomalies.push_back(anomaly);
        }
    }
};

// ============================================================================
// MockDataProvider Implementation
// ============================================================================

MockDataProvider::MockDataProvider()
    : m_impl(std::make_unique<Impl>())
{
}

MockDataProvider::~MockDataProvider() = default;

void MockDataProvider::startTraining() {
    m_impl->isTraining = true;
    m_impl->state.systemState = SystemState::Training;
}

void MockDataProvider::pauseTraining() {
    m_impl->isTraining = false;
    m_impl->state.systemState = SystemState::Paused;
}

void MockDataProvider::simulateNaN() {
    AnomalyEvent anomaly;
    anomaly.type = AnomalyType::NaN;
    anomaly.step = m_impl->state.step;
    anomaly.layerId = 5;
    anomaly.severity = 1.0f;
    anomaly.message = "NaN detected in FC1 weights";
    m_impl->anomalies.push_back(anomaly);
    m_impl->state.systemState = SystemState::Error;
}

void MockDataProvider::simulateDivergence() {
    m_impl->state.loss = 999999.0f;
    
    AnomalyEvent anomaly;
    anomaly.type = AnomalyType::LossDivergence;
    anomaly.step = m_impl->state.step;
    anomaly.severity = 1.0f;
    anomaly.message = "Loss diverged to infinity";
    m_impl->anomalies.push_back(anomaly);
}

TrainingState MockDataProvider::getTrainingState() {
    m_impl->simulateStep();
    return m_impl->state;
}

ModelGraph MockDataProvider::getModelGraph() {
    return m_impl->graph;
}

uint64_t MockDataProvider::getModelVersion() {
    return m_impl->modelVersion;
}

LayerStats MockDataProvider::getLayerStats(LayerId layerId, size_t historySteps) {
    LayerStats stats;
    
    if (layerId < m_impl->graph.nodes.size()) {
        const auto& node = m_impl->graph.nodes[layerId];
        stats.layerId = layerId;
        stats.name = node.name;
        stats.outputShape = node.outputShape;
        stats.numParams = node.numParams;
    }
    
    // Generate fake histogram data
    stats.weightHist.bins.resize(50);
    stats.gradHist.bins.resize(50);
    stats.activationHist.bins.resize(50);
    
    for (int i = 0; i < 50; ++i) {
        float x = (i - 25) / 10.0f;
        stats.weightHist.bins[i] = std::exp(-x * x);
        stats.gradHist.bins[i] = std::exp(-x * x * 4);
        stats.activationHist.bins[i] = std::max(0.0f, std::exp(-x * x) - 0.2f);
    }
    
    stats.weightHist.min = -2.0f;
    stats.weightHist.max = 2.0f;
    stats.gradHist.min = -0.1f;
    stats.gradHist.max = 0.1f;
    stats.activationHist.min = 0.0f;
    stats.activationHist.max = 1.0f;
    
    stats.gradientNorm = 0.01f + m_impl->uniformDist(m_impl->rng) * 0.005f;
    stats.deadNeuronsPct = 0.02f + m_impl->uniformDist(m_impl->rng) * 0.03f;
    stats.activationMean = 0.5f;
    stats.activationStd = 0.2f;
    
    return stats;
}

TensorView MockDataProvider::getTensorView(TensorId tensorId, int downsample) {
    TensorView view;
    view.tensorId = tensorId;
    view.shape = {64, 64};
    view.dtype = "float32";
    
    // Generate some random data
    view.data.resize(64 * 64 * sizeof(float));
    float* ptr = reinterpret_cast<float*>(view.data.data());
    for (int i = 0; i < 64 * 64; ++i) {
        ptr[i] = m_impl->normalDist(m_impl->rng);
    }
    
    return view;
}

std::vector<AnomalyEvent> MockDataProvider::getAnomalies(uint64_t sinceStep) {
    std::vector<AnomalyEvent> result;
    for (const auto& a : m_impl->anomalies) {
        if (a.step > sinceStep) {
            result.push_back(a);
        }
    }
    return result;
}

std::vector<Hyperparameter> MockDataProvider::getHyperparameters() {
    return m_impl->hyperparams;
}

std::optional<TrainingState> MockDataProvider::getHistoricalState(uint64_t step) {
    // Not implemented for mock
    return std::nullopt;
}

std::pair<uint64_t, uint64_t> MockDataProvider::getHistoryRange() {
    return {0, m_impl->state.step};
}

// ============================================================================
// MockCommandHandler Implementation
// ============================================================================

void MockCommandHandler::pause() {
    m_log.push_back("pause()");
}

void MockCommandHandler::resume() {
    m_log.push_back("resume()");
}

void MockCommandHandler::stop() {
    m_log.push_back("stop()");
}

void MockCommandHandler::setHyperparameter(const std::string& name, 
                                            HyperparamValue value, 
                                            bool immediate) {
    m_log.push_back("setHyperparameter(" + name + ", ..., " + 
                    (immediate ? "true" : "false") + ")");
}

void MockCommandHandler::commitHyperparameters() {
    m_log.push_back("commitHyperparameters()");
}

void MockCommandHandler::revertHyperparameters() {
    m_log.push_back("revertHyperparameters()");
}

std::future<std::string> MockCommandHandler::executeScript(const std::string& script,
                                                            const std::string& language) {
    m_log.push_back("executeScript(" + script + ", " + language + ")");
    std::promise<std::string> p;
    p.set_value("Script executed: " + script);
    return p.get_future();
}

void MockCommandHandler::saveCheckpoint(const std::string& path) {
    m_log.push_back("saveCheckpoint(" + path + ")");
}

void MockCommandHandler::loadCheckpoint(const std::string& path) {
    m_log.push_back("loadCheckpoint(" + path + ")");
}

void MockCommandHandler::setLayerFrozen(LayerId layerId, bool frozen) {
    m_log.push_back("setLayerFrozen(" + std::to_string(layerId) + ", " +
                    (frozen ? "true" : "false") + ")");
}

void MockCommandHandler::addLayer(LayerId afterLayer,
                                   const std::string& blockType,
                                   const std::vector<Hyperparameter>& parameters) {
    m_log.push_back("addLayer(" + std::to_string(afterLayer) + ", " + blockType + ")");
}

void MockCommandHandler::removeLayer(LayerId layerId) {
    m_log.push_back("removeLayer(" + std::to_string(layerId) + ")");
}

void MockCommandHandler::requestSnapshot() {
    m_log.push_back("requestSnapshot()");
}

void MockCommandHandler::setAnomalyBreakpoint(AnomalyType type, bool enabled) {
    m_log.push_back("setAnomalyBreakpoint()");
}

const std::vector<std::string>& MockCommandHandler::getCommandLog() const {
    return m_log;
}

} // namespace lumora::gui
