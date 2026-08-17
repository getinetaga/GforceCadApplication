#include "Entities.h"
#include <QPen>
#include <QPainterPath>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
#include <numbers>

namespace GForceCAD {

static QPen entityPen(bool selected, double scale)
{
    QPen pen(selected ? QColor("#00d7ff") : QColor("#e8edf2"));
    pen.setWidthF(std::max(1.0, 1.6 / std::max(scale, 0.01)));
    return pen;
}

static Vec2 scalePoint(const Vec2& point, double factor, const Vec2& origin)
{
    return origin + (point - origin) * factor;
}

static double pointSegmentDistance(const Vec2& p, const Vec2& a, const Vec2& b)
{
    const Vec2 ab = b - a;
    const double denom = ab.x * ab.x + ab.y * ab.y;
    if (denom < 1e-12) return distance(p, a);

    double t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom;
    t = clamp(t, 0.0, 1.0);

    const Vec2 q{a.x + t * ab.x, a.y + t * ab.y};
    return distance(p, q);
}

LineEntity::LineEntity(int id, Vec2 a, Vec2 b, const QString& layer)
    : Entity(id, EntityType::Line, layer), m_a(a), m_b(b) {}

void LineEntity::draw(QPainter& painter, double scale) const
{
    painter.setPen(entityPen(selected(), scale));
    painter.drawLine(toQPoint(m_a), toQPoint(m_b));
}

bool LineEntity::hitTest(const Vec2& world, double tolerance) const
{
    return pointSegmentDistance(world, m_a, m_b) <= tolerance;
}

QVector<Vec2> LineEntity::snapPoints() const
{
    return {
        m_a,
        m_b,
        {(m_a.x + m_b.x) * 0.5, (m_a.y + m_b.y) * 0.5}
    };
}

QJsonObject LineEntity::toJson() const
{
    return {
        {"type", "LINE"}, {"id", id()}, {"layer", layer()},
        {"x1", m_a.x}, {"y1", m_a.y}, {"x2", m_b.x}, {"y2", m_b.y}
    };
}

void LineEntity::moveBy(const Vec2& delta)
{
    m_a = m_a + delta;
    m_b = m_b + delta;
}

void LineEntity::scaleBy(double factor, const Vec2& origin)
{
    m_a = scalePoint(m_a, factor, origin);
    m_b = scalePoint(m_b, factor, origin);
}

QString LineEntity::properties() const
{
    return QString("LINE\nID: %1\nLayer: %2\nStart: (%3, %4)\nEnd: (%5, %6)\nLength: %7")
        .arg(id()).arg(layer())
        .arg(m_a.x, 0, 'f', 3).arg(m_a.y, 0, 'f', 3)
        .arg(m_b.x, 0, 'f', 3).arg(m_b.y, 0, 'f', 3)
        .arg(distance(m_a, m_b), 0, 'f', 3);
}

CircleEntity::CircleEntity(int id, Vec2 center, double radius, const QString& layer)
    : Entity(id, EntityType::Circle, layer), m_center(center), m_radius(radius) {}

void CircleEntity::draw(QPainter& painter, double scale) const
{
    painter.setPen(entityPen(selected(), scale));
    painter.drawEllipse(toQPoint(m_center), m_radius, m_radius);
}

bool CircleEntity::hitTest(const Vec2& world, double tolerance) const
{
    return std::abs(distance(world, m_center) - m_radius) <= tolerance;
}

QVector<Vec2> CircleEntity::snapPoints() const
{
    return {m_center};
}

QJsonObject CircleEntity::toJson() const
{
    return {
        {"type", "CIRCLE"}, {"id", id()}, {"layer", layer()},
        {"cx", m_center.x}, {"cy", m_center.y}, {"radius", m_radius}
    };
}

void CircleEntity::moveBy(const Vec2& delta)
{
    m_center = m_center + delta;
}

void CircleEntity::scaleBy(double factor, const Vec2& origin)
{
    m_center = scalePoint(m_center, factor, origin);
    m_radius *= std::abs(factor);
}

QString CircleEntity::properties() const
{
    return QString("CIRCLE\nID: %1\nLayer: %2\nCenter: (%3, %4)\nRadius: %5\nDiameter: %6\nCircumference: %7\nArea: %8")
        .arg(id()).arg(layer())
        .arg(m_center.x, 0, 'f', 3).arg(m_center.y, 0, 'f', 3)
        .arg(m_radius, 0, 'f', 3)
        .arg(circleDiameter(m_radius), 0, 'f', 3)
        .arg(circleCircumference(m_radius), 0, 'f', 3)
        .arg(circleArea(m_radius), 0, 'f', 3);
}

EllipseEntity::EllipseEntity(int id, Vec2 center, double semiMajor, double semiMinor,
                           const QString& layer)
    : Entity(id, EntityType::Ellipse, layer),
      m_center(center),
      m_semiMajor(semiMajor),
      m_semiMinor(semiMinor)
{}

void EllipseEntity::draw(QPainter& painter, double scale) const
{
    painter.setPen(entityPen(selected(), scale));
    painter.drawEllipse(toQPoint(m_center), m_semiMajor, m_semiMinor);
}

bool EllipseEntity::hitTest(const Vec2& world, double tolerance) const
{
    const double dx = world.x - m_center.x;
    const double dy = world.y - m_center.y;
    const double normalized = (dx * dx) / (m_semiMajor * m_semiMajor)
                           + (dy * dy) / (m_semiMinor * m_semiMinor);
    return std::abs(normalized - 1.0) <= tolerance / std::max(m_semiMajor, m_semiMinor);
}

QVector<Vec2> EllipseEntity::snapPoints() const
{
    return {m_center};
}

QJsonObject EllipseEntity::toJson() const
{
    return {
        {"type", "ELLIPSE"}, {"id", id()}, {"layer", layer()},
        {"cx", m_center.x}, {"cy", m_center.y},
        {"semiMajor", m_semiMajor}, {"semiMinor", m_semiMinor}
    };
}

void EllipseEntity::moveBy(const Vec2& delta)
{
    m_center = m_center + delta;
}

void EllipseEntity::scaleBy(double factor, const Vec2& origin)
{
    m_center = scalePoint(m_center, factor, origin);
    m_semiMajor *= std::abs(factor);
    m_semiMinor *= std::abs(factor);
}

QString EllipseEntity::properties() const
{
    return QString("ELLIPSE\nID: %1\nLayer: %2\nCenter: (%3, %4)\nSemi-major: %5\nSemi-minor: %6\nMajor diameter: %7\nMinor diameter: %8\nArea: %9\nPerimeter approx.: %10")
        .arg(id()).arg(layer())
        .arg(m_center.x, 0, 'f', 3).arg(m_center.y, 0, 'f', 3)
        .arg(m_semiMajor, 0, 'f', 3)
        .arg(m_semiMinor, 0, 'f', 3)
        .arg(ellipseMajorDiameter(m_semiMajor), 0, 'f', 3)
        .arg(ellipseMinorDiameter(m_semiMinor), 0, 'f', 3)
        .arg(ellipseArea(m_semiMajor, m_semiMinor), 0, 'f', 3)
        .arg(ellipsePerimeterApprox(m_semiMajor, m_semiMinor), 0, 'f', 3);
}

ArcEntity::ArcEntity(int id, Vec2 center, double radius, double startDeg, double endDeg,
                     const QString& layer)
    : Entity(id, EntityType::Arc, layer),
      m_center(center), m_radius(radius),
      m_startDeg(startDeg), m_endDeg(endDeg) {}

void ArcEntity::draw(QPainter& painter, double scale) const
{
    painter.setPen(entityPen(selected(), scale));

    QRectF r(
        m_center.x - m_radius,
        m_center.y - m_radius,
        2.0 * m_radius,
        2.0 * m_radius
    );

    painter.drawArc(
        r,
        static_cast<int>(m_startDeg * 16.0),
        static_cast<int>((m_endDeg - m_startDeg) * 16.0)
    );
}

bool ArcEntity::hitTest(const Vec2& world, double tolerance) const
{
    return std::abs(distance(world, m_center) - m_radius) <= tolerance;
}

QVector<Vec2> ArcEntity::snapPoints() const
{
    const double startRad = m_startDeg * std::numbers::pi / 180.0;
    const double endRad = m_endDeg * std::numbers::pi / 180.0;

    const Vec2 start{m_center.x + std::cos(startRad) * m_radius,
                     m_center.y + std::sin(startRad) * m_radius};
    const Vec2 end{m_center.x + std::cos(endRad) * m_radius,
                   m_center.y + std::sin(endRad) * m_radius};

    return {m_center, start, end};
}

QJsonObject ArcEntity::toJson() const
{
    return {
        {"type", "ARC"}, {"id", id()}, {"layer", layer()},
        {"cx", m_center.x}, {"cy", m_center.y},
        {"radius", m_radius},
        {"start", m_startDeg}, {"end", m_endDeg}
    };
}

void ArcEntity::moveBy(const Vec2& delta)
{
    m_center = m_center + delta;
}

void ArcEntity::scaleBy(double factor, const Vec2& origin)
{
    m_center = scalePoint(m_center, factor, origin);
    m_radius *= std::abs(factor);
}

QString ArcEntity::properties() const
{
    return QString("ARC\nID: %1\nLayer: %2\nCenter: (%3, %4)\nRadius: %5\nStart: %6°\nEnd: %7°")
        .arg(id()).arg(layer())
        .arg(m_center.x, 0, 'f', 3).arg(m_center.y, 0, 'f', 3)
        .arg(m_radius, 0, 'f', 3)
        .arg(m_startDeg, 0, 'f', 2).arg(m_endDeg, 0, 'f', 2);
}

TriangleEntity::TriangleEntity(int id, const QVector<Vec2>& points,
                             const QString& layer)
    : Entity(id, EntityType::Triangle, layer), m_points(points) {}

void TriangleEntity::draw(QPainter& painter, double scale) const
{
    if (m_points.size() < 3) return;

    painter.setPen(entityPen(selected(), scale));

    QPainterPath path;
    path.moveTo(toQPoint(m_points[0]));
    path.lineTo(toQPoint(m_points[1]));
    path.lineTo(toQPoint(m_points[2]));
    path.closeSubpath();
    painter.drawPath(path);
}

bool TriangleEntity::hitTest(const Vec2& world, double tolerance) const
{
    if (m_points.size() < 3) return false;

    for (int i = 1; i < m_points.size(); ++i) {
        if (pointSegmentDistance(world, m_points[i - 1], m_points[i]) <= tolerance)
            return true;
    }

    return pointSegmentDistance(world, m_points.last(), m_points.first()) <= tolerance;
}

QVector<Vec2> TriangleEntity::snapPoints() const
{
    QVector<Vec2> points = m_points;

    for (int i = 1; i < m_points.size(); ++i) {
        const Vec2& a = m_points[i - 1];
        const Vec2& b = m_points[i];
        points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
    }

    const Vec2& a = m_points.last();
    const Vec2& b = m_points.first();
    points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});

    return points;
}

