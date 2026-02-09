#pragma once
/**
 * @file Observer.hpp
 * @brief Ambient cursor-tracking presence
 * 
 * The Observer is a small, unobtrusive entity that lives in the corner
 * of the IDE and provides ambient feedback about system state.
 * 
 * Features:
 * - Follows cursor with smooth eye tracking
 * - Reacts to system state (blinks more during errors, etc.)
 * - Provides subtle visual feedback without distraction
 * - Optional - can be completely disabled
 * 
 *    ┌─────┐
 *    │ ◉ ◉ │  <- Eyes track cursor
 *    └─────┘
 * 
 * States:
 * - Idle: Slow blink, relaxed
 * - Training: Alert, follows cursor actively
 * - Paused: Half-closed eyes
 * - Error: Wide eyes, rapid blink, red tint
 * - Finished: Happy expression
 */

#include "../core/Types.hpp"
#include "../theme/NeonPalette.hpp"

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <cmath>
#include <random>

namespace lumora::gui::widgets {

/**
 * @brief Ambient cursor-tracking entity
 */
class Observer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float blinkPhase READ blinkPhase WRITE setBlinkPhase)
    Q_PROPERTY(float emotionIntensity READ emotionIntensity WRITE setEmotionIntensity)
    
public:
    explicit Observer(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_eyeRadius(8.0f)
        , m_pupilRadius(3.0f)
        , m_eyeSpacing(24.0f)
        , m_blinkPhase(0.0f)
        , m_emotionIntensity(0.0f)
        , m_state(SystemState::Idle)
        , m_lastCursorPos(0, 0)
        , m_targetPupilOffset(0, 0)
        , m_currentPupilOffset(0, 0)
        , m_isSleeping(false)
    {
        setFixedSize(60, 40);
        setMouseTracking(true);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        
        // Random number generator for natural movements
        m_rng.seed(std::random_device{}());
        
        // Blink timer
        m_blinkTimer = new QTimer(this);
        connect(m_blinkTimer, &QTimer::timeout, this, &Observer::doBlink);
        scheduleNextBlink();
        
        // Eye tracking update
        m_trackTimer = new QTimer(this);
        connect(m_trackTimer, &QTimer::timeout, this, &Observer::updateEyeTracking);
        m_trackTimer->start(16);  // ~60 FPS
        
        // Micro-movement timer (saccades)
        m_saccadeTimer = new QTimer(this);
        connect(m_saccadeTimer, &QTimer::timeout, this, &Observer::doMicroMovement);
        scheduleNextSaccade();
    }
    
    void setState(SystemState state) {
        m_state = state;
        
        // Adjust behavior based on state
        switch (state) {
            case SystemState::Idle:
                m_blinkInterval = {2000, 6000};  // Relaxed
                m_isSleeping = false;
                break;
            case SystemState::Training:
                m_blinkInterval = {3000, 8000};  // Alert
                m_isSleeping = false;
                break;
            case SystemState::Paused:
                m_blinkInterval = {1000, 3000};  // Sleepy
                m_isSleeping = false;
                break;
            case SystemState::Error:
                m_blinkInterval = {500, 1500};   // Startled
                m_isSleeping = false;
                triggerEmotionPulse(1.0f, 2000);
                break;
            case SystemState::Finished:
                m_blinkInterval = {2000, 5000};  // Content
                m_isSleeping = false;
                break;
        }
        
        update();
    }
    
    void triggerEmotionPulse(float intensity, int durationMs) {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "emotionIntensity");
        anim->setDuration(durationMs);
        anim->setStartValue(intensity);
        anim->setEndValue(0.0f);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
    void setAsleep(bool asleep) {
        m_isSleeping = asleep;
        update();
    }
    
    float blinkPhase() const { return m_blinkPhase; }
    void setBlinkPhase(float phase) { m_blinkPhase = phase; update(); }
    
    float emotionIntensity() const { return m_emotionIntensity; }
    void setEmotionIntensity(float intensity) { m_emotionIntensity = intensity; update(); }
    
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        
        // Face background (subtle)
        QColor bgColor = theme::colors::CARBON;
        if (m_emotionIntensity > 0 && m_state == SystemState::Error) {
            // Red tint for errors
            int red = int(m_emotionIntensity * 40);
            bgColor = QColor(bgColor.red() + red, bgColor.green(), bgColor.blue());
        }
        
        p.setBrush(bgColor);
        p.setPen(QPen(theme::colors::SLATE, 1));
        p.drawRoundedRect(2, 2, w - 4, h - 4, 8, 8);
        
        // Calculate eye positions
        float centerX = w / 2.0f;
        float centerY = h / 2.0f;
        float leftEyeX = centerX - m_eyeSpacing / 2;
        float rightEyeX = centerX + m_eyeSpacing / 2;
        
        // Draw eyes
        drawEye(p, leftEyeX, centerY, m_currentPupilOffset);
        drawEye(p, rightEyeX, centerY, m_currentPupilOffset);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        m_lastCursorPos = event->pos();
        QWidget::mouseMoveEvent(event);
    }
    
    void enterEvent(QEnterEvent* event) override {
        // Wake up when cursor enters
        if (m_isSleeping) {
            m_isSleeping = false;
            update();
        }
        QWidget::enterEvent(event);
    }
    
