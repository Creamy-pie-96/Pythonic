#pragma once
/**
 * @file XRayGraph.hpp
 * @brief Node-based neural network graph visualization
 * 
 * Interactive model topology view with real-time data flow.
 * Shows layers as nodes, connections as edges with gradient/activation flow.
 * 
 * Features:
 * - Pan/zoom navigation
 * - Node selection and inspection
 * - Gradient magnitude color-coding on edges
 * - Activation heatmap overlays
 * - Animated data flow (forward/backward pass)
 * 
 * Layout:
 * ┌─────────────────────────────────────────────────────────┐
 * │  [Zoom +/-] [Fit] [Auto-layout]           [Filter ▼]   │
 * ├─────────────────────────────────────────────────────────┤
 * │                                                         │
 * │    ┌─────┐   ┌─────┐   ┌─────┐                         │
 * │    │Input│───│Conv │───│ReLU │───...                   │
 * │    └─────┘   └──┬──┘   └─────┘                         │
 * │                 │                                       │
 * │              ┌──▼──┐                                    │
 * │              │ BN  │                                    │
 * │              └─────┘                                    │
 * │                                                         │
 * └─────────────────────────────────────────────────────────┘
 */

#include "../core/Types.hpp"
#include "../core/LumoraAPI.hpp"
#include "../theme/NeonPalette.hpp"

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <cmath>
#include <unordered_map>
#include <memory>

namespace lumora::gui::widgets {

// Forward declarations
class GraphNode;
class GraphEdge;

// ============================================================================
// Layer Node Item
// ============================================================================

/**
 * @brief Visual representation of a layer in the graph
 */
class LayerNodeItem : public QGraphicsItem {
public:
    LayerNodeItem(const GraphNode& data, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
        , m_data(data)
        , m_isSelected(false)
        , m_isHovered(false)
        , m_gradientMagnitude(0.0f)
    {
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsMovable);
        setAcceptHoverEvents(true);
        
        // Determine node color based on type
        m_typeColor = getColorForType(data.type);
        
        // Calculate size based on content
        m_width = std::max(100.0, 80 + data.name.length() * 7.0);
        m_height = 60;
    }
    
    QRectF boundingRect() const override {
        float margin = 10;  // For glow effect
        return QRectF(-m_width/2 - margin, -m_height/2 - margin, 
                      m_width + 2*margin, m_height + 2*margin);
    }
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setRenderHint(QPainter::Antialiasing);
        
        // Glow effect when selected or has high gradient
        float glowIntensity = 0;
        QColor glowColor = m_typeColor;
        
        if (m_isSelected) {
            glowIntensity = 1.0f;
            glowColor = theme::colors::NEON_CYAN;
        } else if (m_gradientMagnitude > 0.5f) {
            glowIntensity = m_gradientMagnitude;
        }
        
        if (glowIntensity > 0) {
            QRadialGradient glow(0, 0, m_width * 0.7);
            glow.setColorAt(0, theme::colors::withAlpha(glowColor, int(80 * glowIntensity)));
            glow.setColorAt(1, Qt::transparent);
            painter->setBrush(glow);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(boundingRect().adjusted(5, 5, -5, -5));
        }
        
        // Node body
        QRectF nodeRect(-m_width/2, -m_height/2, m_width, m_height);
        
        // Background
        QColor bgColor = m_isHovered ? 
            theme::colors::GRAPHITE.lighter(110) : theme::colors::GRAPHITE;
        painter->setBrush(bgColor);
        
        // Border
        QColor borderColor = m_isSelected ? theme::colors::NEON_CYAN : m_typeColor;
        painter->setPen(QPen(borderColor, m_isSelected ? 2 : 1));
        
        painter->drawRoundedRect(nodeRect, 8, 8);
        
        // Type indicator bar at top
        QRectF typeBar(-m_width/2, -m_height/2, m_width, 4);
        painter->setBrush(m_typeColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(typeBar, 2, 2);
        
        // Layer name
        painter->setPen(theme::colors::FROST);
        painter->setFont(theme::fonts::sansBold());
        QRectF textRect = nodeRect.adjusted(8, 8, -8, -20);
        painter->drawText(textRect, Qt::AlignCenter, QString::fromStdString(m_data.name));
        
        // Shape info
        painter->setPen(theme::colors::SILVER);
        painter->setFont(theme::fonts::monoSmall());
        QString shapeStr = formatShape(m_data.outputShape);
        painter->drawText(nodeRect.adjusted(8, 0, -8, -8), 
                          Qt::AlignHCenter | Qt::AlignBottom, shapeStr);
    }
    
