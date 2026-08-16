#include "Entity.h"
#include "Entities.h"
#include <QJsonArray>

namespace GForceCAD {

Entity::Entity(int id, EntityType type, const QString& layer)
    : m_id(id), m_type(type), m_layer(layer)
{
}

std::shared_ptr<Entity> Entity::fromJson(const QJsonObject& o)
{
    const QString type = o["type"].toString();
    const int id = o["id"].toInt();
    const QString layer = o["layer"].toString("0");

    if (type == "LINE") {
        return std::make_shared<LineEntity>(
            id,
            Vec2{o["x1"].toDouble(), o["y1"].toDouble()},
            Vec2{o["x2"].toDouble(), o["y2"].toDouble()},
            layer
        );
    }

    if (type == "CIRCLE") {
        return std::make_shared<CircleEntity>(
            id,
            Vec2{o["cx"].toDouble(), o["cy"].toDouble()},
            o["radius"].toDouble(),
            layer
        );
    }

    if (type == "ARC") {
        return std::make_shared<ArcEntity>(
            id,
            Vec2{o["cx"].toDouble(), o["cy"].toDouble()},
            o["radius"].toDouble(),
            o["start"].toDouble(),
            o["end"].toDouble(),
            layer
        );
    }

    if (type == "POLYLINE" || type == "RECTANGLE") {
        QVector<Vec2> points;
        const QJsonArray array = o["points"].toArray();
        for (const auto& value : array) {
            const QJsonObject p = value.toObject();
            points.append({p["x"].toDouble(), p["y"].toDouble()});
        }

        if (type == "RECTANGLE") {
            return std::make_shared<PolylineEntity>(
                id, points, layer, true
            );
        }

        return std::make_shared<PolylineEntity>(
            id, points, layer, false
        );
    }

    return nullptr;
}

}
