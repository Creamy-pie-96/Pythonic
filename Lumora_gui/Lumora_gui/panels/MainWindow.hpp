#pragma once
/**
 * @file MainWindow.hpp
 * @brief Lumora GUI main window
 * 
 * Assembles all panels into a cohesive IDE-like interface.
 * Multi-pane layout with dockable widgets.
 * 
 * Default Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  [File] [View] [Training] [Tools] [Help]          [Observer] ◉ │
 * ├────────────────────────┬────────────────────────────────────────┤
 * │                        │                                        │
 * │   X-Ray Model Graph    │        Cortex Inspector               │
 * │                        │                                        │
 * │        (center)        │          (right dock)                  │
 * │                        │                                        │
 * ├────────────────────────┴────────────────────────────────────────┤
 * │                                                                 │
 * │                      Pulse Dashboard                            │
 * │                        (bottom)                                 │
 * │                                                                 │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  Hyperparameter Valve                                           │
 * │                        (bottom dock)                            │
 * └─────────────────────────────────────────────────────────────────┘
 */

#include "../core/LumoraAPI.hpp"
#include "../theme/NeonPalette.hpp"
#include "PulseDashboard.hpp"
#include "XRayGraph.hpp"
#include "CortexInspector.hpp"
#include "HyperparameterValve.hpp"
#include "Observer.hpp"

#include <QMainWindow>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>