    void setGradientMagnitude(float mag) {
        m_gradientMagnitude = std::clamp(mag, 0.0f, 1.0f);
        update();
    }
    
    void setSelected(bool selected) {
        m_isSelected = selected;
        update();
    }
    
    LayerId layerId() const { return m_data.id; }
    const GraphNode& nodeData() const { return m_data; }
    
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override {
        m_isHovered = true;
        update();
    }
    
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override {
        m_isHovered = false;
        update();
    }
    
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        emit nodeSelected(m_data.id);
        QGraphicsItem::mousePressEvent(event);
    }
    
signals:
    void nodeSelected(LayerId id);
    
private:
    static QColor getColorForType(LayerType type) {
        switch (type) {
            case LayerType::Conv:       return theme::colors::LAYER_COLORS[0];
            case LayerType::Linear:     return theme::colors::LAYER_COLORS[1];
            case LayerType::Norm:       return theme::colors::LAYER_COLORS[2];
            case LayerType::Activation: return theme::colors::LAYER_COLORS[3];
            case LayerType::Attention:  return theme::colors::LAYER_COLORS[4];
            case LayerType::Pool:       return theme::colors::LAYER_COLORS[5];
            case LayerType::Embed:      return theme::colors::LAYER_COLORS[6];
            case LayerType::Loss:       return theme::colors::LAYER_COLORS[7];
            case LayerType::Recurrent:  return theme::colors::LAYER_COLORS[8];
            case LayerType::Dropout:    return theme::colors::LAYER_COLORS[9];
            case LayerType::Skip:       return theme::colors::LAYER_COLORS[10];
            default:                    return theme::colors::LAYER_COLORS[11];
        }
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
    
    GraphNode m_data;
    bool m_isSelected;
    bool m_isHovered;
    float m_gradientMagnitude;
    QColor m_typeColor;
    double m_width, m_height;
};

// ============================================================================
// Edge Item
// ============================================================================

/**
 * @brief Connection between layers with gradient visualization
 */
class EdgeItem : public QGraphicsItem {
public:
    EdgeItem(LayerNodeItem* source, LayerNodeItem* target, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
        , m_source(source)
        , m_target(target)
        , m_gradientFlow(0.0f)
        , m_flowPhase(0.0f)
    {
        setZValue(-1);  // Draw behind nodes
    }
    
    QRectF boundingRect() const override {
        if (!m_source || !m_target) return QRectF();
        
        QPointF p1 = m_source->pos();
        QPointF p2 = m_target->pos();
        return QRectF(p1, p2).normalized().adjusted(-20, -20, 20, 20);
    }
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        if (!m_source || !m_target) return;
        
        painter->setRenderHint(QPainter::Antialiasing);
        
        QPointF p1 = m_source->pos();
        QPointF p2 = m_target->pos();
        
        // Calculate control points for curved path
        QPointF mid = (p1 + p2) / 2;
        QPointF diff = p2 - p1;
        QPointF ctrl(mid.x() + diff.y() * 0.2, mid.y() - diff.x() * 0.2);
        
        QPainterPath path;
        path.moveTo(p1);
        path.quadTo(ctrl, p2);
        
        // Edge color based on gradient flow
        QColor edgeColor = interpolateGradientColor(m_gradientFlow);
        
        // Draw base edge
        painter->setPen(QPen(edgeColor, 2));
        painter->drawPath(path);
        
        // Animated flow particles
        if (m_gradientFlow > 0.1f) {
            drawFlowParticles(painter, path);
        }
        
        // Arrow head at target
        drawArrowHead(painter, path, edgeColor);
    }
    
    void setGradientFlow(float flow) {
        m_gradientFlow = std::clamp(flow, 0.0f, 1.0f);
        update();
    }
    
    void advanceFlowPhase(float delta) {
        m_flowPhase += delta;
        if (m_flowPhase > 1.0f) m_flowPhase -= 1.0f;
        update();
    }
    
private:
    static QColor interpolateGradientColor(float value) {
        // Blue (low) -> Cyan -> Green (mid) -> Yellow -> Red (high)
        if (value < 0.25f) {
            return QColor::fromHsvF(0.6f, 0.7f, 0.5f + value * 2);  // Blue
        } else if (value < 0.5f) {
            return QColor::fromHsvF(0.5f - (value - 0.25f), 0.8f, 0.8f);  // Cyan to green
        } else if (value < 0.75f) {
            return QColor::fromHsvF(0.3f - (value - 0.5f) * 0.6f, 0.9f, 0.9f);  // Green to yellow
        } else {
            return QColor::fromHsvF(0.0f, 1.0f, 1.0f);  // Red
        }
    }
    
