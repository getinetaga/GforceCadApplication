#include <QApplication>
#include <QSurfaceFormat>
#include "app/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication::setApplicationName("GForce CAD");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("GForce");

    GForceCAD::MainWindow window;
    window.show();

    return app.exec();
}
