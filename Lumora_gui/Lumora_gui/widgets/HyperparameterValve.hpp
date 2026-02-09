#pragma once
/**
 * @file HyperparameterValve.hpp
 * @brief Live hyperparameter adjustment panel
 * 
 * Real-time sliders and controls for tuning training.
 * Features "blast radius" indicator showing impact of changes.
 * 
 * Layout:
 * ┌─────────────────────────────────────────────────────┐
 * │  Hyperparameters                    [Commit] [Undo] │
 * ├─────────────────────────────────────────────────────┤
 * │  Learning Rate          ░░░░░█░░░░░░░░   0.001     │
 * │  │ Blast Radius: ████████████████████████ HIGH     │
 * ├─────────────────────────────────────────────────────┤
 * │  Weight Decay           ░░░█░░░░░░░░░░   1e-4      │
 * │  │ Blast Radius: █████░░░░░░░░░░░░░░░░░ LOW       │
 * ├─────────────────────────────────────────────────────┤
 * │  Dropout                ░░░░░░░█░░░░░░   0.3       │
 * │  │ Blast Radius: ███████████░░░░░░░░░░░ MEDIUM    │
 * ├─────────────────────────────────────────────────────┤
 * │  ┌─────────────────────────────────────────────┐   │
 * │  │ >>> script console (Lua)                    │   │
 * │  │ set_lr(0.0001)                              │   │
 * │  └─────────────────────────────────────────────┘   │
 * └─────────────────────────────────────────────────────┘
 */

#include "../core/Types.hpp"
#include "../core/LumoraAPI.hpp"
#include "../theme/NeonPalette.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <cmath>
#include <unordered_map>

namespace lumora::gui::widgets {

// ============================================================================
// Blast Radius Indicator
// ============================================================================

/**
 * @brief Visual indicator of hyperparameter impact
 */
class BlastRadiusBar : public QWidget {
    Q_OBJECT
    
public:
    explicit BlastRadiusBar(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_value(0.0f)
    {
        setFixedHeight(12);
        setMinimumWidth(100);
    }
    
    void setValue(float value) {
        m_value = std::clamp(value, 0.0f, 1.0f);
        update();
    }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        int barHeight = 8;
        int barY = (h - barHeight) / 2;
        
        // Background track
        p.fillRect(0, barY, w, barHeight, theme::colors::SLATE);
        
        // Filled portion with gradient
        int fillWidth = int(w * m_value);
        if (fillWidth > 0) {
            QLinearGradient grad(0, 0, w, 0);
            grad.setColorAt(0.0, theme::colors::NEON_LIME);
            grad.setColorAt(0.5, theme::colors::NEON_YELLOW);
            grad.setColorAt(1.0, theme::colors::NEON_RED);
            
            p.fillRect(0, barY, fillWidth, barHeight, grad);
        }
        
        // Label
        QString label;
        QColor labelColor;
        if (m_value < 0.33f) {
            label = "LOW";
            labelColor = theme::colors::NEON_LIME;
        } else if (m_value < 0.66f) {
            label = "MEDIUM";
            labelColor = theme::colors::NEON_YELLOW;
        } else {
            label = "HIGH";
            labelColor = theme::colors::NEON_RED;
        }
        
        p.setPen(labelColor);
        p.setFont(theme::fonts::monoSmall());
        p.drawText(w + 8, barY + barHeight, label);
    }
    
