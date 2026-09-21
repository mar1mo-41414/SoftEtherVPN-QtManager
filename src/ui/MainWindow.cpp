#include "MainWindow.h"
#include "ConnectionListPage.h"
#include "HubListPage.h"
#include "HubManagementPage.h"

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
    m_hubManagementPage = new HubManagementPage(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_connectionListPage);
    m_stack->addWidget(m_hubListPage);
    m_stack->addWidget(m_hubManagementPage);
    setCentralWidget(m_stack);

    connect(m_connectionListPage, &ConnectionListPage::connected, this, &MainWindow::onConnected);
    connect(m_hubListPage, &HubListPage::disconnectRequested, this, &MainWindow::onDisconnectRequested);
    connect(m_hubListPage, &HubListPage::manageHubRequested, this, &MainWindow::onManageHubRequested);
    connect(m_hubManagementPage, &HubManagementPage::backRequested, this, &MainWindow::onHubManagementBackRequested);

    auto *fileMenu = menuBar()->addMenu(tr("ファイル(&F)"));
    QAction *quitAction = fileMenu->addAction(tr("終了(&X)"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnected(VpnServerRpc *rpc, const QJsonObject &serverInfo, bool hubAdminMode, const QString &hubName)
{
    // rpcの所有者はHubListPageに統一する (仮想HUB管理モードで直接HubManagementPageに
    // 入る場合も、rpc自体はHubListPageに持たせて借用させる)。
    m_hubListPage->setConnection(rpc, serverInfo, hubAdminMode);

    if (hubAdminMode) {
        m_hubManagementEnteredDirectly = true;
        m_hubManagementPage->setContext(rpc, hubName);
        m_stack->setCurrentWidget(m_hubManagementPage);
    } else {
        m_hubManagementEnteredDirectly = false;
        m_stack->setCurrentWidget(m_hubListPage);
    }
}

void MainWindow::onDisconnectRequested()
{
    m_stack->setCurrentWidget(m_connectionListPage);
}

void MainWindow::onManageHubRequested(VpnServerRpc *rpc, const QString &hubName)
{
    m_hubManagementEnteredDirectly = false;
    m_hubManagementPage->setContext(rpc, hubName);
    m_stack->setCurrentWidget(m_hubManagementPage);
}

void MainWindow::onHubManagementBackRequested()
{
    if (m_hubManagementEnteredDirectly) {
        m_stack->setCurrentWidget(m_connectionListPage);
    } else {
        m_stack->setCurrentWidget(m_hubListPage);
    }
}
