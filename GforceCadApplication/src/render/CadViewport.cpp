#include "CadViewport.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QOpenGLFunctions>
#include <cmath>
#include <limits>
#include <numbers>

namespace GForceCAD {

CadViewport::CadViewport(Document& document, ToolController& tools, QWidget* parent)
    : QOpenGLWidget(parent), m_document(document), m_tools(tools)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(700, 500);
}

void CadViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.055f, 0.065f, 0.075f, 1.0f);
}

void CadViewport::resizeGL(int w, int h)
{
    Q_UNUSED(w)
    Q_UNUSED(h)
}

QPointF CadViewport::worldToScreen(const Vec2& world) const
{
    return {
        width() * 0.5 + m_pan.x() + world.x * m_zoom,
        height() * 0.5 + m_pan.y() - world.y * m_zoom
    };
}

Vec2 CadViewport::screenToWorld(const QPointF& screen) const
{
    return {
        (screen.x() - width() * 0.5 - m_pan.x()) / m_zoom,
        -(screen.y() - height() * 0.5 - m_pan.y()) / m_zoom
    };
}

Vec2 CadViewport::snap(const Vec2& point) const
{
    if (m_objectSnap) {
        if (auto candidate = nearestObjectSnap(point))
            return *candidate;
    }

    return m_snap ? snapToGrid(point, 10.0) : point;
}

std::optional<Vec2> CadViewport::nearestObjectSnap(const Vec2& point) const
{
    const double tolerance = 12.0 / std::max(m_zoom, 0.05);
    double best = std::numeric_limits<double>::max();
    std::optional<Vec2> bestPoint;

    for (const auto& entity : m_document.entities()) {
        const Layer* layer = m_document.getLayer(entity->layer());
        if (layer && !layer->visible) continue;

        for (const Vec2& candidate : entity->snapPoints()) {
            const double d = distance(point, candidate);
            if (d <= tolerance && d < best) {
                best = d;
                bestPoint = candidate;
            }
        }
    }

    return bestPoint;
}

