#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPointF>
#include <optional>
#include "../cad/Tools.h"

namespace GForceCAD {

class CadViewport final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CadViewport(Document& document, ToolController& tools,
                         QWidget* parent = nullptr);

    Vec2 screenToWorld(const QPointF& screen) const;
    QPointF worldToScreen(const Vec2& world) const;

    bool gridEnabled() const { return m_grid; }
    bool snapEnabled() const { return m_snap; }
    bool objectSnapEnabled() const { return m_objectSnap; }

public slots:
    void zoomExtents();

    void setGridEnabled(bool enabled);
    void setSnapEnabled(bool enabled);
    void setObjectSnapEnabled(bool enabled);

signals:
    void cursorChanged(const Vec2& point);
    void selectionChanged();
    void viewportChanged();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    Document& m_document;
    ToolController& m_tools;

    double m_zoom{1.0};
    QPointF m_pan{0.0, 0.0};
    QPointF m_lastMouse;
    bool m_panning{false};

    bool m_grid{true};
    bool m_snap{true};
    bool m_objectSnap{true};
    bool m_hasSnapCandidate{false};
    Vec2 m_snapCandidate{};

    Vec2 snap(const Vec2& point) const;
    std::optional<Vec2> nearestObjectSnap(const Vec2& point) const;
    void drawPreview(QPainter& painter) const;
};

}
