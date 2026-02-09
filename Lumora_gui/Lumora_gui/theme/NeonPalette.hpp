#pragma once
/**
 * @file NeonPalette.hpp
 * @brief Neon color palette for Lumora GUI
 * 
 * Dark mode with vibrant, high-contrast neon accents.
 * Optimized for dense information display and long sessions.
 */

#include <QColor>
#include <QFont>
#include <QString>
#include <array>

namespace lumora::gui::theme {

// ============================================================================
// Color Definitions
// ============================================================================

namespace colors {

// ---------- Base Colors (Dark Background) ----------
constexpr QColor VOID_BLACK      {0x0A, 0x0A, 0x0C};        // #0A0A0C - Deepest background
constexpr QColor SPACE_GREY      {0x14, 0x14, 0x18};        // #141418 - Main background
constexpr QColor CARBON          {0x1E, 0x1E, 0x24};        // #1E1E24 - Panel background
constexpr QColor GRAPHITE        {0x28, 0x28, 0x30};        // #282830 - Card background
constexpr QColor SLATE           {0x3A, 0x3A, 0x44};        // #3A3A44 - Border/divider
constexpr QColor STEEL           {0x5A, 0x5A, 0x68};        // #5A5A68 - Disabled
constexpr QColor SILVER          {0x8A, 0x8A, 0x98};        // #8A8A98 - Secondary text
constexpr QColor FROST           {0xC8, 0xC8, 0xD4};        // #C8C8D4 - Primary text
constexpr QColor PURE_WHITE      {0xF0, 0xF0, 0xF8};        // #F0F0F8 - Highlight text

// ---------- Neon Accent Colors ----------
constexpr QColor NEON_CYAN       {0x00, 0xE5, 0xFF};        // #00E5FF - Primary accent
constexpr QColor NEON_MAGENTA    {0xFF, 0x00, 0xD0};        // #FF00D0 - Secondary accent
constexpr QColor NEON_LIME       {0x00, 0xFF, 0x94};        // #00FF94 - Success/positive
constexpr QColor NEON_ORANGE     {0xFF, 0x8C, 0x00};        // #FF8C00 - Warning
constexpr QColor NEON_RED        {0xFF, 0x35, 0x55};        // #FF3555 - Error/danger
constexpr QColor NEON_PURPLE     {0xA855, 0xF7};            // #A855F7 - Gradient/alt
constexpr QColor NEON_BLUE       {0x38, 0xBD, 0xF8};        // #38BDF8 - Links/info
constexpr QColor NEON_YELLOW     {0xFA, 0xCC, 0x15};        // #FACC15 - Attention

// Note: Fix the NEON_PURPLE - it was malformed
constexpr QColor ELECTRIC_PURPLE {0xA8, 0x55, 0xF7};        // #A855F7 - Gradient/alt

// ---------- Semantic Colors ----------
constexpr QColor SUCCESS         = NEON_LIME;
constexpr QColor WARNING         = NEON_ORANGE;
constexpr QColor ERROR           = NEON_RED;
constexpr QColor INFO            = NEON_BLUE;
constexpr QColor ACCENT_PRIMARY  = NEON_CYAN;
constexpr QColor ACCENT_SECONDARY= NEON_MAGENTA;

// ---------- Chart Colors (High Contrast Gradients) ----------
inline const std::array<QColor, 8> CHART_COLORS = {
    NEON_CYAN,
    NEON_MAGENTA,
    NEON_LIME,
    NEON_ORANGE,
    ELECTRIC_PURPLE,
    NEON_BLUE,
    NEON_YELLOW,
    NEON_RED
};

// ---------- Heatmap Colors (Blue → Green → Yellow → Red) ----------
inline const std::array<QColor, 5> HEATMAP = {
    QColor{0x00, 0x00, 0x80},   // Deep blue (low)
    NEON_BLUE,                   // Blue
    NEON_LIME,                   // Green (mid)
    NEON_YELLOW,                 // Yellow
    NEON_RED                     // Red (high)
};

// ---------- Gradient Colors ----------
constexpr QColor GRADIENT_START  = NEON_CYAN;
constexpr QColor GRADIENT_MID    = ELECTRIC_PURPLE;
constexpr QColor GRADIENT_END    = NEON_MAGENTA;

// ---------- State Colors ----------
constexpr QColor TRAINING_ACTIVE {0x00, 0xE5, 0x80};        // Bright green glow
constexpr QColor TRAINING_PAUSED {0xFF, 0xA5, 0x00};        // Amber
constexpr QColor TRAINING_STOPPED{0x88, 0x88, 0x90};        // Muted grey
constexpr QColor TRAINING_ERROR  {0xFF, 0x20, 0x40};        // Red pulse

// ---------- Layer Type Colors (for graph visualization) ----------
inline const std::array<QColor, 12> LAYER_COLORS = {
    QColor{0x38, 0xBD, 0xF8},   // Conv - Blue
    QColor{0x00, 0xFF, 0x94},   // Linear - Lime
    QColor{0xA8, 0x55, 0xF7},   // Norm - Purple
    QColor{0xFF, 0x8C, 0x00},   // Activation - Orange
    QColor{0x00, 0xE5, 0xFF},   // Attention - Cyan
    QColor{0xFF, 0x00, 0xD0},   // Pool - Magenta
    QColor{0xFA, 0xCC, 0x15},   // Embed - Yellow
    QColor{0xFF, 0x35, 0x55},   // Loss - Red
    QColor{0x5A, 0xF7, 0x8F},   // Recurrent - Mint
    QColor{0xF8, 0x71, 0x71},   // Dropout - Coral
    QColor{0x84, 0xCC, 0x16},   // Skip/Add - Olive
    QColor{0x8B, 0x5C, 0xF6}    // Other - Violet
};

// ---------- Transparency Variants ----------
inline QColor withAlpha(const QColor& c, int alpha) {
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

inline const QColor GLASS_OVERLAY = withAlpha(VOID_BLACK, 180);     // Semi-transparent overlay
inline const QColor GLOW_CYAN     = withAlpha(NEON_CYAN, 60);       // Soft cyan glow
inline const QColor GLOW_MAGENTA  = withAlpha(NEON_MAGENTA, 60);    // Soft magenta glow
inline const QColor HOVER_OVERLAY = withAlpha(FROST, 15);           // Subtle hover highlight

} // namespace colors

// ============================================================================
// Typography
// ============================================================================

namespace fonts {

// Font families (with fallbacks)
constexpr const char* MONO_FAMILY = "JetBrains Mono, Fira Code, Consolas, monospace";
constexpr const char* SANS_FAMILY = "Inter, SF Pro Display, Segoe UI, sans-serif";

// Font sizes (in points)
constexpr int SIZE_TINY   = 9;
constexpr int SIZE_SMALL  = 11;
constexpr int SIZE_NORMAL = 13;
constexpr int SIZE_MEDIUM = 15;
constexpr int SIZE_LARGE  = 18;
constexpr int SIZE_XLARGE = 24;
constexpr int SIZE_TITLE  = 32;

// Common font presets
inline QFont monoSmall() {
    QFont f(MONO_FAMILY);
    f.setPointSize(SIZE_SMALL);
    return f;
}

inline QFont monoNormal() {
    QFont f(MONO_FAMILY);
    f.setPointSize(SIZE_NORMAL);
    return f;
}

inline QFont sansNormal() {
    QFont f(SANS_FAMILY);
    f.setPointSize(SIZE_NORMAL);
    return f;
}

inline QFont sansBold() {
    QFont f(SANS_FAMILY);
    f.setPointSize(SIZE_NORMAL);
    f.setBold(true);
    return f;
}

inline QFont sansLarge() {
    QFont f(SANS_FAMILY);
    f.setPointSize(SIZE_LARGE);
    return f;
}

inline QFont sansTitle() {
    QFont f(SANS_FAMILY);
    f.setPointSize(SIZE_TITLE);
    f.setWeight(QFont::DemiBold);
    return f;
}

} // namespace fonts

// ============================================================================
// Spacing & Sizing
// ============================================================================

namespace spacing {

constexpr int TINY   = 2;
constexpr int SMALL  = 4;
constexpr int NORMAL = 8;
constexpr int MEDIUM = 12;
constexpr int LARGE  = 16;
constexpr int XLARGE = 24;
constexpr int HUGE   = 32;

// Panel/card dimensions
constexpr int BORDER_RADIUS_SMALL  = 4;
constexpr int BORDER_RADIUS_NORMAL = 8;
constexpr int BORDER_RADIUS_LARGE  = 12;

constexpr int BORDER_WIDTH_THIN    = 1;
constexpr int BORDER_WIDTH_NORMAL  = 2;

// Widget sizing
constexpr int SLIDER_HEIGHT    = 24;
constexpr int BUTTON_HEIGHT    = 32;
constexpr int BUTTON_MIN_WIDTH = 80;
constexpr int INPUT_HEIGHT     = 28;
constexpr int ICON_SIZE_SMALL  = 16;
constexpr int ICON_SIZE_NORMAL = 24;
constexpr int ICON_SIZE_LARGE  = 32;

} // namespace spacing

// ============================================================================
// Animation Timings (milliseconds)
// ============================================================================

namespace anim {

constexpr int INSTANT    = 0;
constexpr int FAST       = 100;
constexpr int NORMAL     = 200;
constexpr int SMOOTH     = 300;
constexpr int SLOW       = 500;
constexpr int VERY_SLOW  = 1000;

// Specific animations
constexpr int HOVER      = FAST;
constexpr int TRANSITION = NORMAL;
constexpr int EXPAND     = SMOOTH;
constexpr int PULSE      = VERY_SLOW;
constexpr int GLOW       = 2000;      // Breathing glow effect

} // namespace anim

// ============================================================================
// Style Presets (QString for QSS)
// ============================================================================

namespace styles {

// Base panel style
inline QString panel() {
    return QString(R"(
        background-color: %1;
        border: %2px solid %3;
        border-radius: %4px;
    )")
    .arg(colors::CARBON.name())
    .arg(spacing::BORDER_WIDTH_THIN)
    .arg(colors::SLATE.name())
    .arg(spacing::BORDER_RADIUS_NORMAL);
}

// Card style (elevated panel)
inline QString card() {
    return QString(R"(
        background-color: %1;
        border: %2px solid %3;
        border-radius: %4px;
    )")
    .arg(colors::GRAPHITE.name())
    .arg(spacing::BORDER_WIDTH_THIN)
    .arg(colors::SLATE.name())
    .arg(spacing::BORDER_RADIUS_SMALL);
}

// Glowing accent border
inline QString glowBorder(const QColor& color) {
    return QString(R"(
        border: 2px solid %1;
        border-radius: %2px;
    )")
    .arg(color.name())
    .arg(spacing::BORDER_RADIUS_NORMAL);
}

// Primary button
inline QString buttonPrimary() {
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: %3px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %4;
        }
        QPushButton:pressed {
            background-color: %5;
        }
        QPushButton:disabled {
            background-color: %6;
            color: %7;
        }
    )")
    .arg(colors::NEON_CYAN.name())
    .arg(colors::VOID_BLACK.name())
    .arg(spacing::BORDER_RADIUS_SMALL)
    .arg(colors::NEON_CYAN.lighter(110).name())
    .arg(colors::NEON_CYAN.darker(120).name())
    .arg(colors::SLATE.name())
    .arg(colors::STEEL.name());
}

// Secondary button (outlined)
inline QString buttonSecondary() {
    return QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: 2px solid %1;
            border-radius: %2px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background-color: %3;
        }
        QPushButton:pressed {
            background-color: %4;
        }
    )")
    .arg(colors::NEON_CYAN.name())
    .arg(spacing::BORDER_RADIUS_SMALL)
    .arg(colors::GLOW_CYAN.name())
    .arg(colors::NEON_CYAN.name());
}

