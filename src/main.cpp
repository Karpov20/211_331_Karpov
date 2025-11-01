#include "mainwindow.h"
#include "security/securitymanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    security::installGuards();
    MainWindow window;
    window.show();
    return app.exec();
}
