#pragma once

#include "Geometry.h"
#include <QPainter>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <memory>

namespace GForceCAD {

enum class EntityType
{
    Line,
    Circle,
    Ellipse,
    Arc,
    Polyline,
    Rectangle,
    Polygon,
    Triangle
};

class Entity
{
public:
    Entity(int id, EntityType type, const QString& layer = "0");
    virtual ~Entity() = default;

    int id() const { return m_id; }
    EntityType type() const { return m_type; }

    QString layer() const { return m_layer; }
    void setLayer(const QString& layer) { m_layer = layer; }

    bool selected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

    virtual void draw(QPainter& painter, double scale) const = 0;
    virtual bool hitTest(const Vec2& world, double tolerance) const = 0;
    virtual QVector<Vec2> snapPoints() const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual void moveBy(const Vec2& delta) = 0;
    virtual QString properties() const = 0;

    static std::shared_ptr<Entity> fromJson(const QJsonObject& object);

protected:
    int m_id;
    EntityType m_type;
    QString m_layer;
    bool m_selected{false};
};

}
