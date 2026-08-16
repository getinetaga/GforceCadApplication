#include "MainWindow.h"
#include "../render/CadViewport.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileInfo>
#include <QStringList>

namespace GForceCAD {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_tools(m_document)
{
    buildUi();
    buildMenus();
    buildToolbars();
    buildPanels();

    resize(1500, 900);
    setWindowTitle("GForce CAD — Untitled");

    updateLayerPanel();
    updateStatus();
    updateCommandPrompt();
}

void MainWindow::buildUi()
{
    m_viewport = new CadViewport(m_document, m_tools, this);
    setCentralWidget(m_viewport);

    connect(m_viewport, &CadViewport::cursorChanged,
            this, &MainWindow::updateCursor);

    connect(m_viewport, &CadViewport::selectionChanged,
            this, &MainWindow::updateProperties);
}

void MainWindow::buildMenus()
{
    auto* file = menuBar()->addMenu("&File");

    file->addAction("New", this, &MainWindow::newDocument, QKeySequence::New);
    file->addAction("Open...", this, &MainWindow::openDocument, QKeySequence::Open);
    file->addAction("Save", this, &MainWindow::saveDocument, QKeySequence::Save);
    file->addAction("Save As...", this, &MainWindow::saveDocumentAs);
    file->addSeparator();
    file->addAction("Exit", this, &QWidget::close, QKeySequence::Quit);

    auto* edit = menuBar()->addMenu("&Edit");
    edit->addAction("Undo", this, &MainWindow::undo, QKeySequence::Undo);
    edit->addAction("Redo", this, &MainWindow::redo, QKeySequence::Redo);
    edit->addAction("Delete", m_viewport, nullptr, QKeySequence::Delete);

    auto* view = menuBar()->addMenu("&View");
    m_gridAction = view->addAction("Grid");
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(true);
    connect(m_gridAction, &QAction::toggled, m_viewport, &CadViewport::setGridEnabled);

    m_snapAction = view->addAction("Snap");
    m_snapAction->setCheckable(true);
    m_snapAction->setChecked(true);
    connect(m_snapAction, &QAction::toggled, m_viewport, &CadViewport::setSnapEnabled);

        m_objectSnapAction = view->addAction("Object Snap");
        m_objectSnapAction->setCheckable(true);
        m_objectSnapAction->setChecked(true);
        connect(m_objectSnapAction, &QAction::toggled,
            m_viewport, &CadViewport::setObjectSnapEnabled);

    view->addAction("Zoom Extents", m_viewport, &CadViewport::zoomExtents);
}

void MainWindow::buildToolbars()
{
    auto* toolbar = addToolBar("GForce CAD Tools");
    toolbar->setMovable(false);

    auto addTool = [&](const QString& name, ToolType type) {
        QAction* action = toolbar->addAction(name);
        connect(action, &QAction::triggered, this, [this, type]() {
            chooseTool(type);
        });
    };

    addTool("Select", ToolType::Select);
    toolbar->addSeparator();
    addTool("Line", ToolType::Line);
    addTool("Circle", ToolType::Circle);
    addTool("Ellipse", ToolType::Ellipse);
    addTool("Arc", ToolType::Arc);
    addTool("Polyline", ToolType::Polyline);
    addTool("Rectangle", ToolType::Rectangle);

    toolbar->addSeparator();

    addTool("Offset", ToolType::Offset);
    addTool("Trim", ToolType::Trim);
    addTool("Extend", ToolType::Extend);
    addTool("Fillet", ToolType::Fillet);
    addTool("Chamfer", ToolType::Chamfer);

    toolbar->addSeparator();

    toolbar->addAction("Undo", this, &MainWindow::undo);
    toolbar->addAction("Redo", this, &MainWindow::redo);
}

void MainWindow::buildPanels()
{
    auto* layerDock = new QDockWidget("Layers", this);
    layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* layerWidget = new QWidget;
    auto* layerLayout = new QVBoxLayout(layerWidget);

    m_layerCombo = new QComboBox;
    layerLayout->addWidget(new QLabel("Current Layer"));
    layerLayout->addWidget(m_layerCombo);

    connect(m_layerCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::setLayerFromPanel);

    m_layers = new QListWidget;
    layerLayout->addWidget(m_layers);

    auto* addLayer = new QPushButton("New Layer");
    layerLayout->addWidget(addLayer);

    connect(addLayer, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, "New Layer", "Layer name:", QLineEdit::Normal, "", &ok
        );

        if (ok && !name.trimmed().isEmpty()) {
            m_document.ensureLayer(name.trimmed());
            m_document.setCurrentLayer(name.trimmed());
            updateLayerPanel();
            m_viewport->update();
        }
    });

    layerDock->setWidget(layerWidget);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock);

    auto* propertiesDock = new QDockWidget("Properties", this);
    propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_properties = new QTextEdit;
    m_properties->setReadOnly(true);
    propertiesDock->setWidget(m_properties);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);

    auto* commandDock = new QDockWidget("Command", this);
    commandDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* commandWidget = new QWidget;
    auto* commandLayout = new QHBoxLayout(commandWidget);

    m_promptLabel = new QLabel;
    m_commandLine = new QLineEdit;
    m_commandLine->setPlaceholderText("Type a CAD command: LINE, CIRCLE, ARC, RECTANGLE...");

    commandLayout->addWidget(m_promptLabel);
    commandLayout->addWidget(m_commandLine);

    connect(m_commandLine, &QLineEdit::returnPressed,
            this, &MainWindow::runCommand);

    commandDock->setWidget(commandWidget);
    addDockWidget(Qt::BottomDockWidgetArea, commandDock);

    m_coordinateLabel = new QLabel("X: 0.000  Y: 0.000");
    statusBar()->addPermanentWidget(m_coordinateLabel);
}

