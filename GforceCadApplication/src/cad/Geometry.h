#pragma once

// Shared geometry utilities used by the CAD engine.
// Contains vector math, transforms, primitive calculations, and helper functions for modeling and editing.
#include <QPointF>
#include <QVector>
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

struct Vec3
{
    double x{0.0};
    double y{0.0};
    double z{0.0};

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
};

enum class BooleanOperationType
{
    Union,
    Difference,
    Intersection,
    XOR
};

struct Transform2D
{
    double rotationRadians{0.0};
    double scaleX{1.0};
    double scaleY{1.0};
    double shearX{0.0};
    double shearY{0.0};
    double translateX{0.0};
    double translateY{0.0};
};

struct Transform3D
{
    double rotationX{0.0};
    double rotationY{0.0};
    double rotationZ{0.0};
    double scaleX{1.0};
    double scaleY{1.0};
    double scaleZ{1.0};
    double translateX{0.0};
    double translateY{0.0};
    double translateZ{0.0};
};

struct MeshTriangle
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
};

struct Mesh
{
    QVector<MeshTriangle> triangles;
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

inline double trianglePerimeter(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return distance(a, b) + distance(b, c) + distance(c, a);
}

inline double triangleArea(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return std::abs(
        (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) / 2.0
    );
}

inline Vec2 transformPoint(const Vec2& p, const Transform2D& transform)
{
    const double cosAngle = std::cos(transform.rotationRadians);
    const double sinAngle = std::sin(transform.rotationRadians);

    const double x1 = p.x * cosAngle - p.y * sinAngle;
    const double y1 = p.x * sinAngle + p.y * cosAngle;

    const double x2 = x1 * transform.scaleX + y1 * transform.shearX;
    const double y2 = x1 * transform.shearY + y1 * transform.scaleY;

    return {x2 + transform.translateX, y2 + transform.translateY};
}

inline Vec3 transformPoint(const Vec3& p, const Transform3D& transform)
{
    const double cx = std::cos(transform.rotationX);
    const double sx = std::sin(transform.rotationX);
    const double cy = std::cos(transform.rotationY);
    const double sy = std::sin(transform.rotationY);
    const double cz = std::cos(transform.rotationZ);
    const double sz = std::sin(transform.rotationZ);

    double x = p.x;
    double y = p.y;
    double z = p.z;

    const double y1 = x * sx + y * cx;
    x = x * cx - y * sx;
    y = y1;

    const double z1 = y * sy + z * cy;
    const double x1 = x * cy - z * sy;
    y = y * cy - z * sy;
    z = z1;

    const double xr = x * cz - y * sz;
    const double yr = x * sz + y * cz;
    x = xr;
    y = yr;

    x *= transform.scaleX;
    y *= transform.scaleY;
    z *= transform.scaleZ;

    return {x + transform.translateX, y + transform.translateY, z + transform.translateZ};
}

inline bool pointInTriangle(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
{
    const auto area = [](const Vec2& v1, const Vec2& v2, const Vec2& v3) {
        return std::abs((v1.x * (v2.y - v3.y) + v2.x * (v3.y - v1.y) + v3.x * (v1.y - v2.y)) / 2.0);
    };

    const double total = area(a, b, c);
    const double p1 = area(p, b, c);
    const double p2 = area(a, p, c);
    const double p3 = area(a, b, p);

    return std::abs(total - (p1 + p2 + p3)) < 1e-6;
}

inline bool segmentsIntersect(const Vec2& a1, const Vec2& a2, const Vec2& b1, const Vec2& b2)
{
    const auto orientation = [](const Vec2& p, const Vec2& q, const Vec2& r) {
        const double val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
        if (std::abs(val) < 1e-6) return 0;
        return val > 0 ? 1 : -1;
    };

    const int o1 = orientation(a1, a2, b1);
    const int o2 = orientation(a1, a2, b2);
    const int o3 = orientation(b1, b2, a1);
    const int o4 = orientation(b1, b2, a2);

    if (o1 == 0 && o2 == 0 && o3 == 0 && o4 == 0) {
        return true;
    }

    return (o1 != o2 && o3 != o4);
}

inline bool circleIntersectsCircle(double radiusA, const Vec2& centerA,
                                  double radiusB, const Vec2& centerB)
{
    return distance(centerA, centerB) <= radiusA + radiusB;
}

inline double polygonArea(const QVector<Vec2>& points)
{
    if (points.size() < 3) return 0.0;

    double area = 0.0;
    for (int i = 0; i < points.size(); ++i) {
        const Vec2& current = points[i];
        const Vec2& next = points[(i + 1) % points.size()];
        area += current.x * next.y - next.x * current.y;
    }

    return std::abs(area) / 2.0;
}

inline QVector<Vec2> applyBooleanOperation(const QVector<Vec2>& source,
                                          const QVector<Vec2>& operand,
                                          BooleanOperationType operation)
{
    Q_UNUSED(source)
    Q_UNUSED(operand)

    switch (operation) {
    case BooleanOperationType::Union:
        return source;
    case BooleanOperationType::Difference:
        return source;
    case BooleanOperationType::Intersection:
        return source;
    case BooleanOperationType::XOR:
        return source;
    }

    return source;
}

inline double clamp(double value, double low, double high)
{
    return std::max(low, std::min(high, value));
}

}
