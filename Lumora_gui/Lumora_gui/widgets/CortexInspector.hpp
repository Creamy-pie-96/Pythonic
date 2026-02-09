#pragma once
/**
 * @file CortexInspector.hpp
 * @brief Deep layer inspection and profiling panel
 * 
 * Detailed view for selected layer analysis.
 * Shows activation histograms, weight distributions, gradient health.
 * 
 * Layout:
 * ┌─────────────────────────────────────────────────┐
 * │  Conv2d_1                        [📌 Pin]      │
 * │  Shape: [64, 128, 3, 3]   Params: 73,728       │
 * ├─────────────────────────────────────────────────┤
 * │  ╭─────────────╮  ╭─────────────╮              │
 * │  │  Weights    │  │  Gradients  │              │
 * │  │  Histogram  │  │  Histogram  │              │
 * │  ╰─────────────╯  ╰─────────────╯              │
 * ├─────────────────────────────────────────────────┤
 * │  Health Indicators:                            │
 * │  ● Gradient Norm: 0.0023  ✓                    │
 * │  ● Dead Neurons: 2.3%     ⚠                    │
 * │  ● Weight Saturation: OK  ✓                    │
 * ├─────────────────────────────────────────────────┤
 * │  [Freeze] [Reset] [View Tensors]               │
 * └─────────────────────────────────────────────────┘
 */

#include "../core/Types.hpp"
#include "../core/LumoraAPI.hpp"
#include "../theme/NeonPalette.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace lumora::gui::widgets {

// ============================================================================
// Histogram Widget
// ============================================================================

/**
 * @brief Distribution histogram visualization
 */
class HistogramWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit HistogramWidget(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_title(title)
        , m_numBins(50)
    {
        setMinimumSize(150, 100);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_bins.resize(m_numBins, 0);
    }
    
    void setData(const std::vector<float>& values) {
        if (values.empty()) return;
        
        // Find range
        m_min = *std::min_element(values.begin(), values.end());
        m_max = *std::max_element(values.begin(), values.end());
        
        // Handle zero range
        if (m_max <= m_min) {
            m_max = m_min + 1.0f;
        }
        
        // Bin the data
        std::fill(m_bins.begin(), m_bins.end(), 0);
        float binWidth = (m_max - m_min) / m_numBins;
        
        for (float v : values) {
            int bin = std::min(int((v - m_min) / binWidth), m_numBins - 1);
            m_bins[bin]++;
        }
        
        m_maxCount = *std::max_element(m_bins.begin(), m_bins.end());
        
        // Statistics
        m_mean = 0;
        m_std = 0;
        for (float v : values) m_mean += v;
        m_mean /= values.size();
        for (float v : values) m_std += (v - m_mean) * (v - m_mean);
        m_std = std::sqrt(m_std / values.size());
        
        update();
    }
    
    void setColor(const QColor& color) {
        m_color = color;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        int padding = 8;
        int titleHeight = 20;
        int statsHeight = 16;
        int chartLeft = padding;
        int chartTop = padding + titleHeight;
        int chartWidth = w - 2 * padding;
        int chartHeight = h - padding - titleHeight - statsHeight - padding;
        
        // Background
        p.fillRect(rect(), theme::colors::CARBON);
        
        // Title
        p.setPen(theme::colors::SILVER);
        p.setFont(theme::fonts::sansNormal());
        p.drawText(padding, padding + 14, m_title);
        
        if (m_maxCount == 0) {
            p.setPen(theme::colors::STEEL);
            p.drawText(rect(), Qt::AlignCenter, "No data");
            return;
        }
        
        // Draw bars
        float barWidth = float(chartWidth) / m_numBins;
        
        for (int i = 0; i < m_numBins; ++i) {
            float barHeight = float(m_bins[i]) / m_maxCount * chartHeight;
            float x = chartLeft + i * barWidth;
            float y = chartTop + chartHeight - barHeight;
            
            // Gradient fill
            QLinearGradient grad(x, y, x, chartTop + chartHeight);
            grad.setColorAt(0, m_color);
            grad.setColorAt(1, theme::colors::withAlpha(m_color, 80));
            
            p.fillRect(QRectF(x + 1, y, barWidth - 2, barHeight), grad);
        }
        
        // Zero line if applicable
        if (m_min < 0 && m_max > 0) {
            float zeroX = chartLeft + (-m_min) / (m_max - m_min) * chartWidth;
            p.setPen(QPen(theme::colors::FROST, 1, Qt::DashLine));
            p.drawLine(zeroX, chartTop, zeroX, chartTop + chartHeight);
        }
        
        // Statistics
        p.setPen(theme::colors::SILVER);
        p.setFont(theme::fonts::monoSmall());
        QString stats = QString("μ=%1  σ=%2  [%3, %4]")
            .arg(m_mean, 0, 'e', 2)
            .arg(m_std, 0, 'e', 2)
            .arg(m_min, 0, 'e', 2)
            .arg(m_max, 0, 'e', 2);
        p.drawText(padding, h - padding, stats);
    }
    
private:
    QString m_title;
    std::vector<int> m_bins;
    int m_numBins;
    int m_maxCount = 0;
    float m_min = 0, m_max = 1;
    float m_mean = 0, m_std = 0;
    QColor m_color = theme::colors::NEON_CYAN;
};