namespace lumora::gui {

/**
 * @brief Main application window
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(IDataProvider* provider, 
                        ICommandHandler* handler,
                        QWidget* parent = nullptr)
        : QMainWindow(parent)
        , m_provider(provider)
        , m_handler(handler)
    {
        setWindowTitle("Lumora - Neural Network IDE");
        setMinimumSize(1280, 720);
        resize(1600, 900);
        
        // Apply global stylesheet
        setStyleSheet(theme::styles::applicationStyle());
        
        setupMenuBar();
        setupWidgets();
        setupDocks();
        setupStatusBar();
        setupObserver();
        
        // Connect signals
        connectSignals();
        
        // Start update timer
        m_stateTimer = new QTimer(this);
        connect(m_stateTimer, &QTimer::timeout, this, &MainWindow::updateState);
        m_stateTimer->start(100);  // 10 Hz state updates
    }
    
    ~MainWindow() override = default;
    
    // Widget accessors
    widgets::PulseDashboard* pulseDashboard() { return m_pulseDashboard; }
    widgets::XRayGraph* xrayGraph() { return m_xrayGraph; }
    widgets::CortexInspector* cortexInspector() { return m_cortexInspector; }
    widgets::HyperparameterValve* hyperparameterValve() { return m_hyperparameterValve; }
    widgets::Observer* observer() { return m_observer; }
    
public slots:
    void onPause() {
        if (m_handler) {
            m_handler->pause();
        }
    }
    
    void onResume() {
        if (m_handler) {
            m_handler->resume();
        }
    }
    
    void onStop() {
        if (m_handler) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Stop Training",
                "Are you sure you want to stop training?",
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                m_handler->stop();
            }
        }
    }
    
    void onSaveCheckpoint() {
        // TODO: Show file dialog
        if (m_handler) {
            m_handler->saveCheckpoint("checkpoint.pt");
            m_pulseDashboard->addTickerMessage("Checkpoint saved", theme::colors::NEON_LIME);
        }
    }
    
private:
    void setupMenuBar() {
        QMenuBar* menuBar = new QMenuBar(this);
        
        // File menu
        QMenu* fileMenu = menuBar->addMenu("&File");
        
        QAction* saveAction = fileMenu->addAction("Save Checkpoint");
        saveAction->setShortcut(QKeySequence::Save);
        connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveCheckpoint);
        
        QAction* loadAction = fileMenu->addAction("Load Checkpoint");
        loadAction->setShortcut(QKeySequence::Open);
        
        fileMenu->addSeparator();
        
        QAction* exitAction = fileMenu->addAction("Exit");
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
        
        // View menu
        QMenu* viewMenu = menuBar->addMenu("&View");
        
        m_showPulseAction = viewMenu->addAction("Pulse Dashboard");
        m_showPulseAction->setCheckable(true);
        m_showPulseAction->setChecked(true);
        
        m_showGraphAction = viewMenu->addAction("X-Ray Graph");
        m_showGraphAction->setCheckable(true);
        m_showGraphAction->setChecked(true);
        
        m_showCortexAction = viewMenu->addAction("Cortex Inspector");
        m_showCortexAction->setCheckable(true);
        m_showCortexAction->setChecked(true);
        
        m_showValveAction = viewMenu->addAction("Hyperparameter Valve");
        m_showValveAction->setCheckable(true);
        m_showValveAction->setChecked(true);
        
        viewMenu->addSeparator();
        
        m_showObserverAction = viewMenu->addAction("Observer");
        m_showObserverAction->setCheckable(true);
        m_showObserverAction->setChecked(true);
        connect(m_showObserverAction, &QAction::toggled, this, [this](bool checked) {
            m_observer->setVisible(checked);
        });
        
        // Training menu
        QMenu* trainingMenu = menuBar->addMenu("&Training");
        
        QAction* pauseAction = trainingMenu->addAction("Pause");
        pauseAction->setShortcut(Qt::Key_Space);
        connect(pauseAction, &QAction::triggered, this, &MainWindow::onPause);
        
        QAction* resumeAction = trainingMenu->addAction("Resume");
        connect(resumeAction, &QAction::triggered, this, &MainWindow::onResume);
        
        QAction* stopAction = trainingMenu->addAction("Stop");
        stopAction->setShortcut(Qt::CTRL | Qt::Key_Q);
        connect(stopAction, &QAction::triggered, this, &MainWindow::onStop);
        
        // Tools menu
        QMenu* toolsMenu = menuBar->addMenu("&Tools");
        
        QAction* snapshotAction = toolsMenu->addAction("Request Snapshot");
        connect(snapshotAction, &QAction::triggered, this, [this]() {
            if (m_handler) {
                m_handler->requestSnapshot();
                m_pulseDashboard->addTickerMessage("Snapshot requested", theme::colors::NEON_CYAN);
            }
        });
        
        // Help menu
        QMenu* helpMenu = menuBar->addMenu("&Help");
        
        QAction* aboutAction = helpMenu->addAction("About Lumora");
        connect(aboutAction, &QAction::triggered, this, [this]() {
            QMessageBox::about(this, "About Lumora",
                "<h2>Lumora</h2>"
                "<p>Neural Network IDE for Deep Learning Introspection</p>"
                "<p>Version 0.1.0</p>");
        });
        
        setMenuBar(menuBar);
    }
    
    void setupWidgets() {
        // Create all widgets
        m_pulseDashboard = new widgets::PulseDashboard(m_provider, this);
        m_xrayGraph = new widgets::XRayGraph(m_provider, this);
        m_cortexInspector = new widgets::CortexInspector(m_provider, m_handler, this);
        m_hyperparameterValve = new widgets::HyperparameterValve(m_provider, m_handler, this);
    }
    
    void setupDocks() {
        // Central widget: XRay Graph + Pulse Dashboard in splitter
        QSplitter* centralSplitter = new QSplitter(Qt::Vertical, this);
        centralSplitter->addWidget(m_xrayGraph);
        centralSplitter->addWidget(m_pulseDashboard);
        centralSplitter->setSizes({600, 300});
        setCentralWidget(centralSplitter);
        
        // Right dock: Cortex Inspector
        QDockWidget* cortexDock = new QDockWidget("Cortex Inspector", this);
        cortexDock->setWidget(m_cortexInspector);
        cortexDock->setMinimumWidth(300);
        cortexDock->setStyleSheet(QString("QDockWidget { color: %1; }")
            .arg(theme::colors::FROST.name()));
        addDockWidget(Qt::RightDockWidgetArea, cortexDock);
        
        // Connect view action
        connect(m_showCortexAction, &QAction::toggled, cortexDock, &QDockWidget::setVisible);
        connect(cortexDock, &QDockWidget::visibilityChanged, m_showCortexAction, &QAction::setChecked);
        
        // Bottom dock: Hyperparameter Valve
        QDockWidget* valveDock = new QDockWidget("Hyperparameter Valve", this);
        valveDock->setWidget(m_hyperparameterValve);
        valveDock->setMinimumHeight(200);
        valveDock->setStyleSheet(QString("QDockWidget { color: %1; }")
            .arg(theme::colors::FROST.name()));
        addDockWidget(Qt::BottomDockWidgetArea, valveDock);
        
        // Connect view action
        connect(m_showValveAction, &QAction::toggled, valveDock, &QDockWidget::setVisible);
        connect(valveDock, &QDockWidget::visibilityChanged, m_showValveAction, &QAction::setChecked);
    }
    
    void setupStatusBar() {
        QStatusBar* status = statusBar();
        status->setStyleSheet(QString("QStatusBar { background: %1; color: %2; }")
            .arg(theme::colors::CARBON.name())
            .arg(theme::colors::SILVER.name()));
        
        m_statusLabel = new QLabel("Ready", this);
        m_stepLabel = new QLabel("Step: 0", this);
        m_memoryLabel = new QLabel("Memory: --", this);
        
        status->addWidget(m_statusLabel);
        status->addPermanentWidget(m_memoryLabel);
        status->addPermanentWidget(m_stepLabel);
    }
    
    void setupObserver() {
        // Place observer in top-right corner of menu bar
        m_observer = new widgets::Observer(this);
        
        QWidget* menuBarCorner = new QWidget(this);
        QHBoxLayout* cornerLayout = new QHBoxLayout(menuBarCorner);
        cornerLayout->setContentsMargins(0, 0, 8, 0);
        cornerLayout->addWidget(m_observer);
        menuBar()->setCornerWidget(menuBarCorner, Qt::TopRightCorner);
    }
    
    void connectSignals() {
        // Connect graph selection to inspector
        connect(m_xrayGraph, &widgets::XRayGraph::nodeSelected,
                m_cortexInspector, &widgets::CortexInspector::inspectLayer);
    }
    
    void updateState() {
        if (!m_provider) return;
        
        TrainingState state = m_provider->getTrainingState();
        
        // Update observer
        m_observer->setState(state.systemState);
        
        // Update status bar
        QString statusText;
        switch (state.systemState) {
            case SystemState::Idle:
                statusText = "Idle";
                break;
            case SystemState::Training:
                statusText = "Training...";
                break;
            case SystemState::Paused:
                statusText = "Paused";
                break;
            case SystemState::Error:
                statusText = "Error!";
                m_observer->triggerEmotionPulse(1.0f, 500);
                break;
            case SystemState::Finished:
                statusText = "Training Complete";
                break;
        }
        m_statusLabel->setText(statusText);
        m_stepLabel->setText(QString("Step: %1").arg(state.step));
        
        // Check for anomalies
        auto anomalies = m_provider->getAnomalies(m_lastAnomalyStep);
        for (const auto& anomaly : anomalies) {
            QString msg;
            QColor color;
            switch (anomaly.type) {
                case AnomalyType::NaN:
                    msg = "NaN detected!";
                    color = theme::colors::NEON_RED;
                    break;
                case AnomalyType::Inf:
                    msg = "Infinity detected!";
                    color = theme::colors::NEON_RED;
                    break;
                case AnomalyType::ExplodingGradient:
                    msg = "Exploding gradient!";
                    color = theme::colors::NEON_ORANGE;
                    break;
                case AnomalyType::VanishingGradient:
                    msg = "Vanishing gradient";
                    color = theme::colors::NEON_ORANGE;
                    break;
                case AnomalyType::DeadNeuron:
                    msg = "Dead neurons detected";
                    color = theme::colors::NEON_YELLOW;
                    break;
                case AnomalyType::LossDivergence:
                    msg = "Loss diverging!";
                    color = theme::colors::NEON_RED;
                    break;
            }
            m_pulseDashboard->addTickerMessage(msg, color);
            m_observer->triggerEmotionPulse(0.8f, 1000);
            m_lastAnomalyStep = anomaly.step;
        }
    }
    
private:
    IDataProvider* m_provider;
    ICommandHandler* m_handler;
    
    // Widgets
    widgets::PulseDashboard* m_pulseDashboard;
    widgets::XRayGraph* m_xrayGraph;
    widgets::CortexInspector* m_cortexInspector;
    widgets::HyperparameterValve* m_hyperparameterValve;
    widgets::Observer* m_observer;
    
    // Menu actions
    QAction* m_showPulseAction;
    QAction* m_showGraphAction;
    QAction* m_showCortexAction;
    QAction* m_showValveAction;
    QAction* m_showObserverAction;
    
    // Status bar labels
    QLabel* m_statusLabel;
    QLabel* m_stepLabel;
    QLabel* m_memoryLabel;
    
    // Timers
    QTimer* m_stateTimer;
    
    // State tracking
    uint64_t m_lastAnomalyStep = 0;
};

} // namespace lumora::gui
