#include "mainwindow.h"

#include <QApplication>

/// Entry point that starts the Qt event loop and shows the main window.
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