// ============================================================================
// Health Indicator
// ============================================================================

/**
 * @brief Single health metric with status icon
 */
class HealthIndicator : public QWidget {
    Q_OBJECT
    
public:
    enum Status { Good, Warning, Bad };
    
    explicit HealthIndicator(const QString& label, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_label(label)
        , m_status(Good)
    {
        setFixedHeight(24);
    }
    
    void setValue(const QString& value, Status status = Good) {
        m_value = value;
        m_status = status;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        // Status dot
        QColor dotColor;
        QString icon;
        switch (m_status) {
            case Good:
                dotColor = theme::colors::NEON_LIME;
                icon = "✓";
                break;
            case Warning:
                dotColor = theme::colors::NEON_ORANGE;
                icon = "⚠";
                break;
            case Bad:
                dotColor = theme::colors::NEON_RED;
                icon = "✗";
                break;
        }
        
        p.setBrush(dotColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 8, 8, 8);
        
        // Label
        p.setPen(theme::colors::SILVER);
        p.setFont(theme::fonts::sansNormal());
        p.drawText(20, 16, m_label + ":");
        
        // Value
        p.setPen(theme::colors::FROST);
        p.drawText(150, 16, m_value);
        
        // Status icon
        p.setPen(dotColor);
        p.drawText(width() - 20, 16, icon);
    }
    
private:
    QString m_label;
    QString m_value;
    Status m_status;
};

// ============================================================================
// Cortex Inspector Widget
// ============================================================================

/**
 * @brief Main Cortex Inspector panel
 */
class CortexInspector : public QWidget {
    Q_OBJECT
    
public:
    explicit CortexInspector(IDataProvider* provider, 
                             ICommandHandler* handler,
                             QWidget* parent = nullptr)
        : QWidget(parent)
        , m_provider(provider)
        , m_handler(handler)
        , m_currentLayerId(INVALID_LAYER)
    {
        setupUI();
        
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &CortexInspector::refresh);
    }
    
    void setProvider(IDataProvider* provider, ICommandHandler* handler) {
        m_provider = provider;
        m_handler = handler;
    }
    
public slots:
    void inspectLayer(LayerId layerId) {
        m_currentLayerId = layerId;
        
        if (layerId == INVALID_LAYER) {
            showPlaceholder();
            m_updateTimer->stop();
            return;
        }
        
        m_updateTimer->start(500);  // Refresh every 500ms
        refresh();
    }
    
