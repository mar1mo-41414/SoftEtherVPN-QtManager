#include "MainWindow.h"
#include "ConnectionListPage.h"
#include "HubListPage.h"
#include "HubManagementPage.h"

#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SoftEtherVPN-QtManager"));

    m_connectionListPage = new ConnectionListPage(this);
    m_hubListPage = new HubListPage(this);
    m_hubManagementPage = new HubManagementPage(this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_connectionListPage);
    m_stack->addWidget(m_hubListPage);
    m_stack->addWidget(m_hubManagementPage);
    setCentralWidget(m_stack);

    connect(m_connectionListPage, &ConnectionListPage::connected, this, &MainWindow::onConnected);
    connect(m_connectionListPage, &ConnectionListPage::quitRequested, this, &QMainWindow::close);
    // 公式Managerは画面ごとに独立した大きさのウィンドウなので、ページ切り替え時に大きさを合わせる。
    connect(m_stack, &QStackedWidget::currentChanged, this, &MainWindow::fitToCurrentPage);
    fitToCurrentPage();
    connect(m_hubListPage, &HubListPage::disconnectRequested, this, &MainWindow::onDisconnectRequested);
    connect(m_hubListPage, &HubListPage::manageHubRequested, this, &MainWindow::onManageHubRequested);
    connect(m_hubManagementPage, &HubManagementPage::backRequested, this, &MainWindow::onHubManagementBackRequested);

}

MainWindow::~MainWindow() = default;

void MainWindow::fitToCurrentPage()
{
    QWidget *page = m_stack->currentWidget();
    if (page == m_connectionListPage) {
        resize(440, 600);
    } else if (page == m_hubListPage) {
        resize(820, 700);
    } else if (page == m_hubManagementPage) {
        resize(820, 640);
    }
    updateTitle();
}

void MainWindow::updateTitle()
{
    QWidget *page = m_stack->currentWidget();
    if (page == m_hubListPage) {
        setWindowTitle(tr("%1 - SoftEtherVPN-QtManager").arg(m_settingName));
    } else if (page == m_hubManagementPage) {
        setWindowTitle(tr("仮想 HUB の管理 - %1").arg(m_currentHubName));
    } else {
        setWindowTitle(tr("SoftEtherVPN-QtManager"));
    }
}

void MainWindow::onConnected(VpnServerRpc *rpc, const QJsonObject &serverInfo, const ConnectionProfile &profile)
{
    const bool hubAdminMode = profile.hubAdminMode;
    const QString hubName = profile.hubName;
    m_settingName = profile.name;
    m_currentHubName = hubName;
    // rpcの所有者はHubListPageに統一する (仮想HUB管理モードで直接HubManagementPageに
    // 入る場合も、rpc自体はHubListPageに持たせて借用させる)。
    m_hubListPage->setConnection(rpc, serverInfo, profile);

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
    m_currentHubName = hubName;
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
