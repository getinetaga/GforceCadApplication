#pragma once

// CAD document model for a layered drawing.
// Stores entities, undo/redo snapshots, layer metadata, and JSON persistence for the design file.
#include "Entity.h"
#include <QJsonDocument>
#include <QSet>
#include <QStringList>
#include <memory>
#include <vector>

namespace GForceCAD {

struct Layer
{
    QString name;
    QColor color;
    bool visible{true};
};

class Document
{
public:
    Document();

    void clear();

    int nextId() { return m_nextId++; }

    void add(const std::shared_ptr<Entity>& entity);
    void removeSelected();
    void clearSelection();
    std::shared_ptr<Entity> entityAt(const Vec2& point, double tolerance);
    std::shared_ptr<Entity> selectedEntity() const;

    const std::vector<std::shared_ptr<Entity>>& entities() const { return m_entities; }
    std::vector<std::shared_ptr<Entity>>& entities() { return m_entities; }

    QString currentLayer() const { return m_currentLayer; }
    void setCurrentLayer(const QString& layer) { m_currentLayer = layer; ensureLayer(layer); }

    QStringList layerNames() const;
    void ensureLayer(const QString& name);
    const Layer* getLayer(const QString& name) const;

    bool save(const QString& fileName, QString* error = nullptr) const;
    bool load(const QString& fileName, QString* error = nullptr);

    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    void beginEdit();

private:
    std::vector<std::shared_ptr<Entity>> m_entities;
    std::vector<QJsonObject> m_undo;
    std::vector<QJsonObject> m_redo;
    QMap<QString, Layer> m_layers;
    QString m_currentLayer{"0"};
    int m_nextId{1};

    QJsonObject snapshot() const;
    void restore(const QJsonObject& state);
    void checkpoint();
};

}
