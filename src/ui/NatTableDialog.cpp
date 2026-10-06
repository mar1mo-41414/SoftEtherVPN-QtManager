#include "NatTableDialog.h"

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

NatTableDialog::NatTableDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_NM_NAT CAPTION
    setWindowTitle(tr("仮想 NAT ルータ上の NAT セッションテーブル"));

    auto *titleLabel = new QLabel(
        tr("現在、SecureNAT の仮想 NAT ルータ上に以下の TCP または UDP の NAT テーブルエントリがあります。"), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    // NM_NAT_* の並び
    m_table->setHorizontalHeaderLabels({tr("ID"), tr("プロトコル"), tr("接続元ホスト"), tr("接続元ポート"), tr("接続先ホスト"),
                                         tr("接続先ポート"), tr("セッション作成日時"), tr("最終通信時刻"),
                                         tr("受信 / 送信サイズ"), tr("TCP 接続状態")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(refreshButton, &QPushButton::clicked, this, &NatTableDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(700, 400);
    reload();
}

void NatTableDialog::reload()
{
    m_rpc->enumNAT(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray natTable = result.value("NatTable").toArray();
            m_table->setRowCount(natTable.size());
            for (int row = 0; row < natTable.size(); ++row) {
                const QJsonObject entry = natTable.at(row).toObject();
                const auto host = [](const QJsonObject &e, const char *hostKey, const char *ipKey) {
                    const QString name = e.value(hostKey).toString();
                    return name.isEmpty() ? e.value(ipKey).toString() : name;
                };
                // NAT_TCP_CONNECTING〜 (TCP のみ意味を持つ)
                QString tcpStatus;
                if (entry.value("Protocol_u32").toInt() == 0) {
                    static const char *kStatus[] = {"接続中", "切断中", "接続完了", "通信中", "切断中"};
                    const int st = entry.value("TcpStatus_u32").toInt();
                    tcpStatus = (st >= 0 && st < 5) ? tr(kStatus[st]) : QString::number(st);
                }
                m_table->setItem(row, 0, new QTableWidgetItem(QString::number(entry.value("Id_u32").toInt())));
                m_table->setItem(row, 1, new QTableWidgetItem(SoftEtherLabels::natProtocolName(entry.value("Protocol_u32").toInt())));
                m_table->setItem(row, 2, new QTableWidgetItem(host(entry, "SrcHost_str", "SrcIp_ip")));
                m_table->setItem(row, 3, new QTableWidgetItem(QString::number(entry.value("SrcPort_u32").toInt())));
                m_table->setItem(row, 4, new QTableWidgetItem(host(entry, "DestHost_str", "DestIp_ip")));
                m_table->setItem(row, 5, new QTableWidgetItem(QString::number(entry.value("DestPort_u32").toInt())));
                m_table->setItem(row, 6, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("CreatedTime_dt").toString())));
                m_table->setItem(row, 7, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("LastCommTime_dt").toString())));
                m_table->setItem(row, 8, new QTableWidgetItem(tr("%1 / %2")
                                                                   .arg(SoftEtherLabels::bytes(entry.value("RecvSize_u64").toDouble()))
                                                                   .arg(SoftEtherLabels::bytes(entry.value("SendSize_u64").toDouble()))));
                m_table->setItem(row, 9, new QTableWidgetItem(tcpStatus));
            }
            m_table->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("NAT テーブルの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