QJsonObject TriangleEntity::toJson() const
{
    QJsonArray points;

    for (const Vec2& p : m_points)
        points.append(QJsonObject{{"x", p.x}, {"y", p.y}});

    return {
        {"type", "TRIANGLE"},
        {"id", id()},
        {"layer", layer()},
        {"points", points}
    };
}

void TriangleEntity::moveBy(const Vec2& delta)
{
    for (Vec2& p : m_points)
        p = p + delta;
}

void TriangleEntity::scaleBy(double factor, const Vec2& origin)
{
    for (Vec2& p : m_points)
        p = scalePoint(p, factor, origin);
}

QString TriangleEntity::properties() const
{
    if (m_points.size() < 3) {
        return QString("TRIANGLE\nID: %1\nLayer: %2\nIncomplete triangle").arg(id()).arg(layer());
    }

    const double side1 = distance(m_points[0], m_points[1]);
    const double side2 = distance(m_points[1], m_points[2]);
    const double side3 = distance(m_points[2], m_points[0]);
    const double perimeter = trianglePerimeter(m_points[0], m_points[1], m_points[2]);
    const double area = triangleArea(m_points[0], m_points[1], m_points[2]);

    return QString("TRIANGLE\nID: %1\nLayer: %2\nVertex A: (%3, %4)\nVertex B: (%5, %6)\nVertex C: (%7, %8)\nSide AB: %9\nSide BC: %10\nSide CA: %11\nPerimeter: %12\nArea: %13")
        .arg(id()).arg(layer())
        .arg(m_points[0].x, 0, 'f', 3).arg(m_points[0].y, 0, 'f', 3)
        .arg(m_points[1].x, 0, 'f', 3).arg(m_points[1].y, 0, 'f', 3)
        .arg(m_points[2].x, 0, 'f', 3).arg(m_points[2].y, 0, 'f', 3)
        .arg(side1, 0, 'f', 3)
        .arg(side2, 0, 'f', 3)
        .arg(side3, 0, 'f', 3)
        .arg(perimeter, 0, 'f', 3)
        .arg(area, 0, 'f', 3);
}

