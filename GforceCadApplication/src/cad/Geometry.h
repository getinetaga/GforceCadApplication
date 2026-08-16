#pragma once

#include <QPointF>
#include <cmath>
#include <algorithm>

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

inline double clamp(double value, double low, double high)
{
    return std::max(low, std::min(high, value));
}

}