private:
    void drawEye(QPainter& p, float cx, float cy, QPointF pupilOffset) {
        // Eye white (sclera)
        QColor scleraColor = theme::colors::FROST;
        
        // Apply blink by squashing eye vertically
        float blinkScale = 1.0f - m_blinkPhase;
        if (m_isSleeping) {
            blinkScale = 0.2f;  // Half-closed when sleeping
        }
        
        float eyeHeight = m_eyeRadius * 2 * blinkScale;
        float eyeWidth = m_eyeRadius * 2;
        
        // Eye outline with glow
        if (m_state == SystemState::Training) {
            // Subtle cyan glow when training
            QRadialGradient glow(cx, cy, m_eyeRadius * 1.5);
            glow.setColorAt(0, theme::colors::withAlpha(theme::colors::NEON_CYAN, 40));
            glow.setColorAt(1, Qt::transparent);
            p.setBrush(glow);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx, cy), m_eyeRadius * 1.5, m_eyeRadius * 1.5 * blinkScale);
        }
        
        // Sclera
        p.setBrush(scleraColor);
        p.setPen(QPen(theme::colors::SILVER, 1));
        p.drawEllipse(QPointF(cx, cy), m_eyeRadius, m_eyeRadius * blinkScale);
        
        // Pupil (only if eye is mostly open)
        if (blinkScale > 0.3f) {
            // Clamp pupil to eye bounds
            float maxOffset = m_eyeRadius - m_pupilRadius - 1;
            float px = std::clamp(float(pupilOffset.x()), -maxOffset, maxOffset);
            float py = std::clamp(float(pupilOffset.y()), -maxOffset * blinkScale, maxOffset * blinkScale);
            
            // Pupil size varies with state
            float pupilSize = m_pupilRadius;
            if (m_state == SystemState::Error && m_emotionIntensity > 0) {
                pupilSize *= 1.0f + m_emotionIntensity * 0.5f;  // Dilated in fear
            } else if (m_state == SystemState::Paused) {
                pupilSize *= 0.8f;  // Constricted when drowsy
            }
            
            // Draw pupil
            QColor pupilColor = theme::colors::VOID_BLACK;
            if (m_state == SystemState::Error) {
                pupilColor = QColor::fromHsvF(0.0f, m_emotionIntensity, 0.2f);
            }
            
            p.setBrush(pupilColor);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx + px, cy + py), pupilSize, pupilSize);
            
            // Specular highlight
            p.setBrush(Qt::white);
            p.drawEllipse(QPointF(cx + px - pupilSize * 0.3f, cy + py - pupilSize * 0.3f), 
                          pupilSize * 0.3f, pupilSize * 0.3f);
        }
    }
    
    void updateEyeTracking() {
        // Get global cursor position
        QPoint globalCursor = QCursor::pos();
        QPoint localCursor = mapFromGlobal(globalCursor);
        
        // Calculate target pupil offset based on cursor position
        float dx = localCursor.x() - width() / 2.0f;
        float dy = localCursor.y() - height() / 2.0f;
        
        // Normalize and scale
        float dist = std::sqrt(dx * dx + dy * dy);
        float maxDist = 200.0f;  // Distance at which eyes are fully tracking
        float trackStrength = std::min(dist / maxDist, 1.0f);
        
        float maxOffset = m_eyeRadius - m_pupilRadius - 1;
        m_targetPupilOffset = QPointF(
            (dx / maxDist) * maxOffset * trackStrength,
            (dy / maxDist) * maxOffset * trackStrength
        );
        
        // Smooth interpolation
        float smoothing = 0.15f;  // Lower = smoother
        m_currentPupilOffset = m_currentPupilOffset + 
            (m_targetPupilOffset - m_currentPupilOffset) * smoothing;
        
        update();
    }
    
    void doBlink() {
        // Animate blink
        QPropertyAnimation* closeAnim = new QPropertyAnimation(this, "blinkPhase");
        closeAnim->setDuration(60);
        closeAnim->setStartValue(0.0f);
        closeAnim->setEndValue(1.0f);
        
        QPropertyAnimation* openAnim = new QPropertyAnimation(this, "blinkPhase");
        openAnim->setDuration(80);
        openAnim->setStartValue(1.0f);
        openAnim->setEndValue(0.0f);
        
        QSequentialAnimationGroup* blinkGroup = new QSequentialAnimationGroup(this);
        blinkGroup->addAnimation(closeAnim);
        blinkGroup->addAnimation(openAnim);
        blinkGroup->start(QAbstractAnimation::DeleteWhenStopped);
        
        scheduleNextBlink();
    }
    
    void scheduleNextBlink() {
        std::uniform_int_distribution<int> dist(m_blinkInterval.first, m_blinkInterval.second);
        m_blinkTimer->start(dist(m_rng));
    }
    
    void doMicroMovement() {
        // Small random saccade movements for natural look
        if (!m_isSleeping && m_blinkPhase < 0.5f) {
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            QPointF jitter(dist(m_rng), dist(m_rng));
            m_currentPupilOffset += jitter * 0.5f;
        }
        
        scheduleNextSaccade();
    }
    
    void scheduleNextSaccade() {
        std::uniform_int_distribution<int> dist(100, 500);
        m_saccadeTimer->start(dist(m_rng));
    }
    
