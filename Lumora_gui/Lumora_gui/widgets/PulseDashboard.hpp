#pragma once
/**
 * @file PulseDashboard.hpp
 * @brief Real-time training metrics dashboard
 * 
 * The "heartbeat" of the training session.
 * Shows loss curves, accuracy, learning rate, and status ticker.
 * 
 * Layout:
 * ┌─────────────────────────────────────────────┐
 * │  ● TRAINING                   Step: 12,345  │  <- Status bar
 * ├─────────────────────────────────────────────┤
 * │                                             │
 * │        📉 Loss Curve (scrolling)            │  <- Primary chart
 * │                                             │
 * ├─────────────────────────────────────────────┤
 * │  LR: 0.001  │ Acc: 94.2% │ ETA: 2h 15m     │  <- Metric cards
 * ├─────────────────────────────────────────────┤
 * │  [gradient ok] [lr scheduled] [checkpoint]  │  <- Status ticker
 * └─────────────────────────────────────────────┘
 */

#include "../core/Types.hpp"
#include "../core/LumoraAPI.hpp"
#include "../theme/NeonPalette.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <deque>
#include <mutex>
#include <memory>

namespace lumora::gui::widgets {

// ============================================================================
// Sparkline Chart
// ============================================================================

/**
 * @brief Minimal real-time line chart
 * 
 * Shows recent values as a scrolling sparkline.
 * Optimized for smooth 60fps updates.
 */
class SparklineChart : public QWidget {
    Q_OBJECT
    
public:
    explicit SparklineChart(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_maxPoints(200)
        , m_minValue(0.0f)
        , m_maxValue(1.0f)
        , m_autoScale(true)
        , m_lineColor(theme::colors::NEON_CYAN)
        , m_fillGradient(true)
    {
        setMinimumHeight(80);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setStyleSheet(QString("background-color: %1; border-radius: 4px;")
            .arg(theme::colors::CARBON.name()));
    }
    
    void addValue(float value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values.push_back(value);
        if (m_values.size() > m_maxPoints) {
            m_values.pop_front();
        }
        
        if (m_autoScale) {
            m_minValue = *std::min_element(m_values.begin(), m_values.end());
            m_maxValue = *std::max_element(m_values.begin(), m_values.end());
            float margin = (m_maxValue - m_minValue) * 0.1f;
            m_minValue -= margin;
            m_maxValue += margin;
            if (m_maxValue <= m_minValue) {
                m_maxValue = m_minValue + 1.0f;
            }
        }
        
        update();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values.clear();
        update();
    }
    
    void setColor(const QColor& color) { 
        m_lineColor = color; 
        update();
    }
    
    void setAutoScale(bool autoScale) { m_autoScale = autoScale; }
    void setRange(float min, float max) { 
        m_minValue = min; 
        m_maxValue = max;
        m_autoScale = false;
    }
    
    void setTitle(const QString& title) { m_title = title; update(); }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        int padding = 8;
        int chartTop = padding + (m_title.isEmpty() ? 0 : 20);
        int chartHeight = h - chartTop - padding;
        
        // Title
        if (!m_title.isEmpty()) {
            p.setPen(theme::colors::SILVER);
            p.setFont(theme::fonts::sansNormal());
            p.drawText(padding, padding + 14, m_title);
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_values.size() < 2) return;
        
        // Build path
        QPainterPath path;
        QPainterPath fillPath;
        
        float xStep = float(w - 2 * padding) / (m_maxPoints - 1);
        int startX = w - padding - (m_values.size() - 1) * xStep;
        
        for (size_t i = 0; i < m_values.size(); ++i) {
            float normalized = (m_values[i] - m_minValue) / (m_maxValue - m_minValue);
            float y = chartTop + chartHeight * (1.0f - normalized);
            float x = startX + i * xStep;
            
            if (i == 0) {
                path.moveTo(x, y);
                fillPath.moveTo(x, chartTop + chartHeight);
                fillPath.lineTo(x, y);
            } else {
                path.lineTo(x, y);
                fillPath.lineTo(x, y);
            }
        }
        
        // Close fill path
        fillPath.lineTo(startX + (m_values.size() - 1) * xStep, chartTop + chartHeight);
        fillPath.closeSubpath();
        
        // Draw fill gradient
        if (m_fillGradient) {
            QLinearGradient grad(0, chartTop, 0, chartTop + chartHeight);
            grad.setColorAt(0, theme::colors::withAlpha(m_lineColor, 60));
            grad.setColorAt(1, theme::colors::withAlpha(m_lineColor, 10));
            p.fillPath(fillPath, grad);
        }
        
        // Draw line
        QPen pen(m_lineColor, 2);
        p.setPen(pen);
        p.drawPath(path);
        
        // Current value dot
        if (!m_values.empty()) {
            float lastNorm = (m_values.back() - m_minValue) / (m_maxValue - m_minValue);
            float lastY = chartTop + chartHeight * (1.0f - lastNorm);
            float lastX = startX + (m_values.size() - 1) * xStep;
            
            p.setBrush(m_lineColor);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(lastX, lastY), 4, 4);
            
            // Value label
            p.setPen(theme::colors::FROST);
            p.setFont(theme::fonts::monoSmall());
            QString valStr = QString::number(m_values.back(), 'f', 4);
            p.drawText(lastX + 8, lastY + 4, valStr);
        }
        