PolygonEntity::PolygonEntity(int id, const QVector<Vec2>& points,
                           const QString& layer)
    : Entity(id, EntityType::Polygon, layer), m_points(points) {}

void PolygonEntity::draw(QPainter& painter, double scale) const
{
    if (m_points.size() < 3) return;

    painter.setPen(entityPen(selected(), scale));

    QPainterPath path;
    path.moveTo(toQPoint(m_points.first()));

    for (int i = 1; i < m_points.size(); ++i)
        path.lineTo(toQPoint(m_points[i]));

    path.closeSubpath();
    painter.drawPath(path);
}

bool PolygonEntity::hitTest(const Vec2& world, double tolerance) const
{
    for (int i = 1; i < m_points.size(); ++i) {
        if (pointSegmentDistance(world, m_points[i - 1], m_points[i]) <= tolerance)
            return true;
    }

    if (m_points.size() > 2 &&
        pointSegmentDistance(world, m_points.last(), m_points.first()) <= tolerance)
        return true;

    return false;
}

QVector<Vec2> PolygonEntity::snapPoints() const
{
    QVector<Vec2> points = m_points;

    for (int i = 1; i < m_points.size(); ++i) {
        const Vec2& a = m_points[i - 1];
        const Vec2& b = m_points[i];
        points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
    }

    if (m_points.size() > 2) {
        const Vec2& a = m_points.last();
        const Vec2& b = m_points.first();
        points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
    }

    return points;
}