    void refresh() {
        if (!m_provider || m_currentLayerId == INVALID_LAYER) return;
        
        LayerStats stats = m_provider->getLayerStats(m_currentLayerId, 10);
        
        // Update header
        m_nameLabel->setText(QString::fromStdString(stats.name));
        m_shapeLabel->setText(QString("Shape: %1   Params: %2")
            .arg(formatShape(stats.outputShape))
            .arg(formatNumber(stats.numParams)));
        
        // Update histograms
        m_weightsHist->setData(stats.weightHist.bins);
        m_gradsHist->setData(stats.gradHist.bins);
        
        // Update health indicators
        updateHealthIndicators(stats);
        
        // Show panel content
        m_placeholder->hide();
        m_contentWidget->show();
    }
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(theme::spacing::NORMAL);
        layout->setContentsMargins(theme::spacing::NORMAL, theme::spacing::NORMAL,
                                    theme::spacing::NORMAL, theme::spacing::NORMAL);
        
        // Placeholder (shown when no layer selected)
        m_placeholder = new QLabel("Select a layer to inspect", this);
        m_placeholder->setAlignment(Qt::AlignCenter);
        m_placeholder->setStyleSheet(QString("color: %1;").arg(theme::colors::STEEL.name()));
        layout->addWidget(m_placeholder);
        