void MainWindow::newDocument()
{
    m_document.clear();
    m_fileName.clear();
    m_dirty = false;
    m_tools.cancel();

    setWindowTitle("GForce CAD — Untitled");
    updateLayerPanel();
    updateProperties();
    m_viewport->update();
}

void MainWindow::openDocument()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        "Open GForce CAD Drawing",
        QString(),
        "GForce CAD (*.gfcad);;JSON (*.json);;All Files (*)"
    );

    if (file.isEmpty()) return;

    QString error;
    if (!m_document.load(file, &error)) {
        QMessageBox::critical(this, "Open Failed", error);
        return;
    }

    m_fileName = file;
    m_dirty = false;
    setWindowTitle("GForce CAD — " + QFileInfo(file).fileName());

    updateLayerPanel();
    updateProperties();
    m_viewport->update();
}

void MainWindow::saveDocument()
{
    if (m_fileName.isEmpty()) {
        saveDocumentAs();
        return;
    }

    QString error;
    if (!m_document.save(m_fileName, &error)) {
        QMessageBox::critical(this, "Save Failed", error);
        return;
    }

    m_dirty = false;
    statusBar()->showMessage("Drawing saved", 3000);
}

void MainWindow::saveDocumentAs()
{
    const QString file = QFileDialog::getSaveFileName(
        this,
        "Save GForce CAD Drawing",
        "drawing.gfcad",
        "GForce CAD (*.gfcad);;JSON (*.json)"
    );

    if (file.isEmpty()) return;

    m_fileName = file;
    saveDocument();
    setWindowTitle("GForce CAD — " + QFileInfo(file).fileName());
}

void MainWindow::undo()
{
    m_document.undo();
    m_viewport->update();
    updateProperties();
}

void MainWindow::redo()
{
    m_document.redo();
    m_viewport->update();
    updateProperties();
}

void MainWindow::chooseTool(ToolType type)
{
    m_tools.setTool(type);
    updateCommandPrompt();
    statusBar()->showMessage(m_tools.prompt());
    m_viewport->setFocus();
}

void MainWindow::updateCursor(const Vec2& point)
{
    m_coordinateLabel->setText(
        QString("X: %1    Y: %2")
            .arg(point.x, 0, 'f', 3)
            .arg(point.y, 0, 'f', 3)
    );

    updateCommandPrompt();
}

void MainWindow::updateProperties()
{
    auto entity = m_document.selectedEntity();

    if (entity)
        m_properties->setPlainText(entity->properties());
    else
        m_properties->setPlainText("No object selected.");

    updateStatus();
}

