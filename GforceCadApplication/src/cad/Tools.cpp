#include "Tools.h"
#include "Entities.h"
#include <cmath>
#include <numbers>
#include <optional>
#include <limits>

namespace {

using namespace GForceCAD;

enum class EdgeKind
{
    None,
    Line,
    PolylineSegment,
    Arc
};

struct EdgeRef
{
    std::shared_ptr<Entity> entity;
    EdgeKind kind{EdgeKind::None};
    int indexA{-1};
    int indexB{-1};
    Vec2 a{};
    Vec2 b{};
    Vec2 center{};
    double radius{0.0};
    double startDeg{0.0};
    double endDeg{0.0};

    bool valid() const { return entity != nullptr && kind != EdgeKind::None; }
};

double cross(const Vec2& a, const Vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

double dot(const Vec2& a, const Vec2& b)
{
    return a.x * b.x + a.y * b.y;
}

bool lineIntersection(
    const Vec2& a1,
    const Vec2& a2,
    const Vec2& b1,
    const Vec2& b2,
    Vec2* out,
    double* tA = nullptr,
    double* tB = nullptr)
{
    const Vec2 r = a2 - a1;
    const Vec2 s = b2 - b1;
    const double denom = cross(r, s);

    if (std::abs(denom) < 1e-9)
        return false;

    const Vec2 delta = b1 - a1;
    const double ta = cross(delta, s) / denom;
    const double tb = cross(delta, r) / denom;

    if (out) *out = a1 + r * ta;
    if (tA) *tA = ta;
    if (tB) *tB = tb;
    return true;
}

Vec2 normalize(const Vec2& v)
{
    const double len = length(v);
    if (len < 1e-9) return {0.0, 0.0};
    return {v.x / len, v.y / len};
}

double radians(double deg)
{
    return deg * std::numbers::pi / 180.0;
}

double degrees(double rad)
{
    return rad * 180.0 / std::numbers::pi;
}

double normalizeAngle(double deg)
{
    while (deg < 0.0) deg += 360.0;
    while (deg >= 360.0) deg -= 360.0;
    return deg;
}

double positiveDelta(double from, double to)
{
    const double f = normalizeAngle(from);
    const double t = normalizeAngle(to);
    double d = t - f;
    if (d < 0.0) d += 360.0;
    return d;
}

bool angleOnArc(double angleDeg, double startDeg, double endDeg);

bool tangentPointOnArc(const Vec2& point, const Vec2& center,
                       double startDeg, double endDeg)
{
    const double angle = normalizeAngle(degrees(std::atan2(
        point.y - center.y, point.x - center.x
    )));
    return angleOnArc(angle, startDeg, endDeg);
}

QVector<Vec2> tangentPoints(const Vec2& external, const Vec2& center, double radius)
{
    QVector<Vec2> points;
    const Vec2 offset = external - center;
    const double distanceSquared = dot(offset, offset);
    const double radiusSquared = radius * radius;

    if (distanceSquared <= radiusSquared + 1e-9)
        return points;

    const double scale = radiusSquared / distanceSquared;
    const double tangentScale = radius * std::sqrt(distanceSquared - radiusSquared) / distanceSquared;
    const Vec2 base = center + offset * scale;
    const Vec2 perpendicular{-offset.y, offset.x};

    points.append(base + perpendicular * tangentScale);
    points.append(base - perpendicular * tangentScale);
    return points;
}

bool angleOnArc(double angleDeg, double startDeg, double endDeg)
{
    const double span = positiveDelta(startDeg, endDeg);
    const double rel = positiveDelta(startDeg, angleDeg);
    return rel <= span + 1e-6;
}

double pointSegmentDistance(const Vec2& p, const Vec2& a, const Vec2& b)
{
    const Vec2 ab = b - a;
    const double denom = ab.x * ab.x + ab.y * ab.y;
    if (denom < 1e-12) return distance(p, a);

    double t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / denom;
    t = clamp(t, 0.0, 1.0);

    const Vec2 q{a.x + t * ab.x, a.y + t * ab.y};
    return distance(p, q);
}

double pointArcDistance(const Vec2& p, const EdgeRef& edge)
{
    const double radial = std::abs(distance(p, edge.center) - edge.radius);
    const double angle = normalizeAngle(degrees(std::atan2(p.y - edge.center.y, p.x - edge.center.x)));
    if (angleOnArc(angle, edge.startDeg, edge.endDeg))
        return radial;

    return std::min(distance(p, edge.a), distance(p, edge.b));
}

bool pointOnEdge(const EdgeRef& edge, const Vec2& point, double tolerance)
{
    if (edge.kind == EdgeKind::Arc)
        return pointArcDistance(point, edge) <= tolerance;

    return pointSegmentDistance(point, edge.a, edge.b) <= tolerance;
}

std::optional<EdgeRef> findEdgeAt(Document& document, const Vec2& point, double tolerance)
{
    double best = std::numeric_limits<double>::max();
    std::optional<EdgeRef> result;

    for (auto it = document.entities().rbegin(); it != document.entities().rend(); ++it) {
        const auto& entity = *it;
        const Layer* layer = document.getLayer(entity->layer());
        if (layer && !layer->visible) continue;

        if (auto line = std::dynamic_pointer_cast<LineEntity>(entity)) {
            const double d = pointSegmentDistance(point, line->a(), line->b());
            if (d <= tolerance && d < best) {
                best = d;
                result = EdgeRef{entity, EdgeKind::Line, -1, -1, line->a(), line->b()};
            }
            continue;
        }

        if (auto poly = std::dynamic_pointer_cast<PolylineEntity>(entity)) {
            const QVector<Vec2>& pts = poly->points();
            if (pts.size() < 2) continue;

            for (int i = 1; i < pts.size(); ++i) {
                const double d = pointSegmentDistance(point, pts[i - 1], pts[i]);
                if (d <= tolerance && d < best) {
                    best = d;
                    result = EdgeRef{entity, EdgeKind::PolylineSegment, i - 1, i, pts[i - 1], pts[i]};
                }
            }

            if (poly->closed() && pts.size() > 2) {
                const int last = pts.size() - 1;
                const double d = pointSegmentDistance(point, pts[last], pts[0]);
                if (d <= tolerance && d < best) {
                    best = d;
                    result = EdgeRef{entity, EdgeKind::PolylineSegment, last, 0, pts[last], pts[0]};
                }
            }
            continue;
        }

        if (auto arc = std::dynamic_pointer_cast<ArcEntity>(entity)) {
            const double r = arc->radius();
            const Vec2 c = arc->center();
            const double s = arc->startDeg();
            const double e = arc->endDeg();

            EdgeRef edge;
            edge.entity = entity;
            edge.kind = EdgeKind::Arc;
            edge.center = c;
            edge.radius = r;
            edge.startDeg = s;
            edge.endDeg = e;
            edge.a = {c.x + std::cos(radians(s)) * r, c.y + std::sin(radians(s)) * r};
            edge.b = {c.x + std::cos(radians(e)) * r, c.y + std::sin(radians(e)) * r};

            const double d = pointArcDistance(point, edge);
            if (d <= tolerance && d < best) {
                best = d;
                result = edge;
            }
            continue;
        }
    }

    return result;
}

QVector<Vec2> lineCircleIntersections(const Vec2& a, const Vec2& b, const Vec2& c, double r)
{
    QVector<Vec2> out;
    const Vec2 d = b - a;
    const Vec2 f = a - c;

    const double A = dot(d, d);
    const double B = 2.0 * dot(f, d);
    const double C = dot(f, f) - r * r;
    const double disc = B * B - 4.0 * A * C;

    if (disc < -1e-9 || A < 1e-12)
        return out;

    const double sqrtDisc = std::sqrt(std::max(0.0, disc));
    const double t1 = (-B - sqrtDisc) / (2.0 * A);
    const double t2 = (-B + sqrtDisc) / (2.0 * A);
    out.append(a + d * t1);
    if (std::abs(t2 - t1) > 1e-9)
        out.append(a + d * t2);

    return out;
}

QVector<Vec2> circleCircleIntersections(const Vec2& c0, double r0, const Vec2& c1, double r1)
{
    QVector<Vec2> out;
    const double d = distance(c0, c1);
    if (d < 1e-9 || d > r0 + r1 + 1e-9 || d < std::abs(r0 - r1) - 1e-9)
        return out;

    const double a = (r0 * r0 - r1 * r1 + d * d) / (2.0 * d);
    const double h2 = r0 * r0 - a * a;
    if (h2 < -1e-9)
        return out;

    const double h = std::sqrt(std::max(0.0, h2));
    const Vec2 dir = normalize(c1 - c0);
    const Vec2 p{c0.x + dir.x * a, c0.y + dir.y * a};
    const Vec2 perp{-dir.y, dir.x};

    out.append({p.x + perp.x * h, p.y + perp.y * h});
    if (h > 1e-9)
        out.append({p.x - perp.x * h, p.y - perp.y * h});
    return out;
}

QVector<Vec2> intersectionCandidates(const EdgeRef& first, const EdgeRef& second, bool requireOnBoth)
{
    QVector<Vec2> candidates;
    const double tol = 1e-3;

    if (first.kind != EdgeKind::Arc && second.kind != EdgeKind::Arc) {
        Vec2 intersection;
        if (lineIntersection(first.a, first.b, second.a, second.b, &intersection)) {
            if (!requireOnBoth || (pointOnEdge(first, intersection, tol) && pointOnEdge(second, intersection, tol)))
                candidates.append(intersection);
        }
        return candidates;
    }

    if (first.kind == EdgeKind::Arc && second.kind != EdgeKind::Arc) {
        auto points = lineCircleIntersections(second.a, second.b, first.center, first.radius);
        for (const Vec2& p : points) {
            const double angle = normalizeAngle(degrees(std::atan2(p.y - first.center.y, p.x - first.center.x)));
            if (!angleOnArc(angle, first.startDeg, first.endDeg)) continue;
            if (!requireOnBoth || pointOnEdge(second, p, tol)) candidates.append(p);
        }
        return candidates;
    }

    if (first.kind != EdgeKind::Arc && second.kind == EdgeKind::Arc) {
        auto points = lineCircleIntersections(first.a, first.b, second.center, second.radius);
        for (const Vec2& p : points) {
            const double angle = normalizeAngle(degrees(std::atan2(p.y - second.center.y, p.x - second.center.x)));
            if (!angleOnArc(angle, second.startDeg, second.endDeg)) continue;
            if (!requireOnBoth || pointOnEdge(first, p, tol)) candidates.append(p);
        }
        return candidates;
    }

    auto points = circleCircleIntersections(first.center, first.radius, second.center, second.radius);
    for (const Vec2& p : points) {
        const double a1 = normalizeAngle(degrees(std::atan2(p.y - first.center.y, p.x - first.center.x)));
        const double a2 = normalizeAngle(degrees(std::atan2(p.y - second.center.y, p.x - second.center.x)));
        if (!angleOnArc(a1, first.startDeg, first.endDeg)) continue;
        if (!angleOnArc(a2, second.startDeg, second.endDeg)) continue;
        candidates.append(p);
    }

    return candidates;
}

Vec2 closestPoint(const QVector<Vec2>& points, const Vec2& pivot)
{
    Vec2 best{};
    double bestDistance = std::numeric_limits<double>::max();
    for (const Vec2& p : points) {
        const double d = distance(p, pivot);
        if (d < bestDistance) {
            bestDistance = d;
            best = p;
        }
    }
    return best;
}

Vec2 edgeEndpointNearPick(const EdgeRef& edge, const Vec2& pick)
{
    return distance(pick, edge.a) <= distance(pick, edge.b) ? edge.a : edge.b;
}

bool pickPrefersEndpointA(const EdgeRef& edge, const Vec2& pick)
{
    return distance(pick, edge.a) <= distance(pick, edge.b);
}

bool edgeDirectionFromCorner(const EdgeRef& edge, const Vec2& pick, const Vec2& corner, Vec2* out)
{
    if (!out) return false;

    if (edge.kind == EdgeKind::Line || edge.kind == EdgeKind::PolylineSegment) {
        const Vec2 endpoint = edgeEndpointNearPick(edge, pick);
        const Vec2 far = distance(endpoint, edge.a) < distance(endpoint, edge.b) ? edge.b : edge.a;
        *out = normalize(far - corner);
        return length(*out) > 1e-9;
    }

    if (edge.kind == EdgeKind::Arc) {
        const bool nearStart = pickPrefersEndpointA(edge, pick);
        auto arc = std::dynamic_pointer_cast<ArcEntity>(edge.entity);
        if (!arc) return false;

        const double start = arc->startDeg();
        const double end = arc->endDeg();
        const double sign = (end - start) >= 0.0 ? 1.0 : -1.0;

        const double endpointAngle = nearStart ? start : end;
        const Vec2 tangentForward{
            -std::sin(radians(endpointAngle)) * sign,
            std::cos(radians(endpointAngle)) * sign
        };

        // If trimming near arc start, keep the forward branch; if trimming near arc end, keep backward branch.
        *out = nearStart ? normalize(tangentForward) : normalize(tangentForward * -1.0);
        return length(*out) > 1e-9;
    }

    return false;
}

bool trimEdgeFromCornerDistance(
    const EdgeRef& edge,
    const Vec2& pick,
    const Vec2& corner,
    double trimDistance,
    Vec2* outTrimPoint)
{
    if (trimDistance <= 0.0) return false;

    if (edge.kind == EdgeKind::Line) {
        auto line = std::dynamic_pointer_cast<LineEntity>(edge.entity);
        if (!line) return false;

        const bool nearA = pickPrefersEndpointA(edge, pick);
        Vec2 a = line->a();
        Vec2 b = line->b();
        const Vec2 far = nearA ? b : a;

        const double available = distance(corner, far);
        if (available <= trimDistance + 1e-6) return false;

        const Vec2 dir = normalize(far - corner);
        const Vec2 trimmed = corner + dir * trimDistance;

        if (nearA) a = trimmed;
        else b = trimmed;
        line->setEndpoints(a, b);

        if (outTrimPoint) *outTrimPoint = trimmed;
        return true;
    }

    if (edge.kind == EdgeKind::PolylineSegment) {
        auto poly = std::dynamic_pointer_cast<PolylineEntity>(edge.entity);
        if (!poly) return false;

        const bool nearA = pickPrefersEndpointA(edge, pick);
        const Vec2 far = nearA ? edge.b : edge.a;
        const double available = distance(corner, far);
        if (available <= trimDistance + 1e-6) return false;

        const Vec2 dir = normalize(far - corner);
        const Vec2 trimmed = corner + dir * trimDistance;

        if (nearA) poly->setPoint(edge.indexA, trimmed);
        else poly->setPoint(edge.indexB, trimmed);

        if (outTrimPoint) *outTrimPoint = trimmed;
        return true;
    }

    if (edge.kind == EdgeKind::Arc) {
        auto arc = std::dynamic_pointer_cast<ArcEntity>(edge.entity);
        if (!arc) return false;

        const bool nearStart = pickPrefersEndpointA(edge, pick);
        const double start = arc->startDeg();
        const double end = arc->endDeg();
        const double sign = (end - start) >= 0.0 ? 1.0 : -1.0;
        const double spanDeg = std::abs(end - start);
        if (arc->radius() <= 1e-9 || spanDeg <= 1e-6) return false;

        const double trimDeg = degrees(trimDistance / arc->radius());
        if (trimDeg >= spanDeg - 1e-6) return false;

        double newStart = start;
        double newEnd = end;

        if (nearStart)
            newStart = start + sign * trimDeg;
        else
            newEnd = end - sign * trimDeg;

        arc->setAngles(newStart, newEnd);

        const double angle = nearStart ? newStart : newEnd;
        const Vec2 trimmed{
            arc->center().x + std::cos(radians(angle)) * arc->radius(),
            arc->center().y + std::sin(radians(angle)) * arc->radius()
        };

        if (outTrimPoint) *outTrimPoint = trimmed;
        return true;
    }

    return false;
}

bool setEdgeEndpointNearPick(const EdgeRef& edge, const Vec2& pick, const Vec2& replacement)
{
    if (edge.kind == EdgeKind::Line) {
        auto line = std::dynamic_pointer_cast<LineEntity>(edge.entity);
        if (!line) return false;
        Vec2 a = line->a();
        Vec2 b = line->b();
        if (distance(pick, a) <= distance(pick, b))
            a = replacement;
        else
            b = replacement;
        line->setEndpoints(a, b);
        return true;
    }

    if (edge.kind == EdgeKind::PolylineSegment) {
        auto poly = std::dynamic_pointer_cast<PolylineEntity>(edge.entity);
        if (!poly) return false;
        if (distance(pick, edge.a) <= distance(pick, edge.b))
            poly->setPoint(edge.indexA, replacement);
        else
            poly->setPoint(edge.indexB, replacement);
        return true;
    }

    if (edge.kind == EdgeKind::Arc) {
        auto arc = std::dynamic_pointer_cast<ArcEntity>(edge.entity);
        if (!arc) return false;

        const double angle = degrees(std::atan2(
            replacement.y - arc->center().y,
            replacement.x - arc->center().x));

        double start = arc->startDeg();
        double end = arc->endDeg();

        if (distance(pick, edge.a) <= distance(pick, edge.b))
            start = angle;
        else
            end = angle;

        arc->setAngles(start, end);
        return true;
    }

    return false;
}

ToolController::LinePreview toLinePreview(const EdgeRef& edge)
{
    ToolController::LinePreview p;
    if (edge.kind == EdgeKind::Line || edge.kind == EdgeKind::PolylineSegment) {
        p.valid = true;
        p.a = edge.a;
        p.b = edge.b;
    }
    return p;
}

} // namespace

namespace GForceCAD {

ToolController::ToolController(Document& document)
    : m_document(document)
{
}

void ToolController::setTool(ToolType type)
{
    m_tool = type;
    m_points.clear();
    m_previewPoint = {};
    m_primaryEntity.reset();
    m_primaryEdgeKind = EditEdgeKind::None;
    m_primaryIndexA = -1;
    m_primaryIndexB = -1;
    m_primaryPreview = {};
    m_hoverPreview = {};
    m_hasIntersectionPreview = false;
}

void ToolController::cancel()
{
    m_points.clear();
    m_previewPoint = {};
    m_primaryEntity.reset();
    m_primaryEdgeKind = EditEdgeKind::None;
    m_primaryIndexA = -1;
    m_primaryIndexB = -1;
    m_primaryPreview = {};
    m_hoverPreview = {};
    m_hasIntersectionPreview = false;
    m_tool = ToolType::Select;
}

void ToolController::setOffsetDistance(double value)
{
    if (value > 0.0)
        m_offsetDistance = value;
}

void ToolController::setFilletRadius(double value)
{
    if (value > 0.0)
        m_filletRadius = value;
}

void ToolController::setChamferDistances(double first, double second)
{
    if (first > 0.0) m_chamferDistance1 = first;
    if (second > 0.0) m_chamferDistance2 = second;
}

bool ToolController::click(const Vec2& point)
{
    switch (m_tool) {

    case ToolType::Select: {
        m_document.clearSelection();

        auto entity = m_document.entityAt(point, 8.0);
        if (entity) entity->setSelected(true);

        return true;
    }

    case ToolType::Line:
        m_points.append(point);

        if (m_points.size() == 2) {
            m_document.add(
                std::make_shared<LineEntity>(
                    m_document.nextId(),
                    m_points[0],
                    m_points[1],
                    m_document.currentLayer()
                )
            );

            m_points.clear();
        }
        return true;

    case ToolType::Circle:
        m_points.append(point);

        if (m_points.size() == 2) {
            const double radius = distance(m_points[0], m_points[1]);

            m_document.add(
                std::make_shared<CircleEntity>(
                    m_document.nextId(),
                    m_points[0],
                    radius,
                    m_document.currentLayer()
                )
            );

            m_points.clear();
        }
        return true;

    case ToolType::Tangent:
        m_points.append(point);

        if (m_points.size() == 2) {
            const Vec2 external = m_points[0];
            const Vec2 targetPick = m_points[1];
            auto entity = m_document.entityAt(targetPick, 8.0);
            QVector<Vec2> candidates;

            if (auto circle = std::dynamic_pointer_cast<CircleEntity>(entity)) {
                candidates = tangentPoints(external, circle->center(), circle->radius());
            } else if (auto arc = std::dynamic_pointer_cast<ArcEntity>(entity)) {
                const QVector<Vec2> arcCandidates = tangentPoints(
                    external, arc->center(), arc->radius()
                );
                for (const Vec2& candidate : arcCandidates) {
                    if (tangentPointOnArc(candidate, arc->center(), arc->startDeg(), arc->endDeg()))
                        candidates.append(candidate);
                }
            }

            if (!candidates.isEmpty()) {
                const Vec2 tangent = candidates.size() == 1 ||
                    distance(candidates[0], targetPick) <= distance(candidates[1], targetPick)
                    ? candidates[0] : candidates[1];

                m_document.add(std::make_shared<LineEntity>(
                    m_document.nextId(), external, tangent, m_document.currentLayer()
                ));
            }

            m_points.clear();
        }
        return true;

    case ToolType::Ellipse:
        m_points.append(point);

        if (m_points.size() == 3) {
            const Vec2 center = m_points[0];
            const double semiMajor = std::abs(m_points[1].x - center.x);
            const double semiMinor = std::abs(m_points[2].y - center.y);

            if (semiMajor > 1e-6 && semiMinor > 1e-6) {
                m_document.add(
                    std::make_shared<EllipseEntity>(
                        m_document.nextId(),
                        center,
                        semiMajor,
                        semiMinor,
                        m_document.currentLayer()
                    )
                );
            }

            m_points.clear();
        }
        return true;

    case ToolType::Arc:
        m_points.append(point);

        if (m_points.size() == 3) {
            const Vec2 center = m_points[0];
            const double radius = distance(center, m_points[1]);

            auto angle = [center](const Vec2& p) {
                return std::atan2(
                    p.y - center.y,
                    p.x - center.x
                ) * 180.0 / std::numbers::pi;
            };

            m_document.add(
                std::make_shared<ArcEntity>(
                    m_document.nextId(),
                    center,
                    radius,
                    angle(m_points[1]),
                    angle(m_points[2]),
                    m_document.currentLayer()
                )
            );

            m_points.clear();
        }
        return true;

    case ToolType::Polyline:
        if (!m_points.isEmpty() && distance(m_points.last(), point) < 1e-9) {
            if (m_points.size() >= 2) {
                m_document.add(
                    std::make_shared<PolylineEntity>(
                        m_document.nextId(),
                        m_points,
                        m_document.currentLayer(),
                        false
                    )
                );
            }
            m_points.clear();
        } else {
            m_points.append(point);
        }
        return true;

    case ToolType::Polygon:
        if (!m_points.isEmpty() && m_points.size() >= 3 &&
            distance(m_points.first(), point) < 1e-9) {
            m_document.add(
                std::make_shared<PolygonEntity>(
                    m_document.nextId(),
                    m_points,
                    m_document.currentLayer()
                )
            );
            m_points.clear();
        } else {
            m_points.append(point);
        }
        return true;

    case ToolType::Rectangle:
        m_points.append(point);

        if (m_points.size() == 2) {
            const Vec2 a = m_points[0];
            const Vec2 b = m_points[1];

            QVector<Vec2> rectangle{
                {a.x, a.y},
                {b.x, a.y},
                {b.x, b.y},
                {a.x, b.y}
            };

            m_document.add(
                std::make_shared<PolylineEntity>(
                    m_document.nextId(),
                    rectangle,
                    m_document.currentLayer(),
                    true
                )
            );

            m_points.clear();
        }
        return true;

    case ToolType::Offset: {
        const auto edgeOpt = findEdgeAt(m_document, point, 8.0);
        if (!edgeOpt || edgeOpt->kind == EdgeKind::Arc) return false;

        const Vec2 a = edgeOpt->a;
        const Vec2 b = edgeOpt->b;
        const Vec2 dir = b - a;
        const double len = length(dir);
        if (len < 1e-9) return false;

        Vec2 normal{-dir.y / len, dir.x / len};
        const Vec2 side = point - a;
        if (dot(side, normal) < 0.0)
            normal = normal * -1.0;

        const Vec2 delta = normal * m_offsetDistance;
        m_document.add(std::make_shared<LineEntity>(
            m_document.nextId(),
            a + delta,
            b + delta,
            m_document.currentLayer()));
        return true;
    }

    case ToolType::Trim:
    case ToolType::Extend:
    case ToolType::Fillet:
    case ToolType::Chamfer: {
        const auto edgeOpt = findEdgeAt(m_document, point, 8.0);
        if (!edgeOpt) return false;
        const EdgeRef edge = *edgeOpt;

        if (!m_primaryEntity) {
            m_primaryEntity = edge.entity;
            m_primaryEdgeKind = edge.kind == EdgeKind::Line
                ? EditEdgeKind::Line
                : edge.kind == EdgeKind::PolylineSegment
                    ? EditEdgeKind::PolylineSegment
                    : EditEdgeKind::Arc;
            m_primaryIndexA = edge.indexA;
            m_primaryIndexB = edge.indexB;
            m_primaryPick = point;
            m_primaryPreview = toLinePreview(edge);
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return true;
        }

        EdgeRef first;
        first.entity = m_primaryEntity;
        if (m_primaryEdgeKind == EditEdgeKind::Line) {
            auto line = std::dynamic_pointer_cast<LineEntity>(m_primaryEntity);
            if (!line) {
                m_primaryEntity.reset();
                return false;
            }
            first.kind = EdgeKind::Line;
            first.a = line->a();
            first.b = line->b();
        } else if (m_primaryEdgeKind == EditEdgeKind::PolylineSegment) {
            auto poly = std::dynamic_pointer_cast<PolylineEntity>(m_primaryEntity);
            if (!poly) {
                m_primaryEntity.reset();
                return false;
            }
            const QVector<Vec2>& pts = poly->points();
            if (m_primaryIndexA < 0 || m_primaryIndexA >= pts.size() ||
                m_primaryIndexB < 0 || m_primaryIndexB >= pts.size()) {
                m_primaryEntity.reset();
                return false;
            }
            first.kind = EdgeKind::PolylineSegment;
            first.indexA = m_primaryIndexA;
            first.indexB = m_primaryIndexB;
            first.a = pts[m_primaryIndexA];
            first.b = pts[m_primaryIndexB];
        } else if (m_primaryEdgeKind == EditEdgeKind::Arc) {
            auto arc = std::dynamic_pointer_cast<ArcEntity>(m_primaryEntity);
            if (!arc) {
                m_primaryEntity.reset();
                return false;
            }
            first.kind = EdgeKind::Arc;
            first.center = arc->center();
            first.radius = arc->radius();
            first.startDeg = arc->startDeg();
            first.endDeg = arc->endDeg();
            first.a = {first.center.x + std::cos(radians(first.startDeg)) * first.radius,
                       first.center.y + std::sin(radians(first.startDeg)) * first.radius};
            first.b = {first.center.x + std::cos(radians(first.endDeg)) * first.radius,
                       first.center.y + std::sin(radians(first.endDeg)) * first.radius};
        }

        EdgeRef second = edge;
        if (first.entity == second.entity && first.kind == second.kind &&
            first.indexA == second.indexA && first.indexB == second.indexB) {
            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return false;
        }

        const bool requireOnBoth = (m_tool == ToolType::Trim);
        const QVector<Vec2> intersections = intersectionCandidates(first, second, requireOnBoth);
        if (intersections.isEmpty()) {
            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return false;
        }
        const Vec2 intersection = (m_tool == ToolType::Fillet || m_tool == ToolType::Chamfer)
            ? [&]() {
                Vec2 best{};
                double bestScore = std::numeric_limits<double>::max();
                for (const Vec2& p : intersections) {
                    const double score = distance(p, m_primaryPick) + distance(p, point);
                    if (score < bestScore) {
                        bestScore = score;
                        best = p;
                    }
                }
                return best;
            }()
            : closestPoint(intersections, point);

        if (m_tool == ToolType::Trim) {
            m_document.beginEdit();
            if (!setEdgeEndpointNearPick(second, point, intersection)) {
                m_primaryEntity.reset();
                m_primaryEdgeKind = EditEdgeKind::None;
                return false;
            }

            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return true;
        }

        if (m_tool == ToolType::Extend) {
            m_document.beginEdit();
            if (!setEdgeEndpointNearPick(second, point, intersection)) {
                m_primaryEntity.reset();
                m_primaryEdgeKind = EditEdgeKind::None;
                return false;
            }

            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return true;
        }

        const bool arcInvolved = first.kind == EdgeKind::Arc || second.kind == EdgeKind::Arc;
        if (arcInvolved) {
            // Arc-aware mode: require a clear corner pick on edge endpoints to avoid ambiguous side solutions.
            const Vec2 e1 = edgeEndpointNearPick(first, m_primaryPick);
            const Vec2 e2 = edgeEndpointNearPick(second, point);
            const double cornerTolerance = 3.0;
            if (distance(intersection, e1) > cornerTolerance || distance(intersection, e2) > cornerTolerance) {
                m_primaryEntity.reset();
                m_primaryEdgeKind = EditEdgeKind::None;
                m_primaryPreview = {};
                m_hoverPreview = {};
                m_hasIntersectionPreview = false;
                return false;
            }
        }

        Vec2 u1{};
        Vec2 u2{};
        if (!edgeDirectionFromCorner(first, m_primaryPick, intersection, &u1) ||
            !edgeDirectionFromCorner(second, point, intersection, &u2)) {
            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return false;
        }

        const double c = clamp(dot(u1, u2), -1.0, 1.0);
        const double theta = std::acos(c);
        if (theta < 1e-3 || std::abs(theta - std::numbers::pi) < 1e-3) {
            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return false;
        }

        if (m_tool == ToolType::Fillet) {
            const double trimDistance = m_filletRadius / std::tan(theta * 0.5);
            const double centerDistance = m_filletRadius / std::sin(theta * 0.5);

            Vec2 t1{};
            Vec2 t2{};
            const Vec2 bisector = normalize(u1 + u2);
            if (length(bisector) < 1e-9) {
                m_primaryEntity.reset();
                return false;
            }

            const Vec2 center = intersection + bisector * centerDistance;

            m_document.beginEdit();
            if (!trimEdgeFromCornerDistance(first, m_primaryPick, intersection, trimDistance, &t1) ||
                !trimEdgeFromCornerDistance(second, point, intersection, trimDistance, &t2)) {
                m_primaryEntity.reset();
                m_primaryEdgeKind = EditEdgeKind::None;
                m_primaryPreview = {};
                m_hoverPreview = {};
                m_hasIntersectionPreview = false;
                return false;
            }

            auto angle = [center](const Vec2& p) {
                return std::atan2(p.y - center.y, p.x - center.x) * 180.0 / std::numbers::pi;
            };

            auto arc = std::make_shared<ArcEntity>(
                m_document.nextId(),
                center,
                m_filletRadius,
                angle(t1),
                angle(t2),
                m_document.currentLayer());
            m_document.ensureLayer(arc->layer());
            m_document.entities().push_back(arc);

            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return true;
        }

        if (m_tool == ToolType::Chamfer) {
            Vec2 c1{};
            Vec2 c2{};

            m_document.beginEdit();
            if (!trimEdgeFromCornerDistance(first, m_primaryPick, intersection, m_chamferDistance1, &c1) ||
                !trimEdgeFromCornerDistance(second, point, intersection, m_chamferDistance2, &c2)) {
                m_primaryEntity.reset();
                m_primaryEdgeKind = EditEdgeKind::None;
                m_primaryPreview = {};
                m_hoverPreview = {};
                m_hasIntersectionPreview = false;
                return false;
            }

            auto chamfer = std::make_shared<LineEntity>(
                m_document.nextId(),
                c1,
                c2,
                m_document.currentLayer());
            m_document.ensureLayer(chamfer->layer());
            m_document.entities().push_back(chamfer);

            m_primaryEntity.reset();
            m_primaryEdgeKind = EditEdgeKind::None;
            m_primaryPreview = {};
            m_hoverPreview = {};
            m_hasIntersectionPreview = false;
            return true;
        }

        m_primaryEntity.reset();
        m_primaryEdgeKind = EditEdgeKind::None;
        m_primaryPreview = {};
        m_hoverPreview = {};
        m_hasIntersectionPreview = false;
        return false;
    }
    }

    return false;
}

void ToolController::move(const Vec2& point)
{
    m_previewPoint = point;

    if (m_tool != ToolType::Trim &&
        m_tool != ToolType::Extend &&
        m_tool != ToolType::Fillet &&
        m_tool != ToolType::Chamfer) {
        m_hoverPreview = {};
        m_hasIntersectionPreview = false;
        return;
    }

    const auto hoverOpt = findEdgeAt(m_document, point, 8.0);
    if (!hoverOpt) {
        m_hoverPreview = {};
        m_hasIntersectionPreview = false;
        return;
    }

    const EdgeRef hover = *hoverOpt;
    m_hoverPreview = toLinePreview(hover);

    if (!m_primaryEntity || m_primaryEdgeKind == EditEdgeKind::None) {
        m_hasIntersectionPreview = false;
        return;
    }

    EdgeRef first;
    first.entity = m_primaryEntity;

    if (m_primaryEdgeKind == EditEdgeKind::Line) {
        auto line = std::dynamic_pointer_cast<LineEntity>(m_primaryEntity);
        if (!line) {
            m_hasIntersectionPreview = false;
            return;
        }
        first.kind = EdgeKind::Line;
        first.a = line->a();
        first.b = line->b();
    } else if (m_primaryEdgeKind == EditEdgeKind::PolylineSegment) {
        auto poly = std::dynamic_pointer_cast<PolylineEntity>(m_primaryEntity);
        if (!poly) {
            m_hasIntersectionPreview = false;
            return;
        }
        const QVector<Vec2>& pts = poly->points();
        if (m_primaryIndexA < 0 || m_primaryIndexA >= pts.size() ||
            m_primaryIndexB < 0 || m_primaryIndexB >= pts.size()) {
            m_hasIntersectionPreview = false;
            return;
        }
        first.kind = EdgeKind::PolylineSegment;
        first.indexA = m_primaryIndexA;
        first.indexB = m_primaryIndexB;
        first.a = pts[m_primaryIndexA];
        first.b = pts[m_primaryIndexB];
    } else if (m_primaryEdgeKind == EditEdgeKind::Arc) {
        auto arc = std::dynamic_pointer_cast<ArcEntity>(m_primaryEntity);
        if (!arc) {
            m_hasIntersectionPreview = false;
            return;
        }
        first.kind = EdgeKind::Arc;
        first.center = arc->center();
        first.radius = arc->radius();
        first.startDeg = arc->startDeg();
        first.endDeg = arc->endDeg();
        first.a = {first.center.x + std::cos(radians(first.startDeg)) * first.radius,
                   first.center.y + std::sin(radians(first.startDeg)) * first.radius};
        first.b = {first.center.x + std::cos(radians(first.endDeg)) * first.radius,
                   first.center.y + std::sin(radians(first.endDeg)) * first.radius};
    }

    const bool requireOnBoth = (m_tool == ToolType::Trim);
    const QVector<Vec2> intersections = intersectionCandidates(first, hover, requireOnBoth);
    if (intersections.isEmpty()) {
        m_hasIntersectionPreview = false;
        return;
    }

    m_hasIntersectionPreview = true;
    m_intersectionPreview = closestPoint(intersections, point);
}

QString ToolController::prompt() const
{
    switch (m_tool) {
    case ToolType::Select: return "Select object";
    case ToolType::Line:
        return m_points.isEmpty() ? "Specify first point" : "Specify second point";
    case ToolType::Circle:
        return m_points.isEmpty() ? "Specify center" : "Specify radius";
    case ToolType::Tangent:
        return m_points.isEmpty()
            ? "Specify external point"
            : "Select circle or arc for tangent line";
    case ToolType::Ellipse:
        if (m_points.isEmpty()) return "Specify center";
        if (m_points.size() == 1) return "Specify major radius";
        return "Specify minor radius";
    case ToolType::Arc:
        if (m_points.isEmpty()) return "Specify center";
        if (m_points.size() == 1) return "Specify start point";
        return "Specify end point";
    case ToolType::Polyline:
        return "Specify next point; click the last point to finish";
    case ToolType::Polygon:
        return m_points.isEmpty()
            ? "Specify first vertex"
            : (m_points.size() < 3 ? "Specify next vertex" : "Click the first vertex to close the polygon");
    case ToolType::Rectangle:
        return m_points.isEmpty() ? "Specify first corner" : "Specify opposite corner";
    case ToolType::Offset:
        return QString("Select line to offset (%1)").arg(m_offsetDistance, 0, 'f', 2);
    case ToolType::Trim:
        return m_primaryEntity
            ? "Select target edge to trim"
            : "Select cutting edge";
    case ToolType::Extend:
        return m_primaryEntity
            ? "Select edge to extend"
            : "Select boundary edge";
    case ToolType::Fillet:
        return m_primaryEntity
            ? QString("Select second edge (R=%1)").arg(m_filletRadius, 0, 'f', 2)
            : QString("Select first edge (R=%1)").arg(m_filletRadius, 0, 'f', 2);
    case ToolType::Chamfer:
        return m_primaryEntity
            ? QString("Select second edge (%1,%2)")
                .arg(m_chamferDistance1, 0, 'f', 2)
                .arg(m_chamferDistance2, 0, 'f', 2)
            : QString("Select first edge (%1,%2)")
                .arg(m_chamferDistance1, 0, 'f', 2)
                .arg(m_chamferDistance2, 0, 'f', 2);
    }

    return {};
}

}