// Danger button
inline QString buttonDanger() {
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: %3px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %4;
        }
    )")
    .arg(colors::NEON_RED.name())
    .arg(colors::PURE_WHITE.name())
    .arg(spacing::BORDER_RADIUS_SMALL)
    .arg(colors::NEON_RED.lighter(110).name());
}

// Slider style
inline QString slider() {
    return QString(R"(
        QSlider::groove:horizontal {
            background: %1;
            height: 8px;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: %2;
            width: 16px;
            height: 16px;
            margin: -4px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background: %3;
        }
        QSlider::sub-page:horizontal {
            background: %4;
            border-radius: 4px;
        }
    )")
    .arg(colors::SLATE.name())
    .arg(colors::NEON_CYAN.name())
    .arg(colors::NEON_CYAN.lighter(110).name())
    .arg(colors::NEON_CYAN.darker(150).name());
}

// Text input
inline QString input() {
    return QString(R"(
        QLineEdit, QTextEdit {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: %4px;
            padding: 6px 8px;
            selection-background-color: %5;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: %6;
        }
    )")
    .arg(colors::GRAPHITE.name())
    .arg(colors::FROST.name())
    .arg(colors::SLATE.name())
    .arg(spacing::BORDER_RADIUS_SMALL)
    .arg(colors::NEON_CYAN.darker(200).name())
    .arg(colors::NEON_CYAN.name());
}