        // Y-axis labels
        p.setPen(theme::colors::STEEL);
        p.setFont(theme::fonts::monoSmall());
        p.drawText(padding, chartTop + 10, QString::number(m_maxValue, 'f', 3));
        p.drawText(padding, chartTop + chartHeight, QString::number(m_minValue, 'f', 3));
    }
    
private:
    std::deque<float> m_values;
    std::mutex m_mutex;
    size_t m_maxPoints;
    float m_minValue, m_maxValue;
    bool m_autoScale;
    QColor m_lineColor;
    bool m_fillGradient;
    QString m_title;
};

// ============================================================================
// Metric Card
// ============================================================================

/**
 * @brief Small metric display with label and value
 */
class MetricCard : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor glowColor READ glowColor WRITE setGlowColor)
    
public:
    explicit MetricCard(const QString& label, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_label(label)
        , m_glowColor(theme::colors::SLATE)
    {
        setFixedHeight(60);
        setMinimumWidth(100);
        setStyleSheet(theme::styles::card());
    }
    
    void setValue(const QString& value) {
        m_value = value;
        update();
    }
    
    void setUnit(const QString& unit) { m_unit = unit; update(); }
    
    void setGlowColor(const QColor& color) { 
        m_glowColor = color; 
        update();
    }
    
    QColor glowColor() const { return m_glowColor; }
    
    void pulse(const QColor& color) {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowColor");
        anim->setDuration(theme::anim::PULSE);
        anim->setStartValue(color);
        anim->setEndValue(theme::colors::SLATE);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        
        // Background
        p.setPen(QPen(m_glowColor, 1));
        p.setBrush(theme::colors::GRAPHITE);
        p.drawRoundedRect(1, 1, w - 2, h - 2, 
                          theme::spacing::BORDER_RADIUS_SMALL,
                          theme::spacing::BORDER_RADIUS_SMALL);
        
        // Label
        p.setPen(theme::colors::SILVER);
        p.setFont(theme::fonts::sansNormal());
        p.drawText(8, 18, m_label);
        
        // Value
        p.setPen(theme::colors::FROST);
        p.setFont(theme::fonts::sansBold());
        QString displayVal = m_value + (m_unit.isEmpty() ? "" : " " + m_unit);
        p.drawText(8, 44, displayVal);
    }
    
private:
    QString m_label;
    QString m_value;
    QString m_unit;
    QColor m_glowColor;
};

// ============================================================================
// Status Ticker
// ============================================================================

/**
 * @brief Scrolling status message ticker
 */
