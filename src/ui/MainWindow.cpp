#include "MainWindow.h"
#include "ConnectDialog.h"

#include <QAction>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>

namespace {

// SM_HUB_ONLINE / SM_HUB_OFFLINE (strtable_ja.stb)
QString onlineStatusText(bool online)
{
    return online ? MainWindow::tr("オンライン") : MainWindow::tr("オフライン");
}

// SM_HUB_STANDALONE / SM_HUB_STATIC / SM_HUB_DYNAMIC (strtable_ja.stb)
QString hubTypeText(int hubType)
{
    switch (hubType) {
    case 0:
        return MainWindow::tr("スタンドアロン");
    case 1:
        return MainWindow::tr("スタティック仮想 HUB");
    case 2:
        return MainWindow::tr("ダイナミック仮想 HUB");
    default:
        return MainWindow::tr("不明");
    }
}

QString formatDateTime(const QString &isoString)
{
    const QDateTime dt = QDateTime::fromString(isoString, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return dt.toLocalTime().toString(QStringLiteral("yyyy/MM/dd HH:mm:ss"));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SoftEtherVPN-QtManager"));
    resize(900, 560);

    // 公式Managerの列構成 (SM_HUB_COLUMN_1〜11) を踏襲。
    m_hubTable = new QTableWidget(this);
    m_hubTable->setColumnCount(9);
    m_hubTable->setHorizontalHeaderLabels({
        tr("仮想 HUB 名"),
        tr("状態"),
        tr("種類"),
        tr("ユーザー"),
        tr("グループ"),
        tr("セッション"),
        tr("MAC テーブル"),
        tr("IP テーブル"),
        tr("ログイン回数"),
    });
    m_hubTable->horizontalHeader()->setStretchLastSection(true);
    m_hubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    setCentralWidget(m_hubTable);

    m_serverInfoLabel = new QLabel(this);
    statusBar()->addWidget(m_serverInfoLabel);

    auto *fileMenu = menuBar()->addMenu(tr("ファイル(&F)"));
    QAction *connectAction = fileMenu->addAction(tr("接続(&C)..."));
    connect(connectAction, &QAction::triggered, this, &MainWindow::showConnectDialog);
    QAction *refreshAction = fileMenu->addAction(tr("再読み込み(&R)"));
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshHubList);
    fileMenu->addSeparator();
    QAction *quitAction = fileMenu->addAction(tr("終了(&Q)"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

MainWindow::~MainWindow() = default;

void MainWindow::setConnection(VpnServerRpc *rpc, const QJsonObject &serverInfo)
{
    if (m_rpc) {
        m_rpc->deleteLater();
    }
    m_rpc = rpc;
    m_rpc->setParent(this);

    applyServerInfo(serverInfo);
    refreshHubList();
}

void MainWindow::applyServerInfo(const QJsonObject &info)
{
    const QString productName = info.value("ServerProductName_str").toString();
    const QString version = info.value("ServerVersionString_str").toString();
    const QString hostName = info.value("ServerHostName_str").toString();
    m_serverInfoLabel->setText(tr("接続先: %1 (%2 %3)").arg(hostName, productName, version));
}

void MainWindow::showConnectDialog()
{
    ConnectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        setConnection(dialog.takeConnectedRpc(), dialog.serverInfo());
    }
}

void MainWindow::refreshHubList()
{
    if (!m_rpc) {
        return;
    }

    m_rpc->enumHub(
        [this](const QJsonObject &result) {
            const QJsonArray hubList = result.value("HubList").toArray();
            m_hubTable->setRowCount(hubList.size());

            for (int row = 0; row < hubList.size(); ++row) {
                const QJsonObject hub = hubList.at(row).toObject();

                auto *nameItem = new QTableWidgetItem(hub.value("HubName_str").toString());
                auto *statusItem = new QTableWidgetItem(onlineStatusText(hub.value("Online_bool").toBool()));
                auto *typeItem = new QTableWidgetItem(hubTypeText(hub.value("HubType_u32").toInt()));
                auto *usersItem = new QTableWidgetItem(QString::number(hub.value("NumUsers_u32").toInt()));
                auto *groupsItem = new QTableWidgetItem(QString::number(hub.value("NumGroups_u32").toInt()));
                auto *sessionsItem = new QTableWidgetItem(QString::number(hub.value("NumSessions_u32").toInt()));
                auto *macItem = new QTableWidgetItem(QString::number(hub.value("NumMacTables_u32").toInt()));
                auto *ipItem = new QTableWidgetItem(QString::number(hub.value("NumIpTables_u32").toInt()));
                auto *loginItem = new QTableWidgetItem(QString::number(hub.value("NumLogin_u32").toInt()));

                m_hubTable->setItem(row, 0, nameItem);
                m_hubTable->setItem(row, 1, statusItem);
                m_hubTable->setItem(row, 2, typeItem);
                m_hubTable->setItem(row, 3, usersItem);
                m_hubTable->setItem(row, 4, groupsItem);
                m_hubTable->setItem(row, 5, sessionsItem);
                m_hubTable->setItem(row, 6, macItem);
                m_hubTable->setItem(row, 7, ipItem);
                m_hubTable->setItem(row, 8, loginItem);
            }

            m_hubTable->resizeColumnsToContents();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("仮想 HUB 一覧の取得に失敗しました: %1 (code %2)")
                                      .arg(error.message)
                                      .arg(error.code));
        });
}