    void drawFlowParticles(QPainter* painter, const QPainterPath& path) {
        painter->setBrush(Qt::white);
        painter->setPen(Qt::NoPen);
        
        int numParticles = 3;
        for (int i = 0; i < numParticles; ++i) {
            float t = std::fmod(m_flowPhase + i * 0.33f, 1.0f);
            QPointF pt = path.pointAtPercent(t);
            float size = 3 + m_gradientFlow * 2;
            painter->drawEllipse(pt, size, size);
        }
    }
    
    void drawArrowHead(QPainter* painter, const QPainterPath& path, const QColor& color) {
        // Get point and tangent near end of path
        QPointF endPt = path.pointAtPercent(0.95);
        QPointF tipPt = path.pointAtPercent(1.0);
        QPointF dir = tipPt - endPt;
        float len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len < 0.001f) return;
        dir /= len;
        
        // Arrow size
        float arrowLen = 10;
        float arrowWidth = 6;
        
        QPointF p1 = tipPt;
        QPointF p2 = tipPt - dir * arrowLen + QPointF(-dir.y(), dir.x()) * arrowWidth;
        QPointF p3 = tipPt - dir * arrowLen - QPointF(-dir.y(), dir.x()) * arrowWidth;
        
        QPolygonF arrow;
        arrow << p1 << p2 << p3;
        
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawPolygon(arrow);
    }
    
    LayerNodeItem* m_source;
    LayerNodeItem* m_target;
    float m_gradientFlow;
    float m_flowPhase;
};

// ============================================================================
// Graph View
// ============================================================================

/**
 * @brief Custom QGraphicsView with pan/zoom
 */
class GraphView : public QGraphicsView {
    Q_OBJECT
    
public:
    explicit GraphView(QWidget* parent = nullptr)
        : QGraphicsView(parent)
        , m_isPanning(false)
    {
        setRenderHint(QPainter::Antialiasing);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setDragMode(QGraphicsView::NoDrag);
        
        setBackgroundBrush(theme::colors::SPACE_GREY);
        
        // Enable smooth zooming
        setTransform(QTransform::fromScale(1.0, 1.0));
    }
    
    void zoomIn() { scale(1.2, 1.2); }
    void zoomOut() { scale(1 / 1.2, 1 / 1.2); }
    
    void fitToContent() {
        if (scene()) {
            fitInView(scene()->itemsBoundingRect().adjusted(-50, -50, 50, 50), 
                      Qt::KeepAspectRatio);
        }
    }
    
protected:
    void wheelEvent(QWheelEvent* event) override {
        // Zoom with wheel
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    }
    
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::MiddleButton) {
            m_isPanning = true;
            m_lastPanPos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
        } else {
            QGraphicsView::mousePressEvent(event);
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_isPanning) {
            QPoint delta = event->pos() - m_lastPanPos;
            m_lastPanPos = event->pos();
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            event->accept();
        } else {
            QGraphicsView::mouseMoveEvent(event);
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::MiddleButton) {
            m_isPanning = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
        } else {
            QGraphicsView::mouseReleaseEvent(event);
        }
    }
    
private:
    bool m_isPanning;
    QPoint m_lastPanPos;
};

// ============================================================================
// X-Ray Graph Widget
// ============================================================================

/**
 * @brief Main X-Ray Model Graph panel
 */
class XRayGraph : public QWidget {
    Q_OBJECT
    
public:
    explicit XRayGraph(IDataProvider* provider, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_provider(provider)
        , m_modelVersion(0)
    {
        setupUI();
        
        // Animation timer for flow effects
        m_flowTimer = new QTimer(this);
        connect(m_flowTimer, &QTimer::timeout, this, &XRayGraph::animateFlow);
        m_flowTimer->start(50);  // 20 FPS for flow animation
        
        // Model update timer
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &XRayGraph::checkModelUpdate);
        m_updateTimer->start(500);  // Check for model changes every 500ms
    }
    
    void setProvider(IDataProvider* provider) {
        m_provider = provider;
        m_modelVersion = 0;
        rebuildGraph();
    }
    
signals:
    void nodeSelected(LayerId layerId);
    void nodeDoubleClicked(LayerId layerId);
    
public slots:
    void rebuildGraph() {
        if (!m_provider) return;
        
        m_scene->clear();
        m_nodeItems.clear();
        m_edgeItems.clear();
        
        ModelGraph graph = m_provider->getModelGraph();
        m_modelVersion = m_provider->getModelVersion();
        
        // Create nodes
        for (const auto& node : graph.nodes) {
            auto* item = new LayerNodeItem(node);
            m_scene->addItem(item);
            m_nodeItems[node.id] = item;
        }
        
        // Create edges
        for (const auto& edge : graph.edges) {
            auto srcIt = m_nodeItems.find(edge.source);
            auto dstIt = m_nodeItems.find(edge.target);
            if (srcIt != m_nodeItems.end() && dstIt != m_nodeItems.end()) {
                auto* edgeItem = new EdgeItem(srcIt->second, dstIt->second);
                m_scene->addItem(edgeItem);
                m_edgeItems.push_back(edgeItem);
            }
        }
        
        // Auto-layout
        autoLayout();
        m_view->fitToContent();
    }
    
