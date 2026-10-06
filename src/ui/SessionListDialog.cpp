#include "SessionListDialog.h"
#include "IpTableDialog.h"
#include "MacTableDialog.h"
#include "SessionStatusDialog.h"

#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

SessionListDialog::SessionListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_SESSION CAPTION
    setWindowTitle(tr("セッションの管理 - %1").arg(m_hubName));

    // S_TITLE
    auto *titleLabel = new QLabel(tr("現在、仮想 HUB \"%1\" に以下のセッションが接続しています。").arg(m_hubName), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    // SM_SESS_COLUMN_1〜8
    // 公式Managerの列順: セッション名 / VLAN ID / 場所 / ユーザー名 / 接続元ホスト名 / TCP コネクション / 転送バイト数 / 転送パケット数
    m_table->setHorizontalHeaderLabels({tr("セッション名"), tr("VLAN ID"), tr("場所"), tr("ユーザー名"),
                                         tr("接続元ホスト名"), tr("TCP コネクション"), tr("転送バイト数"),
                                         tr("転送パケット数")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &SessionListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &SessionListDialog::onShowStatus);

    // STATIC1: セッションに関する操作
    m_statusButton = new QPushButton(tr("セッションの情報を表示(&I)"), this);
    m_disconnectButton = new QPushButton(tr("切断(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    m_sessionMacButton = new QPushButton(tr("このセッションの MAC テーブル(&M)"), this);
    m_sessionIpButton = new QPushButton(tr("このセッションの IP テーブル(&P)"), this);
    connect(m_statusButton, &QPushButton::clicked, this, &SessionListDialog::onShowStatus);
    connect(m_disconnectButton, &QPushButton::clicked, this, &SessionListDialog::onDisconnect);
    connect(refreshButton, &QPushButton::clicked, this, &SessionListDialog::reload);
    connect(m_sessionMacButton, &QPushButton::clicked, this, &SessionListDialog::onSessionMacTable);
    connect(m_sessionIpButton, &QPushButton::clicked, this, &SessionListDialog::onSessionIpTable);

    auto *opGroup = new QGroupBox(tr("セッションに関する操作"), this);
    auto *opLayout = new QGridLayout(opGroup);
    opLayout->addWidget(m_statusButton, 0, 0);
    opLayout->addWidget(m_disconnectButton, 0, 1);
    opLayout->addWidget(refreshButton, 0, 2);
    opLayout->addWidget(m_sessionMacButton, 0, 3);
    opLayout->addWidget(m_sessionIpButton, 0, 4);

    // STATIC2: その他の管理タスク
    auto *macTableButton = new QPushButton(tr("MAC アドレステーブル一覧(&A)"), this);
    auto *ipTableButton = new QPushButton(tr("IP アドレステーブル一覧(&B)"), this);
    connect(macTableButton, &QPushButton::clicked, this, &SessionListDialog::onMacTable);
    connect(ipTableButton, &QPushButton::clicked, this, &SessionListDialog::onIpTable);

    auto *otherGroup = new QGroupBox(tr("その他の管理タスク"), this);
    auto *otherLayout = new QHBoxLayout(otherGroup);
    otherLayout->addWidget(macTableButton);
    otherLayout->addWidget(ipTableButton);

    // IDCANCEL
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(otherGroup);
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton, 0, Qt::AlignBottom);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addWidget(opGroup);
    layout->addLayout(bottomLayout);

    resize(960, 520);
    onSelectionChanged();
    reload();
}

QString SessionListDialog::selectedSessionName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return m_table->item(selected.first()->row(), 0)->text();
}

void SessionListDialog::onSelectionChanged()
{
    const bool hasSelection = !selectedSessionName().isEmpty();
    m_statusButton->setEnabled(hasSelection);
    m_disconnectButton->setEnabled(hasSelection);
    m_sessionMacButton->setEnabled(hasSelection);
    m_sessionIpButton->setEnabled(hasSelection);
}

void SessionListDialog::reload()
{
    m_rpc->enumSession(
        m_hubName,
        [this](const QJsonObject &result) {
            const QJsonArray sessionList = result.value("SessionList").toArray();
            m_table->setRowCount(sessionList.size());
            for (int row = 0; row < sessionList.size(); ++row) {
                const QJsonObject session = sessionList.at(row).toObject();

                const QString location = SoftEtherLabels::sessionLocation(
                    session.value("LinkMode_bool").toBool(), session.value("SecureNATMode_bool").toBool(),
                    session.value("BridgeMode_bool").toBool(), session.value("Layer3Mode_bool").toBool(),
                    session.value("RemoteSession_bool").toBool(), session.value("RemoteHostname_str").toString());
                const QString tcp = tr("%1 / %2")
                                         .arg(session.value("CurrentNumTcp_u32").toInt())
                                         .arg(session.value("MaxNumTcp_u32").toInt());

                const int vlan = session.value("VLanId_u32").toInt();
                m_table->setItem(row, 0, new QTableWidgetItem(session.value("Name_str").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(vlan == 0 ? tr("－") : QString::number(vlan)));
                m_table->setItem(row, 2, new QTableWidgetItem(location));
                m_table->setItem(row, 3, new QTableWidgetItem(session.value("Username_str").toString()));
                m_table->setItem(row, 4, new QTableWidgetItem(session.value("Hostname_str").toString()));
                m_table->setItem(row, 5, new QTableWidgetItem(tcp));
                m_table->setItem(row, 6, new QTableWidgetItem(SoftEtherLabels::number(session.value("PacketSize_u64").toDouble())));
                m_table->setItem(row, 7, new QTableWidgetItem(SoftEtherLabels::number(session.value("PacketNum_u64").toDouble())));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("セッション一覧の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SessionListDialog::onShowStatus()
{
    const QString sessionName = selectedSessionName();
    if (sessionName.isEmpty()) {
        return;
    }
    m_rpc->getSessionStatus(
        m_hubName, sessionName,
        [this, sessionName](const QJsonObject &status) {
            auto *dialog = new SessionStatusDialog(sessionName, status, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("セッション情報の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SessionListDialog::onDisconnect()
{
    const QString sessionName = selectedSessionName();
    if (sessionName.isEmpty()) {
        return;
    }

    // SM_SESS_DISCONNECT_MSG
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("セッション \"%1\" を切断します。よろしいですか?").arg(sessionName), QMessageBox::NoButton,
                            this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->deleteSession(
        m_hubName, sessionName, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("セッションの切断に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void SessionListDialog::onSessionMacTable()
{
    const QString sessionName = selectedSessionName();
    if (sessionName.isEmpty()) {
        return;
    }
    MacTableDialog dialog(m_rpc, m_hubName, sessionName, this);
    dialog.exec();
}

void SessionListDialog::onSessionIpTable()
{
    const QString sessionName = selectedSessionName();
    if (sessionName.isEmpty()) {
        return;
    }
    IpTableDialog dialog(m_rpc, m_hubName, sessionName, this);
    dialog.exec();
}

void SessionListDialog::onMacTable()
{
    MacTableDialog dialog(m_rpc, m_hubName, QString(), this);
    dialog.exec();
}

void SessionListDialog::onIpTable()
{
    IpTableDialog dialog(m_rpc, m_hubName, QString(), this);
    dialog.exec();
}