class StatusTicker : public QWidget {
    Q_OBJECT
    
public:
    explicit StatusTicker(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_scrollOffset(0)
    {
        setFixedHeight(28);
        setStyleSheet(QString("background-color: %1; border-radius: 4px;")
            .arg(theme::colors::CARBON.name()));
        
        m_scrollTimer = new QTimer(this);
        connect(m_scrollTimer, &QTimer::timeout, this, [this]() {
            m_scrollOffset += 1;
            update();
        });
    }
    
    void addMessage(const QString& msg, const QColor& color = theme::colors::FROST) {
        m_messages.push_back({msg, color});
        if (m_messages.size() > 20) {
            m_messages.pop_front();
        }
        
        if (!m_scrollTimer->isActive()) {
            m_scrollTimer->start(50);
        }
        update();
    }
    
    void clear() {
        m_messages.clear();
        m_scrollTimer->stop();
        m_scrollOffset = 0;
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(theme::fonts::monoSmall());
        
        int x = width() - m_scrollOffset;
        int y = height() / 2 + 4;
        
        for (const auto& msg : m_messages) {
            p.setPen(msg.color);
            QRect bounds = p.fontMetrics().boundingRect(msg.text);
            p.drawText(x, y, "[" + msg.text + "]");
            x += bounds.width() + 40;  // Gap between messages
        }
        
        // Reset scroll when all messages are off-screen
        if (!m_messages.empty() && x < 0) {
            m_scrollOffset = 0;
        }
    }
    
private:
    struct Message {
        QString text;
        QColor color;
    };
    
    std::deque<Message> m_messages;
    QTimer* m_scrollTimer;
    int m_scrollOffset;
};

// ============================================================================
// Training Status Indicator
// ============================================================================

/**
 * @brief Pulsing dot with state label
 */
class TrainingStatusIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float pulsePhase READ pulsePhase WRITE setPulsePhase)
    
public:
    explicit TrainingStatusIndicator(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_state(SystemState::Idle)
        , m_pulsePhase(0.0f)
    {
        setFixedSize(150, 30);
        
        m_pulseTimer = new QTimer(this);
        connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
            m_pulsePhase += 0.1f;
            if (m_pulsePhase > 6.28f) m_pulsePhase = 0;
            update();
        });
    }
    
    void setState(SystemState state) {
        m_state = state;
        
        if (state == SystemState::Training) {
            m_pulseTimer->start(50);
        } else {
            m_pulseTimer->stop();
        }
        update();
    }
    
    float pulsePhase() const { return m_pulsePhase; }
    void setPulsePhase(float phase) { m_pulsePhase = phase; update(); }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        QColor dotColor;
        QString stateText;
        
        switch (m_state) {
            case SystemState::Idle:
                dotColor = theme::colors::STEEL;
                stateText = "IDLE";
                break;
            case SystemState::Training:
                dotColor = theme::colors::TRAINING_ACTIVE;
                stateText = "TRAINING";
                break;
            case SystemState::Paused:
                dotColor = theme::colors::TRAINING_PAUSED;
                stateText = "PAUSED";
                break;
            case SystemState::Error:
                dotColor = theme::colors::TRAINING_ERROR;
                stateText = "ERROR";
                break;
            case SystemState::Finished:
                dotColor = theme::colors::NEON_LIME;
                stateText = "FINISHED";
                break;
        }
        
        // Pulsing glow for training state
        float glow = 0;
        if (m_state == SystemState::Training) {
            glow = (1.0f + std::sin(m_pulsePhase)) * 0.5f;
        }
        
        // Draw glow
        if (glow > 0) {
            QRadialGradient grad(15, 15, 15 + glow * 10);
            grad.setColorAt(0, theme::colors::withAlpha(dotColor, int(100 * glow)));
            grad.setColorAt(1, Qt::transparent);
            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawEllipse(5, 5, 20, 20);
        }
        
        // Draw dot
        p.setBrush(dotColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(10, 10, 10, 10);
        
        // Draw text
        p.setPen(dotColor);
        p.setFont(theme::fonts::sansBold());
        p.drawText(30, 19, stateText);
    }
    
private:
    SystemState m_state;
    float m_pulsePhase;
    QTimer* m_pulseTimer;
};

// ============================================================================
// Pulse Dashboard Widget
// ============================================================================

/**
 * @brief Main Pulse Dashboard panel
 * 
 * Aggregates all training metrics into a single view.
 */
class PulseDashboard : public QWidget {
    Q_OBJECT
    
public:
    explicit PulseDashboard(IDataProvider* provider, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_provider(provider)
    {
        setupUI();
        
        // Update timer
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &PulseDashboard::refresh);
        m_updateTimer->start(33);  // ~30 Hz for smooth updates
    }
    
