#pragma once

#include "Entity.h"
#include <QVector>

namespace GForceCAD {

class LineEntity final : public Entity
{
public:
    LineEntity(int id, Vec2 a, Vec2 b, const QString& layer = "0");

    Vec2 a() const { return m_a; }
    Vec2 b() const { return m_b; }
    void setEndpoints(const Vec2& a, const Vec2& b) { m_a = a; m_b = b; }

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    Vec2 m_a, m_b;
};

class CircleEntity final : public Entity
{
public:
    CircleEntity(int id, Vec2 center, double radius, const QString& layer = "0");

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    Vec2 m_center;
    double m_radius;
};

class EllipseEntity final : public Entity
{
public:
    EllipseEntity(int id, Vec2 center, double semiMajor, double semiMinor,
                 const QString& layer = "0");

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    Vec2 m_center;
    double m_semiMajor;
    double m_semiMinor;
};

class ArcEntity final : public Entity
{
public:
    ArcEntity(int id, Vec2 center, double radius, double startDeg, double endDeg,
              const QString& layer = "0");

    Vec2 center() const { return m_center; }
    double radius() const { return m_radius; }
    double startDeg() const { return m_startDeg; }
    double endDeg() const { return m_endDeg; }
    void setAngles(double startDeg, double endDeg)
    {
        m_startDeg = startDeg;
        m_endDeg = endDeg;
    }

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    Vec2 m_center;
    double m_radius;
    double m_startDeg;
    double m_endDeg;
};

class TriangleEntity final : public Entity
{
public:
    TriangleEntity(int id, const QVector<Vec2>& points,
                   const QString& layer = "0");

    const QVector<Vec2>& points() const { return m_points; }
    void setPoint(int index, const Vec2& value)
    {
        if (index >= 0 && index < m_points.size()) m_points[index] = value;
    }
    void setPoints(const QVector<Vec2>& points)
    {
        m_points = points;
    }

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    QVector<Vec2> m_points;
};

class PolygonEntity final : public Entity
{
public:
    PolygonEntity(int id, const QVector<Vec2>& points,
                  const QString& layer = "0");

    const QVector<Vec2>& points() const { return m_points; }
    void setPoint(int index, const Vec2& value)
    {
        if (index >= 0 && index < m_points.size()) m_points[index] = value;
    }
    void setPoints(const QVector<Vec2>& points)
    {
        m_points = points;
    }

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    QVector<Vec2> m_points;
};

class PolylineEntity final : public Entity
{
public:
    PolylineEntity(int id, const QVector<Vec2>& points,
                   const QString& layer = "0", bool closed = false);

    const QVector<Vec2>& points() const { return m_points; }
    bool closed() const { return m_closed; }
    void setPoint(int index, const Vec2& value)
    {
        if (index >= 0 && index < m_points.size()) m_points[index] = value;
    }
    void setPoints(const QVector<Vec2>& points)
    {
        m_points = points;
    }

    void draw(QPainter& painter, double scale) const override;
    bool hitTest(const Vec2& world, double tolerance) const override;
    QVector<Vec2> snapPoints() const override;
    QJsonObject toJson() const override;
    void moveBy(const Vec2& delta) override;
    QString properties() const override;

private:
    QVector<Vec2> m_points;
    bool m_closed{false};
};

}
