#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SoftEtherVPN-QtManager"));
    app.setOrganizationName(QStringLiteral("SoftEtherVPN-QtManager"));

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
