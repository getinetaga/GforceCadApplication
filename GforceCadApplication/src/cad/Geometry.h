#pragma once

#include <QPointF>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace GForceCAD {

struct Vec2
{
    double x{0.0};
    double y{0.0};

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
};

inline double length(const Vec2& v)
{
    return std::hypot(v.x, v.y);
}

inline double distance(const Vec2& a, const Vec2& b)
{
    return length(a - b);
}

inline QPointF toQPoint(const Vec2& p)
{
    return {p.x, p.y};
}

inline Vec2 snapToGrid(const Vec2& p, double grid)
{
    if (grid <= 0.0) return p;
    return {
        std::round(p.x / grid) * grid,
        std::round(p.y / grid) * grid
    };
}

inline double circleDiameter(double radius)
{
    return 2.0 * radius;
}

inline double circleCircumference(double radius)
{
    return 2.0 * std::numbers::pi * radius;
}

inline double circleArea(double radius)
{
    return std::numbers::pi * radius * radius;
}

inline double ellipseMajorDiameter(double semiMajorAxis)
{
    return 2.0 * semiMajorAxis;
}

inline double ellipseMinorDiameter(double semiMinorAxis)
{
    return 2.0 * semiMinorAxis;
}

inline double ellipseArea(double semiMajorAxis, double semiMinorAxis)
{
    return std::numbers::pi * semiMajorAxis * semiMinorAxis;
}

inline double ellipsePerimeterApprox(double semiMajorAxis, double semiMinorAxis)
{
    const double a = semiMajorAxis;
    const double b = semiMinorAxis;
    const double meanSquares = (a * a + b * b) / 2.0;
    return 2.0 * std::numbers::pi * std::sqrt(meanSquares);
}

inline double clamp(double value, double low, double high)
{
    return std::max(low, std::min(high, value));
}

}
