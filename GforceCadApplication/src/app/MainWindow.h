#pragma once

#include <QMainWindow>
#include "../cad/Document.h"
#include "../cad/Tools.h"

class QListWidget;
class QLabel;
class QTextEdit;
class QComboBox;
class QAction;
class QLineEdit;

namespace GForceCAD {

class CadViewport;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void newDocument();
    void openDocument();
    void saveDocument();
    void saveDocumentAs();

    void undo();
    void redo();

    void chooseTool(ToolType type);

    void updateCursor(const Vec2& point);
    void updateProperties();

    void setLayerFromPanel(int index);
    void runCommand();

private:
    void buildUi();
    void buildMenus();
    void buildToolbars();
    void buildPanels();
    void updateStatus();
    void updateLayerPanel();
    void updateCommandPrompt();
    void scaleSelected(double factor);

    Document m_document;
    ToolController m_tools;
    CadViewport* m_viewport{nullptr};

    QListWidget* m_layers{nullptr};
    QTextEdit* m_properties{nullptr};
    QComboBox* m_layerCombo{nullptr};
    QLabel* m_coordinateLabel{nullptr};
    QLabel* m_promptLabel{nullptr};
    QLineEdit* m_commandLine{nullptr};

    QString m_fileName;
    bool m_dirty{false};

    QAction* m_gridAction{nullptr};
    QAction* m_snapAction{nullptr};
    QAction* m_objectSnapAction{nullptr};
};

}