QJsonObject PolygonEntity::toJson() const
{
    QJsonArray points;

    for (const Vec2& p : m_points)
        points.append(QJsonObject{{"x", p.x}, {"y", p.y}});

    return {
        {"type", "POLYGON"},
        {"id", id()},
        {"layer", layer()},
        {"points", points}
    };
}

void PolygonEntity::moveBy(const Vec2& delta)
{
    for (Vec2& p : m_points)
        p = p + delta;
}

void PolygonEntity::scaleBy(double factor, const Vec2& origin)
{
    for (Vec2& p : m_points)
        p = scalePoint(p, factor, origin);
}

QString PolygonEntity::properties() const
{
    if (m_points.size() < 3) {
        return QString("POLYGON\nID: %1\nLayer: %2\nVertices: %3\nIncomplete polygon")
            .arg(id()).arg(layer()).arg(m_points.size());
    }

    double length = 0.0;
    double minX = m_points.first().x;
    double maxX = minX;
    double minY = m_points.first().y;
    double maxY = minY;

    for (int i = 0; i < m_points.size(); ++i) {
        const Vec2& current = m_points[i];
        const Vec2& next = m_points[(i + 1) % m_points.size()];
        length += distance(current, next);
        minX = std::min(minX, current.x);
        maxX = std::max(maxX, current.x);
        minY = std::min(minY, current.y);
        maxY = std::max(maxY, current.y);
    }

    const double width = maxX - minX;
    const double height = maxY - minY;
    const double area = polygonArea(m_points);

    return QString("POLYGON\nID: %1\nLayer: %2\nVertices: %3\nLength: %4\nArea: %5\nWidth: %6\nHeight: %7\nPerimeter: %8")
        .arg(id()).arg(layer()).arg(m_points.size())
        .arg(length, 0, 'f', 3)
        .arg(area, 0, 'f', 3)
        .arg(width, 0, 'f', 3)
        .arg(height, 0, 'f', 3)
        .arg(length, 0, 'f', 3);
}

PolylineEntity::PolylineEntity(int id, const QVector<Vec2>& points,
                               const QString& layer, bool closed)
    : Entity(id, closed ? EntityType::Rectangle : EntityType::Polyline, layer),
      m_points(points), m_closed(closed) {}

void PolylineEntity::draw(QPainter& painter, double scale) const
{
    if (m_points.size() < 2) return;

    painter.setPen(entityPen(selected(), scale));

    QPainterPath path;
    path.moveTo(toQPoint(m_points.first()));

    for (int i = 1; i < m_points.size(); ++i)
        path.lineTo(toQPoint(m_points[i]));

    if (m_closed)
        path.closeSubpath();

    painter.drawPath(path);
}

bool PolylineEntity::hitTest(const Vec2& world, double tolerance) const
{
    for (int i = 1; i < m_points.size(); ++i) {
        if (pointSegmentDistance(world, m_points[i - 1], m_points[i]) <= tolerance)
            return true;
    }

    if (m_closed && m_points.size() > 2) {
        if (pointSegmentDistance(world, m_points.last(), m_points.first()) <= tolerance)
            return true;
    }

    return false;
}

QVector<Vec2> PolylineEntity::snapPoints() const
{
    QVector<Vec2> points = m_points;

    for (int i = 1; i < m_points.size(); ++i) {
        const Vec2& a = m_points[i - 1];
        const Vec2& b = m_points[i];
        points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
    }

    if (m_closed && m_points.size() > 2) {
        const Vec2& a = m_points.last();
        const Vec2& b = m_points.first();
        points.append({(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
    }

    return points;
}

QJsonObject PolylineEntity::toJson() const
{
    QJsonArray points;

    for (const Vec2& p : m_points)
        points.append(QJsonObject{{"x", p.x}, {"y", p.y}});

    return {
        {"type", m_closed ? "RECTANGLE" : "POLYLINE"},
        {"id", id()},
        {"layer", layer()},
        {"points", points}
    };
}

void PolylineEntity::moveBy(const Vec2& delta)
{
    for (Vec2& p : m_points)
        p = p + delta;
}

void PolylineEntity::scaleBy(double factor, const Vec2& origin)
{
    for (Vec2& p : m_points)
        p = scalePoint(p, factor, origin);
}

QString PolylineEntity::properties() const
{
    return QString("%1\nID: %2\nLayer: %3\nVertices: %4")
        .arg(m_closed ? "RECTANGLE" : "POLYLINE")
        .arg(id()).arg(layer()).arg(m_points.size());
}

}