private:
    float m_eyeRadius;
    float m_pupilRadius;
    float m_eyeSpacing;
    float m_blinkPhase;
    float m_emotionIntensity;
    
    SystemState m_state;
    QPoint m_lastCursorPos;
    QPointF m_targetPupilOffset;
    QPointF m_currentPupilOffset;
    bool m_isSleeping;
    
    std::pair<int, int> m_blinkInterval = {2000, 6000};
    
    QTimer* m_blinkTimer;
    QTimer* m_trackTimer;
    QTimer* m_saccadeTimer;
    
    std::mt19937 m_rng;
};

/**
 * @brief Floating Observer that can be placed anywhere
 */
class FloatingObserver : public QWidget {
    Q_OBJECT
    
public:
    explicit FloatingObserver(QWidget* parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(70, 50);
        
        m_observer = new Observer(this);
        m_observer->move(5, 5);
        
        // Draggable
        m_isDragging = false;
    }
    
    Observer* observer() { return m_observer; }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_isDragging = true;
            m_dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_isDragging) {
            move(event->globalPosition().toPoint() - m_dragStart);
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_isDragging = false;
        }
    }
    
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        // Subtle shadow
        p.setBrush(theme::colors::withAlpha(theme::colors::VOID_BLACK, 100));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect().adjusted(3, 3, 0, 0), 10, 10);
    }
    
private:
    Observer* m_observer;
    bool m_isDragging;
    QPoint m_dragStart;
};

} // namespace lumora::gui::widgets