    QSize sizeHint() const override {
        return QSize(150, 12);
    }
    
private:
    float m_value;
};

// ============================================================================
// Hyperparameter Slider
// ============================================================================

/**
 * @brief Single hyperparameter control with slider and value display
 */
class HyperparamSlider : public QWidget {
    Q_OBJECT
    
public:
    explicit HyperparamSlider(const Hyperparameter& param, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_param(param)
        , m_isModified(false)
    {
        setupUI();
        setValue(param.value);
    }
    
    void setValue(const HyperparamValue& value) {
        if (std::holds_alternative<double>(value)) {
            double v = std::get<double>(value);
            m_currentValue = v;
            
            // Convert to slider position (0-1000)
            double min = std::get<double>(m_param.minValue);
            double max = std::get<double>(m_param.maxValue);
            
            double normalized;
            if (m_param.logScale) {
                // Log scale
                normalized = (std::log(v) - std::log(min)) / (std::log(max) - std::log(min));
            } else {
                normalized = (v - min) / (max - min);
            }
            
            m_slider->blockSignals(true);
            m_slider->setValue(int(normalized * 1000));
            m_slider->blockSignals(false);
            
            // Update display
            updateValueDisplay(v);
        }
    }
    
    void setModified(bool modified) {
        m_isModified = modified;
        updateStyle();
    }
    
    void setBlastRadius(float radius) {
        m_blastBar->setValue(radius);
    }
    
    const Hyperparameter& param() const { return m_param; }
    HyperparamValue currentValue() const { return m_currentValue; }
    
signals:
    void valueChanged(const QString& name, HyperparamValue value);
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(theme::spacing::SMALL);
        layout->setContentsMargins(0, theme::spacing::SMALL, 0, theme::spacing::SMALL);
        
        // Main row: label, slider, value
        QHBoxLayout* mainRow = new QHBoxLayout();
        
        m_nameLabel = new QLabel(QString::fromStdString(m_param.name), this);
        m_nameLabel->setMinimumWidth(120);
        m_nameLabel->setStyleSheet(QString("color: %1;").arg(theme::colors::FROST.name()));
        
        m_slider = new QSlider(Qt::Horizontal, this);
        m_slider->setRange(0, 1000);
        m_slider->setStyleSheet(theme::styles::slider());
        
        m_valueEdit = new QLineEdit(this);
        m_valueEdit->setFixedWidth(80);
        m_valueEdit->setAlignment(Qt::AlignRight);
        m_valueEdit->setStyleSheet(theme::styles::input());
        
        connect(m_slider, &QSlider::valueChanged, this, &HyperparamSlider::onSliderChanged);
        connect(m_valueEdit, &QLineEdit::editingFinished, this, &HyperparamSlider::onValueEdited);
        
        mainRow->addWidget(m_nameLabel);
        mainRow->addWidget(m_slider, 1);
        mainRow->addWidget(m_valueEdit);
        layout->addLayout(mainRow);
        
        // Blast radius row
        QHBoxLayout* blastRow = new QHBoxLayout();
        blastRow->setContentsMargins(theme::spacing::LARGE, 0, 0, 0);
        
        QLabel* blastLabel = new QLabel("Blast Radius:", this);
        blastLabel->setStyleSheet(QString("color: %1; font-size: 10px;")
            .arg(theme::colors::STEEL.name()));
        
        m_blastBar = new BlastRadiusBar(this);
        m_blastBar->setValue(m_param.blastRadius);
        
        blastRow->addWidget(blastLabel);
        blastRow->addWidget(m_blastBar, 1);
        blastRow->addSpacing(120);  // Align with value edit
        layout->addLayout(blastRow);
        
        setLayout(layout);
    }
    
    void updateValueDisplay(double v) {
        QString str;
        if (std::abs(v) < 0.001 || std::abs(v) > 1000) {
            str = QString::number(v, 'e', 2);
        } else {
            str = QString::number(v, 'f', 4);
        }
        m_valueEdit->setText(str);
    }
    
    void updateStyle() {
        if (m_isModified) {
            m_nameLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
                .arg(theme::colors::NEON_CYAN.name()));
        } else {
            m_nameLabel->setStyleSheet(QString("color: %1;")
                .arg(theme::colors::FROST.name()));
        }
    }
    
private slots:
    void onSliderChanged(int pos) {
        double normalized = pos / 1000.0;
        double min = std::get<double>(m_param.minValue);
        double max = std::get<double>(m_param.maxValue);
        
        double value;
        if (m_param.logScale) {
            value = std::exp(std::log(min) + normalized * (std::log(max) - std::log(min)));
        } else {
            value = min + normalized * (max - min);
        }
        
        m_currentValue = value;
        updateValueDisplay(value);
        setModified(true);
        
        emit valueChanged(QString::fromStdString(m_param.name), value);
    }
    
    void onValueEdited() {
        bool ok;
        double value = m_valueEdit->text().toDouble(&ok);
        if (ok) {
            m_currentValue = value;
            setValue(value);
            setModified(true);
            emit valueChanged(QString::fromStdString(m_param.name), value);
        }
    }
    
private:
    Hyperparameter m_param;
    HyperparamValue m_currentValue;
    bool m_isModified;
    
