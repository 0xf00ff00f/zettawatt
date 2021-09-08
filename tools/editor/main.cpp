#include <QApplication>

#include "editorwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    EditorWindow w;
    w.resize(1024, 600);
    w.show();

    return app.exec();
}