void MainWindow::setLayerFromPanel(int index)
{
    if (index < 0 || index >= m_layerCombo->count()) return;

    m_document.setCurrentLayer(m_layerCombo->itemText(index));
    updateStatus();
}

void MainWindow::runCommand()
{
    const QString command = m_commandLine->text().trimmed().toUpper();
    m_commandLine->clear();

    const QStringList tokens = command.split(' ', Qt::SkipEmptyParts);
    const QString keyword = tokens.isEmpty() ? QString() : tokens.first();
    auto tokenDouble = [&tokens](int index, double fallback) {
        if (index < 0 || index >= tokens.size()) return fallback;
        bool ok = false;
        const double value = tokens[index].toDouble(&ok);
        return ok ? value : fallback;
    };

    if (command == "LINE" || keyword == "L") chooseTool(ToolType::Line);
    else if (command == "CIRCLE" || keyword == "C") chooseTool(ToolType::Circle);
    else if (command == "ELLIPSE" || keyword == "E") chooseTool(ToolType::Ellipse);
    else if (command == "ARC" || keyword == "A") chooseTool(ToolType::Arc);
    else if (command == "POLYLINE" || command == "PLINE") chooseTool(ToolType::Polyline);
    else if (command == "RECTANGLE" || command == "RECTANG" || keyword == "REC") chooseTool(ToolType::Rectangle);
    else if (command == "SELECT") chooseTool(ToolType::Select);
    else if (keyword == "OFFSET" || keyword == "O") {
        m_tools.setOffsetDistance(tokenDouble(1, m_tools.offsetDistance()));
        chooseTool(ToolType::Offset);
    }
    else if (keyword == "TRIM" || keyword == "TR") {
        chooseTool(ToolType::Trim);
    }
    else if (keyword == "EXTEND" || keyword == "EX") {
        chooseTool(ToolType::Extend);
    }
    else if (keyword == "FILLET" || keyword == "F") {
        m_tools.setFilletRadius(tokenDouble(1, m_tools.filletRadius()));
        chooseTool(ToolType::Fillet);
    }
    else if (keyword == "CHAMFER" || keyword == "CHA") {
        const double d1 = tokenDouble(1, m_tools.chamferDistance1());
        const double d2 = tokenDouble(2, m_tools.chamferDistance2());
        m_tools.setChamferDistances(d1, d2);
        chooseTool(ToolType::Chamfer);
    }
    else if (command == "UNDO") undo();
    else if (command == "REDO") redo();
    else if (command == "DELETE" || command == "ERASE") {
        m_document.removeSelected();
        m_viewport->update();
        updateProperties();
    }
    else if (command == "GRID") {
        m_gridAction->setChecked(!m_gridAction->isChecked());
    }
    else if (command == "SNAP") {
        m_snapAction->setChecked(!m_snapAction->isChecked());
    }
    else if (command == "OSNAP" || command == "OBJECTSNAP") {
        m_objectSnapAction->setChecked(!m_objectSnapAction->isChecked());
    }
    else if (command == "ZOOM EXTENTS") {
        m_viewport->zoomExtents();
    }
    else if (command == "SAVE") {
        saveDocument();
    }
    else if (!command.isEmpty()) {
        statusBar()->showMessage("Unknown command: " + command, 3000);
    }
}

void MainWindow::updateStatus()
{
    statusBar()->showMessage(
        QString("Layer: %1 | Objects: %2 | %3")
            .arg(m_document.currentLayer())
            .arg(m_document.entities().size())
            .arg(m_tools.prompt())
    );
}

void MainWindow::updateLayerPanel()
{
    m_layerCombo->blockSignals(true);
    m_layerCombo->clear();
    m_layerCombo->addItems(m_document.layerNames());
    m_layerCombo->setCurrentText(m_document.currentLayer());
    m_layerCombo->blockSignals(false);

    m_layers->clear();

    for (const QString& layer : m_document.layerNames()) {
        const Layer* data = m_document.getLayer(layer);
        QString label = layer;
        if (data && !data->visible) label += " [OFF]";
        m_layers->addItem(label);
    }
}

void MainWindow::updateCommandPrompt()
{
    m_promptLabel->setText(m_tools.prompt() + "   ");
}

}