void CadViewport::paintGL()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor("#0e1115"));

    painter.save();

    painter.translate(
        width() * 0.5 + m_pan.x(),
        height() * 0.5 + m_pan.y()
    );
    painter.scale(m_zoom, -m_zoom);

    if (m_grid) {
        const double worldWidth = width() / m_zoom;
        const double worldHeight = height() / m_zoom;

        const double step = 10.0;
        const double xMin = -worldWidth / 2.0 - 100.0;
        const double xMax = worldWidth / 2.0 + 100.0;
        const double yMin = -worldHeight / 2.0 - 100.0;
        const double yMax = worldHeight / 2.0 + 100.0;

        QPen gridPen(QColor("#1d242c"));
        gridPen.setWidthF(0.0);
        painter.setPen(gridPen);

        for (double x = std::floor(xMin / step) * step; x <= xMax; x += step)
            painter.drawLine(QPointF(x, yMin), QPointF(x, yMax));

        for (double y = std::floor(yMin / step) * step; y <= yMax; y += step)
            painter.drawLine(QPointF(xMin, y), QPointF(xMax, y));

        QPen axisPen(QColor("#3a4652"));
        axisPen.setWidthF(0.0);
        painter.setPen(axisPen);
        painter.drawLine(QPointF(xMin, 0), QPointF(xMax, 0));
        painter.drawLine(QPointF(0, yMin), QPointF(0, yMax));
    }

    for (const auto& entity : m_document.entities()) {
        const Layer* layer = m_document.getLayer(entity->layer());
        if (layer && !layer->visible) continue;
        entity->draw(painter, m_zoom);
    }

    const auto primary = m_tools.primaryPreview();
    if (primary.valid) {
        QPen pen(QColor("#ffb347"));
        pen.setWidthF(std::max(1.0, 2.0 / std::max(m_zoom, 0.01)));
        painter.setPen(pen);
        painter.drawLine(toQPoint(primary.a), toQPoint(primary.b));
    }

    const auto hover = m_tools.hoverPreview();
    if (hover.valid) {
        QPen pen(QColor("#8ad0ff"));
        pen.setWidthF(std::max(1.0, 1.7 / std::max(m_zoom, 0.01)));
        painter.setPen(pen);
        painter.drawLine(toQPoint(hover.a), toQPoint(hover.b));
    }

    if (m_tools.hasIntersectionPreview()) {
        const Vec2 p = m_tools.intersectionPreview();
        const double size = 7.0 / std::max(m_zoom, 0.01);
        QPen pen(QColor("#ffd166"));
        pen.setWidthF(std::max(1.0, 1.6 / std::max(m_zoom, 0.01)));
        painter.setPen(pen);
        painter.drawLine(toQPoint({p.x - size, p.y - size}), toQPoint({p.x + size, p.y + size}));
        painter.drawLine(toQPoint({p.x - size, p.y + size}), toQPoint({p.x + size, p.y - size}));
    }

    drawPreview(painter);

    painter.restore();

    if (m_hasSnapCandidate) {
        const QPointF screen = worldToScreen(m_snapCandidate);
        QPen snapPen(QColor("#3be4ff"));
        snapPen.setWidth(2);
        painter.setPen(snapPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(screen, 5.0, 5.0);
        painter.drawLine(QPointF(screen.x() - 8.0, screen.y()), QPointF(screen.x() + 8.0, screen.y()));
        painter.drawLine(QPointF(screen.x(), screen.y() - 8.0), QPointF(screen.x(), screen.y() + 8.0));
    }

    // Crosshair
    QPoint cursor = mapFromGlobal(QCursor::pos());
    if (rect().contains(cursor)) {
        QPen cross(QColor("#52606d"));
        cross.setWidth(1);
        painter.setPen(cross);
        painter.drawLine(cursor.x(), 0, cursor.x(), height());
        painter.drawLine(0, cursor.y(), width(), cursor.y());
    }
}

void CadViewport::mousePressEvent(QMouseEvent* event)
{
    setFocus();
    m_lastMouse = event->position();

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_tools.tool() == ToolType::Select) {
            const Vec2 world = screenToWorld(event->position());
            auto entity = m_document.entityAt(world, 8.0 / std::max(m_zoom, 0.05));

            if (event->modifiers() & Qt::ShiftModifier) {
                if (entity) entity->setSelected(!entity->selected());
            } else {
                m_document.clearSelection();
                if (entity) entity->setSelected(true);
            }
        } else {
            const Vec2 point = snap(screenToWorld(event->position()));
            m_tools.click(point);
        }

        emit selectionChanged();
        emit viewportChanged();
        update();
    }
}

void CadViewport::mouseMoveEvent(QMouseEvent* event)
{
    const Vec2 world = screenToWorld(event->position());

    m_hasSnapCandidate = false;
    if (m_objectSnap) {
        if (auto candidate = nearestObjectSnap(world)) {
            m_hasSnapCandidate = true;
            m_snapCandidate = *candidate;
        }
    }

    const Vec2 point = snap(world);
    m_tools.move(point);
    emit cursorChanged(point);

    if (m_panning) {
        const QPointF delta = event->position() - m_lastMouse;
        m_pan += delta;
        m_lastMouse = event->position();
        emit viewportChanged();
    }

    update();
}

void CadViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        unsetCursor();
    }
}

void CadViewport::wheelEvent(QWheelEvent* event)
{
    const QPointF mouse = event->position();
    const Vec2 before = screenToWorld(mouse);

    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    m_zoom = clamp(m_zoom * factor, 0.05, 100.0);

    const QPointF after = worldToScreen(before);
    m_pan += mouse - after;

    emit viewportChanged();
    update();
}

