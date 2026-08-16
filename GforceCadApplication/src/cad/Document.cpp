#include "Document.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <algorithm>

namespace GForceCAD {

Document::Document()
{
    ensureLayer("0");
    ensureLayer("Walls");
    ensureLayer("Doors");
    ensureLayer("Windows");
    ensureLayer("Dimensions");
}

void Document::ensureLayer(const QString& name)
{
    if (!m_layers.contains(name))
        m_layers.insert(name, Layer{name, QColor("#e8edf2"), true});
}

QStringList Document::layerNames() const
{
    return m_layers.keys();
}

const Layer* Document::getLayer(const QString& name) const
{
    auto it = m_layers.constFind(name);
    if (it == m_layers.constEnd()) return nullptr;
    return &it.value();
}

void Document::checkpoint()
{
    m_undo.push_back(snapshot());
    m_redo.clear();

    if (m_undo.size() > 100)
        m_undo.erase(m_undo.begin());
}

QJsonObject Document::snapshot() const
{
    QJsonArray entities;

    for (const auto& entity : m_entities)
        entities.append(entity->toJson());

    QJsonObject state;
    state["nextId"] = m_nextId;
    state["currentLayer"] = m_currentLayer;
    state["entities"] = entities;
    return state;
}

void Document::restore(const QJsonObject& state)
{
    m_entities.clear();

    m_nextId = state["nextId"].toInt(1);
    m_currentLayer = state["currentLayer"].toString("0");

    for (const auto& value : state["entities"].toArray()) {
        auto entity = Entity::fromJson(value.toObject());
        if (entity) {
            ensureLayer(entity->layer());
            m_entities.push_back(entity);
        }
    }
}

void Document::add(const std::shared_ptr<Entity>& entity)
{
    checkpoint();
    ensureLayer(entity->layer());
    m_entities.push_back(entity);
}

void Document::clear()
{
    checkpoint();
    m_entities.clear();
    m_nextId = 1;
}

void Document::clearSelection()
{
    for (auto& entity : m_entities)
        entity->setSelected(false);
}

std::shared_ptr<Entity> Document::entityAt(const Vec2& point, double tolerance)
{
    for (auto it = m_entities.rbegin(); it != m_entities.rend(); ++it) {
        if ((*it)->hitTest(point, tolerance))
            return *it;
    }
    return nullptr;
}

std::shared_ptr<Entity> Document::selectedEntity() const
{
    for (const auto& entity : m_entities)
        if (entity->selected())
            return entity;
    return nullptr;
}

void Document::removeSelected()
{
    bool found = false;

    for (const auto& entity : m_entities)
        if (entity->selected()) found = true;

    if (!found) return;

    checkpoint();

    m_entities.erase(
        std::remove_if(
            m_entities.begin(),
            m_entities.end(),
            [](const auto& e) { return e->selected(); }
        ),
        m_entities.end()
    );
}

bool Document::save(const QString& fileName, QString* error) const
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    const QJsonDocument doc(snapshot());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool Document::load(const QString& fileName, QString* error)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }

    checkpoint();
    restore(doc.object());
    return true;
}

void Document::undo()
{
    if (m_undo.empty()) return;

    m_redo.push_back(snapshot());
    const QJsonObject state = m_undo.back();
    m_undo.pop_back();
    restore(state);
}

void Document::redo()
{
    if (m_redo.empty()) return;

    m_undo.push_back(snapshot());
    const QJsonObject state = m_redo.back();
    m_redo.pop_back();
    restore(state);
}

bool Document::canUndo() const
{
    return !m_undo.empty();
}

bool Document::canRedo() const
{
    return !m_redo.empty();
}

void Document::beginEdit()
{
    checkpoint();
}

}
