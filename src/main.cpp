#include "ui/ConnectDialog.h"
#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SoftEtherVPN-QtManager"));
    app.setOrganizationName(QStringLiteral("SoftEtherVPN-QtManager"));

    MainWindow mainWindow;

    ConnectDialog connectDialog;
    if (connectDialog.exec() != QDialog::Accepted) {
        return 0;
    }
    mainWindow.setConnection(connectDialog.takeConnectedRpc(), connectDialog.serverInfo());

    mainWindow.show();
    return app.exec();
}
