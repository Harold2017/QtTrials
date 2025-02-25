#include <QApplication>

#include "QtRhiViewer.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    auto* v = new QtRhiViewer;
    v->loadMesh(argv[1]);
    v->show();

    return app.exec();
}