// Scroll bar
inline QString scrollbar() {
    return QString(R"(
        QScrollBar:vertical {
            background: %1;
            width: 10px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: %2;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: %3;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            height: 0;
        }
        QScrollBar:horizontal {
            background: %1;
            height: 10px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal {
            background: %2;
            border-radius: 5px;
            min-width: 20px;
        }
    )")
    .arg(colors::CARBON.name())
    .arg(colors::SLATE.name())
    .arg(colors::STEEL.name());
}

// Tooltip
inline QString tooltip() {
    return QString(R"(
        QToolTip {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 4px 8px;
        }
    )")
    .arg(colors::GRAPHITE.name())
    .arg(colors::FROST.name())
    .arg(colors::NEON_CYAN.name());
}

// Complete application stylesheet
inline QString applicationStyle() {
    return QString(R"(
        * {
            font-family: %1;
            font-size: %2px;
        }
        QMainWindow, QWidget {
            background-color: %3;
            color: %4;
        }
        QLabel {
            color: %4;
        }
        QGroupBox {
            border: 1px solid %5;
            border-radius: %6px;
            margin-top: 12px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            color: %7;
        }
        QSplitter::handle {
            background: %5;
        }
        QTabWidget::pane {
            border: 1px solid %5;
            border-radius: %6px;
            background: %8;
        }
        QTabBar::tab {
            background: %8;
            color: %9;
            padding: 8px 16px;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:selected {
            color: %7;
            border-bottom-color: %10;
        }
        QTabBar::tab:hover {
            color: %4;
        }
    )")
    .arg(fonts::SANS_FAMILY)
    .arg(fonts::SIZE_NORMAL)
    .arg(colors::SPACE_GREY.name())
    .arg(colors::FROST.name())
    .arg(colors::SLATE.name())
    .arg(spacing::BORDER_RADIUS_NORMAL)
    .arg(colors::NEON_CYAN.name())
    .arg(colors::CARBON.name())
    .arg(colors::SILVER.name())
    .arg(colors::NEON_CYAN.name())
    + input()
    + scrollbar()
    + tooltip();
}

} // namespace styles

} // namespace lumora::gui::theme