    QLabel* m_nameLabel;
    QSlider* m_slider;
    QLineEdit* m_valueEdit;
    BlastRadiusBar* m_blastBar;
};

// ============================================================================
// Script Console
// ============================================================================

/**
 * @brief Lua/Python script console for advanced control
 */
class ScriptConsole : public QWidget {
    Q_OBJECT
    
public:
    explicit ScriptConsole(ICommandHandler* handler, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_handler(handler)
    {
        setupUI();
    }
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(theme::spacing::SMALL);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Header
        QHBoxLayout* header = new QHBoxLayout();
        QLabel* promptLabel = new QLabel(">>> Script Console (Lua)", this);
        promptLabel->setStyleSheet(QString("color: %1; font-family: %2;")
            .arg(theme::colors::NEON_CYAN.name())
            .arg(theme::fonts::MONO_FAMILY));
        
        m_langSelector = new QPushButton("Lua ▼", this);
        m_langSelector->setFixedWidth(60);
        m_langSelector->setStyleSheet(theme::styles::buttonSecondary());
        
        header->addWidget(promptLabel);
        header->addStretch();
        header->addWidget(m_langSelector);
        layout->addLayout(header);
        
        // Output area
        m_output = new QTextEdit(this);
        m_output->setReadOnly(true);
        m_output->setMaximumHeight(80);
        m_output->setStyleSheet(QString(
            "QTextEdit { background-color: %1; color: %2; font-family: %3; font-size: 11px; "
            "border: 1px solid %4; border-radius: 4px; }")
            .arg(theme::colors::VOID_BLACK.name())
            .arg(theme::colors::NEON_LIME.name())
            .arg(theme::fonts::MONO_FAMILY)
            .arg(theme::colors::SLATE.name()));
        layout->addWidget(m_output);
        
        // Input line
        QHBoxLayout* inputRow = new QHBoxLayout();
        
        QLabel* chevron = new QLabel("❯", this);
        chevron->setStyleSheet(QString("color: %1;").arg(theme::colors::NEON_CYAN.name()));
        
        m_input = new QLineEdit(this);
        m_input->setPlaceholderText("Enter command...");
        m_input->setStyleSheet(theme::styles::input());
        connect(m_input, &QLineEdit::returnPressed, this, &ScriptConsole::executeCommand);
        
        m_runButton = new QPushButton("Run", this);
        m_runButton->setFixedWidth(50);
        m_runButton->setStyleSheet(theme::styles::buttonPrimary());
        connect(m_runButton, &QPushButton::clicked, this, &ScriptConsole::executeCommand);
        
        inputRow->addWidget(chevron);
        inputRow->addWidget(m_input, 1);
        inputRow->addWidget(m_runButton);
        layout->addLayout(inputRow);
        
        setLayout(layout);
    }
    
private slots:
    void executeCommand() {
        QString cmd = m_input->text().trimmed();
        if (cmd.isEmpty()) return;
        
        m_input->clear();
        m_output->append(QString("<span style='color:%1'>❯ %2</span>")
            .arg(theme::colors::SILVER.name())
            .arg(cmd));
        
        if (m_handler) {
            auto future = m_handler->executeScript(cmd.toStdString(), m_language);
            // In a real implementation, we'd wait for the future asynchronously
            // For now, just show that it was sent
            m_output->append(QString("<span style='color:%1'>Executing...</span>")
                .arg(theme::colors::NEON_YELLOW.name()));
        }
    }
    
private:
    ICommandHandler* m_handler;
    QString m_language = "lua";
    
    QPushButton* m_langSelector;
    QTextEdit* m_output;
    QLineEdit* m_input;
    QPushButton* m_runButton;
};

// ============================================================================
// Hyperparameter Valve Widget
// ============================================================================

/**
 * @brief Main Hyperparameter Valve panel
 */
class HyperparameterValve : public QWidget {
    Q_OBJECT
    
public:
    explicit HyperparameterValve(IDataProvider* provider, 
                                  ICommandHandler* handler,
                                  QWidget* parent = nullptr)
        : QWidget(parent)
        , m_provider(provider)
        , m_handler(handler)
        , m_hasChanges(false)
    {
        setupUI();
        loadHyperparameters();
    }
    