void CadViewport::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        m_tools.cancel();
        update();
        return;
    }

    if (event->key() == Qt::Key_Delete) {
        m_document.removeSelected();
        emit selectionChanged();
        update();
        return;
    }

    QOpenGLWidget::keyPressEvent(event);
}

void CadViewport::zoomExtents()
{
    if (m_document.entities().empty()) {
        m_zoom = 1.0;
        m_pan = {};
        update();
        return;
    }

    // Practical starter behavior: reset to a useful CAD view.
    m_zoom = 1.0;
    m_pan = {};
    update();
}

void CadViewport::setGridEnabled(bool enabled)
{
    m_grid = enabled;
    update();
}

void CadViewport::setSnapEnabled(bool enabled)
{
    m_snap = enabled;
    update();
}

void CadViewport::setObjectSnapEnabled(bool enabled)
{
    m_objectSnap = enabled;
    m_hasSnapCandidate = false;
    update();
}

void CadViewport::drawPreview(QPainter& painter) const
{
    const QVector<Vec2>& points = m_tools.points();

    if (points.isEmpty()) return;

    QPen pen(QColor("#5d7f9a"));
    pen.setStyle(Qt::DashLine);
    pen.setWidthF(std::max(1.0, 1.2 / std::max(m_zoom, 0.01)));
    painter.setPen(pen);

    const Vec2 preview = m_tools.previewPoint();

    switch (m_tools.tool()) {
    case ToolType::Line:
        if (points.size() == 1)
            painter.drawLine(toQPoint(points[0]), toQPoint(preview));
        break;

    case ToolType::Circle:
        if (points.size() == 1) {
            const double radius = distance(points[0], preview);
            painter.drawEllipse(toQPoint(points[0]), radius, radius);
        }
        break;

    case ToolType::Ellipse:
        if (points.size() == 2) {
            const Vec2 center = points[0];
            const double semiMajor = std::abs(preview.x - center.x);
            const double semiMinor = std::abs((points.size() == 2 ? preview.y : center.y) - center.y);
            painter.drawEllipse(toQPoint(center), semiMajor, semiMinor);
        }
        if (points.size() == 1) {
            painter.drawLine(toQPoint(points[0]), toQPoint(preview));
        }
        break;

    case ToolType::Arc:
        if (points.size() == 2) {
            const Vec2 center = points[0];
            const double radius = distance(center, points[1]);

            auto angleDeg = [center](const Vec2& p) {
                return std::atan2(p.y - center.y, p.x - center.x) * 180.0 / std::numbers::pi;
            };

            const double start = angleDeg(points[1]);
            const double end = angleDeg(preview);

            QRectF r(center.x - radius, center.y - radius, 2.0 * radius, 2.0 * radius);
            painter.drawArc(r, static_cast<int>(start * 16.0), static_cast<int>((end - start) * 16.0));
        }
        break;

    case ToolType::Polyline:
        if (!points.isEmpty()) {
            for (int i = 1; i < points.size(); ++i)
                painter.drawLine(toQPoint(points[i - 1]), toQPoint(points[i]));

            painter.drawLine(toQPoint(points.last()), toQPoint(preview));
        }
        break;

    case ToolType::Rectangle:
        if (points.size() == 1) {
            const Vec2 a = points[0];
            const Vec2 b = preview;
            painter.drawLine(toQPoint({a.x, a.y}), toQPoint({b.x, a.y}));
            painter.drawLine(toQPoint({b.x, a.y}), toQPoint({b.x, b.y}));
            painter.drawLine(toQPoint({b.x, b.y}), toQPoint({a.x, b.y}));
            painter.drawLine(toQPoint({a.x, b.y}), toQPoint({a.x, a.y}));
        }
        break;

    case ToolType::Select:
    case ToolType::Offset:
    case ToolType::Trim:
    case ToolType::Extend:
    case ToolType::Fillet:
    case ToolType::Chamfer:
        break;
    }
}

}
