#include "MainWindow.h"
#include "ConnectionListPage.h"
#include "HubListPage.h"

#include <QAction>
#include <QMenuBar>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SoftEtherVPN-QtManager"));
    resize(900, 560);

    m_connectionListPage = new ConnectionListPage(this);
    m_hubListPage = new HubListPage(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_connectionListPage);
    m_stack->addWidget(m_hubListPage);
    setCentralWidget(m_stack);

    connect(m_connectionListPage, &ConnectionListPage::connected, this, &MainWindow::onConnected);
    connect(m_hubListPage, &HubListPage::disconnectRequested, this, &MainWindow::onDisconnectRequested);

    auto *fileMenu = menuBar()->addMenu(tr("ファイル(&F)"));
    QAction *quitAction = fileMenu->addAction(tr("終了(&X)"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnected(VpnServerRpc *rpc, const QJsonObject &serverInfo)
{
    m_hubListPage->setConnection(rpc, serverInfo);
    m_stack->setCurrentWidget(m_hubListPage);
}

void MainWindow::onDisconnectRequested()
{
    m_stack->setCurrentWidget(m_connectionListPage);
}