        // Content widget (shown when layer selected)
        m_contentWidget = new QWidget(this);
        QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        
        // Header
        QWidget* headerWidget = new QWidget();
        QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        
        m_nameLabel = new QLabel("Layer Name", this);
        m_nameLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 16px;")
            .arg(theme::colors::FROST.name()));
        
        m_pinButton = new QPushButton("📌 Pin", this);
        m_pinButton->setFixedWidth(60);
        m_pinButton->setStyleSheet(theme::styles::buttonSecondary());
        
        headerLayout->addWidget(m_nameLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(m_pinButton);
        contentLayout->addWidget(headerWidget);
        
        m_shapeLabel = new QLabel("Shape: []   Params: 0", this);
        m_shapeLabel->setStyleSheet(QString("color: %1; font-family: %2;")
            .arg(theme::colors::SILVER.name())
            .arg(theme::fonts::MONO_FAMILY));
        contentLayout->addWidget(m_shapeLabel);
        
        // Histograms
        QHBoxLayout* histLayout = new QHBoxLayout();
        m_weightsHist = new HistogramWidget("Weights", this);
        m_weightsHist->setColor(theme::colors::NEON_CYAN);
        m_gradsHist = new HistogramWidget("Gradients", this);
        m_gradsHist->setColor(theme::colors::NEON_MAGENTA);
        histLayout->addWidget(m_weightsHist);
        histLayout->addWidget(m_gradsHist);
        contentLayout->addLayout(histLayout);
        
        // Health indicators
        QLabel* healthLabel = new QLabel("Health Indicators:", this);
        healthLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
            .arg(theme::colors::SILVER.name()));
        contentLayout->addWidget(healthLabel);
        
        m_gradNormIndicator = new HealthIndicator("Gradient Norm", this);
        m_deadNeuronsIndicator = new HealthIndicator("Dead Neurons", this);
        m_saturationIndicator = new HealthIndicator("Weight Saturation", this);
        m_activationIndicator = new HealthIndicator("Activation Range", this);
        
        contentLayout->addWidget(m_gradNormIndicator);
        contentLayout->addWidget(m_deadNeuronsIndicator);
        contentLayout->addWidget(m_saturationIndicator);
        contentLayout->addWidget(m_activationIndicator);
        
        contentLayout->addStretch();
        
        // Action buttons
        QHBoxLayout* actionsLayout = new QHBoxLayout();
        
        m_freezeButton = new QPushButton("Freeze", this);
        m_resetButton = new QPushButton("Reset", this);
        m_viewTensorsButton = new QPushButton("View Tensors", this);
        
        m_freezeButton->setStyleSheet(theme::styles::buttonSecondary());
        m_resetButton->setStyleSheet(theme::styles::buttonSecondary());
        m_viewTensorsButton->setStyleSheet(theme::styles::buttonPrimary());
        
        connect(m_freezeButton, &QPushButton::clicked, this, &CortexInspector::onFreezeClicked);
        
        actionsLayout->addWidget(m_freezeButton);
        actionsLayout->addWidget(m_resetButton);
        actionsLayout->addStretch();
        actionsLayout->addWidget(m_viewTensorsButton);
        contentLayout->addLayout(actionsLayout);
        
        m_contentWidget->setLayout(contentLayout);
        m_contentWidget->hide();
        layout->addWidget(m_contentWidget);
        
        setLayout(layout);
    }
    
    void showPlaceholder() {
        m_contentWidget->hide();
        m_placeholder->show();
    }
    
    void updateHealthIndicators(const LayerStats& stats) {
        // Gradient norm
        float gradNorm = stats.gradientNorm;
        HealthIndicator::Status gradStatus = HealthIndicator::Good;
        if (std::isnan(gradNorm) || std::isinf(gradNorm)) {
            gradStatus = HealthIndicator::Bad;
        } else if (gradNorm < 1e-7 || gradNorm > 100) {
            gradStatus = HealthIndicator::Warning;
        }
        m_gradNormIndicator->setValue(QString::number(gradNorm, 'e', 4), gradStatus);
        
        // Dead neurons
        float deadPct = stats.deadNeuronsPct * 100;
        HealthIndicator::Status deadStatus = HealthIndicator::Good;
        if (deadPct > 50) {
            deadStatus = HealthIndicator::Bad;
        } else if (deadPct > 10) {
            deadStatus = HealthIndicator::Warning;
        }
        m_deadNeuronsIndicator->setValue(QString::number(deadPct, 'f', 1) + "%", deadStatus);
        
        // Weight saturation
        HealthIndicator::Status satStatus = HealthIndicator::Good;
        if (stats.weightHist.max > 10 || stats.weightHist.min < -10) {
            satStatus = HealthIndicator::Warning;
        }
        m_saturationIndicator->setValue("OK", satStatus);
        
        // Activation range
        HealthIndicator::Status actStatus = HealthIndicator::Good;
        if (stats.activationMean != stats.activationMean) {  // NaN check
            actStatus = HealthIndicator::Bad;
        }
        m_activationIndicator->setValue(
            QString("[%1, %2]")
                .arg(stats.activationHist.min, 0, 'f', 2)
                .arg(stats.activationHist.max, 0, 'f', 2),
            actStatus);
    }
    
    static QString formatShape(const TensorShape& shape) {
        if (shape.empty()) return "?";
        QString s = "[";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) s += ", ";
            s += QString::number(shape[i]);
        }
        s += "]";
        return s;
    }
    
    static QString formatNumber(uint64_t n) {
        if (n >= 1000000) {
            return QString("%1M").arg(n / 1000000.0, 0, 'f', 1);
        } else if (n >= 1000) {
            return QString("%1K").arg(n / 1000.0, 0, 'f', 1);
        }
        return QString::number(n);
    }
    
private slots:
    void onFreezeClicked() {
        if (!m_handler || m_currentLayerId == INVALID_LAYER) return;
        
        // Toggle freeze state
        m_isFrozen = !m_isFrozen;
        m_handler->setLayerFrozen(m_currentLayerId, m_isFrozen);
        m_freezeButton->setText(m_isFrozen ? "Unfreeze" : "Freeze");
    }
    
private:
    IDataProvider* m_provider;
    ICommandHandler* m_handler;
    LayerId m_currentLayerId;
    bool m_isFrozen = false;
    
    QTimer* m_updateTimer;
    
    QLabel* m_placeholder;
    QWidget* m_contentWidget;
    
    QLabel* m_nameLabel;
    QLabel* m_shapeLabel;
    QPushButton* m_pinButton;
    
    HistogramWidget* m_weightsHist;
    HistogramWidget* m_gradsHist;
    
    HealthIndicator* m_gradNormIndicator;
    HealthIndicator* m_deadNeuronsIndicator;
    HealthIndicator* m_saturationIndicator;
    HealthIndicator* m_activationIndicator;
    
    QPushButton* m_freezeButton;
    QPushButton* m_resetButton;
    QPushButton* m_viewTensorsButton;
};

} // namespace lumora::gui::widgets