    void setProvider(IDataProvider* provider) {
        m_provider = provider;
    }
    
public slots:
    void refresh() {
        if (!m_provider) return;
        
        TrainingState state = m_provider->getTrainingState();
        
        // Update status indicator
        m_statusIndicator->setState(state.systemState);
        
        // Update step counter
        m_stepLabel->setText(QString("Step: %1").arg(state.step));
        
        // Update charts
        m_lossChart->addValue(state.loss);
        if (state.metrics.count("accuracy")) {
            m_accChart->addValue(state.metrics.at("accuracy"));
        }
        
        // Update metric cards
        m_lrCard->setValue(QString::number(state.learningRate, 'e', 2));
        m_lossCard->setValue(QString::number(state.loss, 'f', 4));
        
        if (state.metrics.count("accuracy")) {
            m_accCard->setValue(QString::number(state.metrics.at("accuracy") * 100, 'f', 1) + "%");
        }
        
        // ETA
        if (state.samplesPerSecond > 0 && state.totalSamples > 0) {
            uint64_t remaining = state.totalSamples - state.samplesProcessed;
            uint64_t seconds = remaining / state.samplesPerSecond;
            int hours = seconds / 3600;
            int minutes = (seconds % 3600) / 60;
            m_etaCard->setValue(QString("%1h %2m").arg(hours).arg(minutes, 2, 10, QChar('0')));
        }
        
        // Speed
        m_speedCard->setValue(QString::number(state.samplesPerSecond, 'f', 1) + "/s");
    }
    
    void addTickerMessage(const QString& msg, const QColor& color = theme::colors::FROST) {
        m_ticker->addMessage(msg, color);
    }
    
private:
    void setupUI() {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(theme::spacing::NORMAL);
        mainLayout->setContentsMargins(theme::spacing::NORMAL, theme::spacing::NORMAL,
                                        theme::spacing::NORMAL, theme::spacing::NORMAL);
        
        // Status bar
        QHBoxLayout* statusBar = new QHBoxLayout();
        m_statusIndicator = new TrainingStatusIndicator(this);
        m_stepLabel = new QLabel("Step: 0", this);
        m_stepLabel->setStyleSheet(QString("color: %1; font-family: %2;")
            .arg(theme::colors::FROST.name())
            .arg(theme::fonts::MONO_FAMILY));
        statusBar->addWidget(m_statusIndicator);
        statusBar->addStretch();
        statusBar->addWidget(m_stepLabel);
        mainLayout->addLayout(statusBar);
        
        // Loss chart
        m_lossChart = new SparklineChart(this);
        m_lossChart->setTitle("Loss");
        m_lossChart->setColor(theme::colors::NEON_CYAN);
        m_lossChart->setMinimumHeight(120);
        mainLayout->addWidget(m_lossChart, 2);
        
        // Accuracy chart
        m_accChart = new SparklineChart(this);
        m_accChart->setTitle("Accuracy");
        m_accChart->setColor(theme::colors::NEON_LIME);
        m_accChart->setRange(0, 1);
        m_accChart->setMinimumHeight(80);
        mainLayout->addWidget(m_accChart, 1);
        
        // Metric cards
        QHBoxLayout* cardsLayout = new QHBoxLayout();
        cardsLayout->setSpacing(theme::spacing::NORMAL);
        
        m_lrCard = new MetricCard("Learning Rate", this);
        m_lossCard = new MetricCard("Loss", this);
        m_accCard = new MetricCard("Accuracy", this);
        m_etaCard = new MetricCard("ETA", this);
        m_speedCard = new MetricCard("Speed", this);
        
        cardsLayout->addWidget(m_lrCard);
        cardsLayout->addWidget(m_lossCard);
        cardsLayout->addWidget(m_accCard);
        cardsLayout->addWidget(m_etaCard);
        cardsLayout->addWidget(m_speedCard);
        mainLayout->addLayout(cardsLayout);
        
        // Status ticker
        m_ticker = new StatusTicker(this);
        mainLayout->addWidget(m_ticker);
        
        setLayout(mainLayout);
    }
    
    IDataProvider* m_provider;
    QTimer* m_updateTimer;
    
    TrainingStatusIndicator* m_statusIndicator;
    QLabel* m_stepLabel;
    SparklineChart* m_lossChart;
    SparklineChart* m_accChart;
    MetricCard* m_lrCard;
    MetricCard* m_lossCard;
    MetricCard* m_accCard;
    MetricCard* m_etaCard;
    MetricCard* m_speedCard;
    StatusTicker* m_ticker;
};

} // namespace lumora::gui::widgets