    void setProvider(IDataProvider* provider, ICommandHandler* handler) {
        m_provider = provider;
        m_handler = handler;
        loadHyperparameters();
    }
    
public slots:
    void loadHyperparameters() {
        if (!m_provider) return;
        
        // Clear existing sliders
        QLayoutItem* item;
        while ((item = m_slidersLayout->takeAt(0))) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        m_sliders.clear();
        
        // Load hyperparameters
        auto params = m_provider->getHyperparameters();
        
        for (const auto& param : params) {
            auto* slider = new HyperparamSlider(param, this);
            connect(slider, &HyperparamSlider::valueChanged, 
                    this, &HyperparameterValve::onValueChanged);
            m_slidersLayout->addWidget(slider);
            m_sliders[param.name] = slider;
        }
        
        m_slidersLayout->addStretch();
    }
    
private slots:
    void onValueChanged(const QString& name, HyperparamValue value) {
        m_pendingChanges[name.toStdString()] = value;
        m_hasChanges = true;
        updateButtonStates();
        
        // Optionally apply immediately (for some params)
        // m_handler->setHyperparameter(name.toStdString(), value, true);
    }
    
    void onCommit() {
        if (!m_handler) return;
        
        for (const auto& [name, value] : m_pendingChanges) {
            m_handler->setHyperparameter(name, value, true);
        }
        m_handler->commitHyperparameters();
        
        // Reset modified states
        for (auto& [name, slider] : m_sliders) {
            slider->setModified(false);
        }
        m_pendingChanges.clear();
        m_hasChanges = false;
        updateButtonStates();
    }
    
    void onRevert() {
        if (!m_handler) return;
        
        m_handler->revertHyperparameters();
        loadHyperparameters();
        
        m_pendingChanges.clear();
        m_hasChanges = false;
        updateButtonStates();
    }
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(theme::spacing::NORMAL);
        layout->setContentsMargins(theme::spacing::NORMAL, theme::spacing::NORMAL,
                                    theme::spacing::NORMAL, theme::spacing::NORMAL);
        
        // Header
        QHBoxLayout* header = new QHBoxLayout();
        
        QLabel* titleLabel = new QLabel("Hyperparameters", this);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;")
            .arg(theme::colors::FROST.name()));
        
        m_commitButton = new QPushButton("Commit", this);
        m_revertButton = new QPushButton("Undo", this);
        
        m_commitButton->setStyleSheet(theme::styles::buttonPrimary());
        m_revertButton->setStyleSheet(theme::styles::buttonSecondary());
        m_commitButton->setEnabled(false);
        m_revertButton->setEnabled(false);
        
        connect(m_commitButton, &QPushButton::clicked, this, &HyperparameterValve::onCommit);
        connect(m_revertButton, &QPushButton::clicked, this, &HyperparameterValve::onRevert);
        
        header->addWidget(titleLabel);
        header->addStretch();
        header->addWidget(m_commitButton);
        header->addWidget(m_revertButton);
        layout->addLayout(header);
        
        // Sliders in scroll area
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        
        QWidget* slidersWidget = new QWidget();
        m_slidersLayout = new QVBoxLayout(slidersWidget);
        m_slidersLayout->setSpacing(theme::spacing::SMALL);
        m_slidersLayout->setContentsMargins(0, 0, 0, 0);
        
        scrollArea->setWidget(slidersWidget);
        layout->addWidget(scrollArea, 1);
        
        // Script console
        m_console = new ScriptConsole(m_handler, this);
        layout->addWidget(m_console);
        
        setLayout(layout);
    }
    
    void updateButtonStates() {
        m_commitButton->setEnabled(m_hasChanges);
        m_revertButton->setEnabled(m_hasChanges);
    }
    
    IDataProvider* m_provider;
    ICommandHandler* m_handler;
    
    QVBoxLayout* m_slidersLayout;
    std::unordered_map<std::string, HyperparamSlider*> m_sliders;
    std::unordered_map<std::string, HyperparamValue> m_pendingChanges;
    bool m_hasChanges;
    
    QPushButton* m_commitButton;
    QPushButton* m_revertButton;
    ScriptConsole* m_console;
};

} // namespace lumora::gui::widgets
