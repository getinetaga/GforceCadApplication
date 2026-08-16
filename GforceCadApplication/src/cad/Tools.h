#pragma once

#include "Document.h"
#include <memory>

namespace GForceCAD {

enum class ToolType
{
    Select,
    Line,
    Circle,
    Arc,
    Polyline,
    Rectangle,
    Offset,
    Trim,
    Extend,
    Fillet,
    Chamfer
};

class ToolController
{
public:
    struct LinePreview
    {
        bool valid{false};
        Vec2 a{};
        Vec2 b{};
    };

    explicit ToolController(Document& document);

    void setTool(ToolType type);
    ToolType tool() const { return m_tool; }

    void cancel();

    bool click(const Vec2& point);
    void move(const Vec2& point);

    void setOffsetDistance(double value);
    void setFilletRadius(double value);
    void setChamferDistances(double first, double second);

    double offsetDistance() const { return m_offsetDistance; }
    double filletRadius() const { return m_filletRadius; }
    double chamferDistance1() const { return m_chamferDistance1; }
    double chamferDistance2() const { return m_chamferDistance2; }

    const QVector<Vec2>& points() const { return m_points; }
    Vec2 previewPoint() const { return m_previewPoint; }
    LinePreview primaryPreview() const { return m_primaryPreview; }
    LinePreview hoverPreview() const { return m_hoverPreview; }
    bool hasIntersectionPreview() const { return m_hasIntersectionPreview; }
    Vec2 intersectionPreview() const { return m_intersectionPreview; }

    QString prompt() const;

private:
    enum class EditEdgeKind
    {
        None,
        Line,
        PolylineSegment,
        Arc
    };

    Document& m_document;
    ToolType m_tool{ToolType::Select};

    QVector<Vec2> m_points;
    Vec2 m_previewPoint{};

    LinePreview m_primaryPreview{};
    LinePreview m_hoverPreview{};
    bool m_hasIntersectionPreview{false};
    Vec2 m_intersectionPreview{};

    std::shared_ptr<Entity> m_primaryEntity;
    EditEdgeKind m_primaryEdgeKind{EditEdgeKind::None};
    int m_primaryIndexA{-1};
    int m_primaryIndexB{-1};
    Vec2 m_primaryPick{};

    double m_offsetDistance{10.0};
    double m_filletRadius{10.0};
    double m_chamferDistance1{10.0};
    double m_chamferDistance2{10.0};
};

}
