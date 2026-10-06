#include "DhcpTableDialog.h"

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

DhcpTableDialog::DhcpTableDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_NM_DHCP CAPTION
    setWindowTitle(tr("仮想 DHCP サーバー上の IP リーステーブル"));

    auto *titleLabel =
        new QLabel(tr("現在、SecureNAT の仮想 DHCP サーバーは以下の IP アドレスをクライアントに配布しています。"), this);
    titleLabel->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("IP アドレス"), tr("MAC アドレス"), tr("ホスト名"), tr("リース時刻"), tr("期限")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(refreshButton, &QPushButton::clicked, this, &DhcpTableDialog::reload);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(m_table);
    layout->addLayout(buttonLayout);

    resize(680, 400);
    reload();
}

void DhcpTableDialog::reload()
{
    m_rpc->enumDHCP(
        m_hubName,
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const QJsonArray dhcpTable = result.value("DhcpTable").toArray();
            m_table->setRowCount(dhcpTable.size());
            for (int row = 0; row < dhcpTable.size(); ++row) {
                const QJsonObject entry = dhcpTable.at(row).toObject();
                m_table->setItem(row, 0, new QTableWidgetItem(entry.value("IpAddress_ip").toString()));
                m_table->setItem(row, 1, new QTableWidgetItem(SoftEtherLabels::macAddress(entry.value("MacAddress_bin").toString())));
                m_table->setItem(row, 2, new QTableWidgetItem(entry.value("Hostname_str").toString()));
                m_table->setItem(row, 3, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("LeasedTime_dt").toString())));
                m_table->setItem(row, 4, new QTableWidgetItem(SoftEtherLabels::dateTime(entry.value("ExpireTime_dt").toString())));
            }
            m_table->resizeColumnsToContents();
        }),
        RpcUi::guarded(this, [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("DHCP リーステーブルの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        }));
}