    void autoLayout() {
        // Simple topological layout (layers in columns)
        // More sophisticated layouts can be added later
        
        std::unordered_map<LayerId, int> depths;
        std::vector<LayerId> sorted;
        
        // Calculate depth for each node (longest path from start)
        for (auto& [id, item] : m_nodeItems) {
            depths[id] = 0;
        }
        
        // Find root nodes (no incoming edges)
        std::unordered_set<LayerId> hasIncoming;
        for (const auto& edge : m_edgeItems) {
            // Can't access edge target directly, skip for now
        }
        
        // Simple grid layout for now
        int col = 0, row = 0;
        int maxRow = std::max(1, int(std::sqrt(m_nodeItems.size())));
        float xSpacing = 180;
        float ySpacing = 100;
        
        for (auto& [id, item] : m_nodeItems) {
            float x = col * xSpacing;
            float y = row * ySpacing;
            item->setPos(x, y);
            
            row++;
            if (row >= maxRow) {
                row = 0;
                col++;
            }
        }
    }
    
private slots:
    void checkModelUpdate() {
        if (!m_provider) return;
        
        uint64_t newVersion = m_provider->getModelVersion();
        if (newVersion != m_modelVersion) {
            rebuildGraph();
        }
    }
    
    void animateFlow() {
        for (auto* edge : m_edgeItems) {
            edge->advanceFlowPhase(0.02f);
        }
    }
    
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(theme::spacing::SMALL);
        layout->setContentsMargins(0, 0, 0, 0);
        
        // Toolbar
        QHBoxLayout* toolbar = new QHBoxLayout();
        
        QPushButton* zoomInBtn = new QPushButton("+", this);
        QPushButton* zoomOutBtn = new QPushButton("-", this);
        QPushButton* fitBtn = new QPushButton("Fit", this);
        QPushButton* layoutBtn = new QPushButton("Auto-Layout", this);
        
        zoomInBtn->setFixedSize(30, 30);
        zoomOutBtn->setFixedSize(30, 30);
        
        QString btnStyle = theme::styles::buttonSecondary();
        zoomInBtn->setStyleSheet(btnStyle);
        zoomOutBtn->setStyleSheet(btnStyle);
        fitBtn->setStyleSheet(btnStyle);
        layoutBtn->setStyleSheet(btnStyle);
        
        connect(zoomInBtn, &QPushButton::clicked, [this]() { m_view->zoomIn(); });
        connect(zoomOutBtn, &QPushButton::clicked, [this]() { m_view->zoomOut(); });
        connect(fitBtn, &QPushButton::clicked, [this]() { m_view->fitToContent(); });
        connect(layoutBtn, &QPushButton::clicked, this, &XRayGraph::autoLayout);
        
        toolbar->addWidget(zoomInBtn);
        toolbar->addWidget(zoomOutBtn);
        toolbar->addWidget(fitBtn);
        toolbar->addWidget(layoutBtn);
        toolbar->addStretch();
        
        // Filter dropdown
        m_filterCombo = new QComboBox(this);
        m_filterCombo->addItem("All Layers");
        m_filterCombo->addItem("Trainable Only");
        m_filterCombo->addItem("Conv/Linear");
        m_filterCombo->addItem("Attention Only");
        m_filterCombo->setStyleSheet(theme::styles::input());
        toolbar->addWidget(m_filterCombo);
        
        layout->addLayout(toolbar);
        
        // Graph view
        m_scene = new QGraphicsScene(this);
        m_view = new GraphView(this);
        m_view->setScene(m_scene);
        
        layout->addWidget(m_view, 1);
        
        setLayout(layout);
    }
    
    IDataProvider* m_provider;
    uint64_t m_modelVersion;
    
    QGraphicsScene* m_scene;
    GraphView* m_view;
    QComboBox* m_filterCombo;
    
    QTimer* m_flowTimer;
    QTimer* m_updateTimer;
    
    std::unordered_map<LayerId, LayerNodeItem*> m_nodeItems;
    std::vector<EdgeItem*> m_edgeItems;
};

} // namespace lumora::gui::widgets
