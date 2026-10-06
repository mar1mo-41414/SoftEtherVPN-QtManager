#include "TcpConnectionListDialog.h"
#include "InfoTableDialog.h"

#include "util/RpcUiHelpers.h"
#include "util/SoftEtherLabels.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QString connectionTypeText(int type)
{
    // SM_CONNECTION_TYPE_0〜9
    static const char *kTypes[] = {"クライアント", "初期化中", "ログイン", "追加接続", "クラスタリング RPC",
                                   "管理用 RPC", "HUB 列挙 RPC", "パスワード変更", "MS-SSTP 接続", "OpenVPN 接続"};
    if (type >= 0 && type < 10) {
        return QCoreApplication::translate("TcpConnectionListDialog", kTypes[type]);
    }
    return QString::number(type);
}

} // namespace

TcpConnectionListDialog::TcpConnectionListDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_CONNECTION CAPTION / S_TITLE
    setWindowTitle(tr("コネクション一覧"));

    auto *titleLabel = new QLabel(
        tr("現在、サーバー %1 に接続中のコネクションは以下のとおりです。ただし、VPN セッションが確立済みのコネクションは表示されません。")
            .arg(serverName),
        this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    // SM_CONN_COLUMN_1〜4
    m_table->setHorizontalHeaderLabels({tr("コネクション名"), tr("接続元"), tr("接続時刻"), tr("種類")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TcpConnectionListDialog::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &TcpConnectionListDialog::onShowInfo);

    // IDOK / B_DISCONNECT / B_REFRESH / IDCANCEL
    m_infoButton = new QPushButton(tr("コネクションの情報を表示(&I)"), this);
    m_disconnectButton = new QPushButton(tr("切断(&D)"), this);
    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(m_infoButton, &QPushButton::clicked, this, &TcpConnectionListDialog::onShowInfo);
    connect(m_disconnectButton, &QPushButton::clicked, this, &TcpConnectionListDialog::onDisconnect);
    connect(refreshButton, &QPushButton::clicked, this, &TcpConnectionListDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_infoButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttonLayout);

    resize(700, 460);
    onSelectionChanged();
    reload();
}

QString TcpConnectionListDialog::selectedName() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    return selected.isEmpty() ? QString() : m_table->item(selected.first()->row(), 0)->text();
}

void TcpConnectionListDialog::onSelectionChanged()
{
    const bool hasSelection = !selectedName().isEmpty();
    m_infoButton->setEnabled(hasSelection);
    m_disconnectButton->setEnabled(hasSelection);
}

void TcpConnectionListDialog::reload()
{
    m_rpc->call(
        QStringLiteral("EnumConnection"), {},
        [this](const QJsonObject &result) {
            const QJsonArray list = result.value("ConnectionList").toArray();
            m_table->setRowCount(list.size());
            for (int row = 0; row < list.size(); ++row) {
                const QJsonObject item = list.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(item.value("Name_str").toString()));
                m_table->setItem(row, 1,
                                  new QTableWidgetItem(QStringLiteral("%1:%2")
                                                           .arg(item.value("Hostname_str").toString())
                                                           .arg(item.value("Port_u32").toInt())));
                m_table->setItem(row, 2, new QTableWidgetItem(SoftEtherLabels::dateTime(item.value("ConnectedTime_dt").toString())));
                m_table->setItem(row, 3, new QTableWidgetItem(connectionTypeText(item.value("Type_u32").toInt())));
            }
            m_table->resizeColumnsToContents();
            onSelectionChanged();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("コネクション一覧の取得"), error); });
}

void TcpConnectionListDialog::onShowInfo()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    auto *dialog = new InfoTableDialog(
        tr("コネクション %1 の情報").arg(name), tr("コネクションの情報"), /*refreshable=*/false,
        [rpc = m_rpc, name](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
            QJsonObject params;
            params["Name_str"] = name;
            rpc->call(
                QStringLiteral("GetConnectionInfo"), params,
                [deliver](const QJsonObject &info) {
                    InfoTable::Rows rows;
                    // SM_CONNINFO_*
                    rows << qMakePair(tr("コネクション名"), info.value("Name_str").toString());
                    rows << qMakePair(tr("コネクションの種類"), connectionTypeText(info.value("Type_u32").toInt()));
                    rows << qMakePair(tr("クライアントホスト名"), info.value("Hostname_str").toString());
                    rows << qMakePair(tr("クライアント IP アドレス"), info.value("Ip_ip").toString());
                    rows << qMakePair(tr("クライアントポート番号 (TCP)"), QString::number(info.value("Port_u32").toInt()));
                    rows << qMakePair(tr("接続時刻"), SoftEtherLabels::dateTime(info.value("ConnectedTime_dt").toString()));
                    rows << qMakePair(tr("サーバー製品名"), info.value("ServerStr_str").toString());
                    rows << qMakePair(tr("サーバー バージョン"), QString::number(info.value("ServerVer_u32").toInt()));
                    rows << qMakePair(tr("サーバー ビルド番号"), QString::number(info.value("ServerBuild_u32").toInt()));
                    rows << qMakePair(tr("クライアント製品名"), info.value("ClientStr_str").toString());
                    rows << qMakePair(tr("クライアントバージョン"), QString::number(info.value("ClientVer_u32").toInt()));
                    rows << qMakePair(tr("クライアントビルド番号"), QString::number(info.value("ClientBuild_u32").toInt()));
                    deliver(rows);
                },
                [fail](const RpcError &error) { fail(error); });
        },
        this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void TcpConnectionListDialog::onDisconnect()
{
    const QString name = selectedName();
    if (name.isEmpty()) {
        return;
    }
    // SM_CONN_DISCONNECT_MSG
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"), tr("コネクション %1 を切断します。\nよろしいですか?").arg(name),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }
    QJsonObject params;
    params["Name_str"] = name;
    m_rpc->call(
        QStringLiteral("DisconnectConnection"), params, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("コネクションの切断"), error); });
}
