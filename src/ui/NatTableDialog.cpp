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
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {tr("プロトコル"), tr("送信元"), tr("宛先"), tr("送受信バイト数"), tr("作成時刻"), tr("最終通信時刻")});
    m_table->horizontalHeader()->setStretchLastSection(true);
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
                const QString src = QStringLiteral("%1:%2").arg(entry.value("SrcIp_ip").toString()).arg(entry.value("SrcPort_u32").toInt());
                const QString dst = QStringLiteral("%1:%2").arg(entry.value("DestIp_ip").toString()).arg(entry.value("DestPort_u32").toInt());
                const QString size = tr("送信 %1 / 受信 %2")
                                          .arg(QString::number(entry.value("SendSize_u64").toDouble(), 'f', 0))
                                          .arg(QString::number(entry.value("RecvSize_u64").toDouble(), 'f', 0));

                m_table->setItem(row, 0, new QTableWidgetItem(SoftEtherLabels::natProtocolName(entry.value("Protocol_u32").toInt())));
                m_table->setItem(row, 1, new QTableWidgetItem(src));
                m_table->setItem(row, 2, new QTableWidgetItem(dst));
                m_table->setItem(row, 3, new QTableWidgetItem(size));
                m_table->setItem(row, 4, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("CreatedTime_dt").toString())));
                m_table->setItem(row, 5, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("LastCommTime_dt").toString())));
            }
            m_table->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("NAT テーブルの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
