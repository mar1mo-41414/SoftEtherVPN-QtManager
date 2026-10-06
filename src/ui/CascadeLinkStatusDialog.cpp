#include "CascadeLinkStatusDialog.h"
#include "InfoTableDialog.h"

#include "util/SoftEtherLabels.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString yesNo(bool value)
{
    // CM_ST_YES / CM_ST_NO
    return value ? CascadeLinkStatusDialog::tr("はい") : CascadeLinkStatusDialog::tr("いいえ");
}

QString sessionStatusText(int status)
{
    // CM_ST_CONNECTING / NEGOTIATION / AUTH / ESTABLISHED / RETRY / IDLE
    switch (status) {
    case 0:
        return CascadeLinkStatusDialog::tr("VPN サーバーに接続開始中");
    case 1:
        return CascadeLinkStatusDialog::tr("ネゴシエーション中");
    case 2:
        return CascadeLinkStatusDialog::tr("ユーザー認証中");
    case 3:
        return CascadeLinkStatusDialog::tr("コネクション確立済み");
    case 4:
        return CascadeLinkStatusDialog::tr("再試行中");
    default:
        return CascadeLinkStatusDialog::tr("アイドル状態");
    }
}

} // namespace

CascadeLinkStatusDialog::CascadeLinkStatusDialog(const QString &accountName, const QJsonObject &s, QWidget *parent)
    : QDialog(parent)
{
    using SoftEtherLabels::bytes;
    using SoftEtherLabels::dateTime;
    using SoftEtherLabels::number;

    // CM_ST_TITLE
    setWindowTitle(tr("%1 の接続状況").arg(accountName));
    auto *heading = new QLabel(tr("%1 の接続状況").arg(accountName), this);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    headingFont.setPointSize(headingFont.pointSize() + 4);
    heading->setFont(headingFont);

    const bool connected = s.value("Connected_bool").toBool();
    InfoTable::Rows rows;
    rows << qMakePair(tr("接続設定名"), s.value("AccountName_utf").toString());
    rows << qMakePair(tr("セッション接続状態"),
                      connected ? tr("接続完了 (セッション確立済み)") : sessionStatusText(s.value("SessionStatus_u32").toInt()));
    if (connected) {
        rows << qMakePair(tr("サーバー名"), s.value("ServerName_str").toString());
        rows << qMakePair(tr("ポート番号"), tr("TCP ポート %1").arg(s.value("ServerPort_u32").toInt()));
        rows << qMakePair(tr("サーバー製品名"), s.value("ServerProductName_str").toString());
        rows << qMakePair(tr("サーバーバージョン"), QString::number(s.value("ServerProductVer_u32").toInt() / 100.0, 'f', 2));
        rows << qMakePair(tr("サーバービルド番号"), QString::number(s.value("ServerProductBuild_u32").toInt()));
    }
    rows << qMakePair(tr("接続開始時刻"), dateTime(s.value("StartTime_dt").toString()));
    if (connected) {
        rows << qMakePair(tr("初回セッションの確立時刻"), dateTime(s.value("FirstConnectionEstablisiedTime_dt").toString()));
        rows << qMakePair(tr("現在のセッションの確立時刻"), dateTime(s.value("CurrentConnectionEstablishTime_dt").toString()));
        rows << qMakePair(tr("セッション確立回数"), tr("%1 回").arg(s.value("NumConnectionsEatablished_u32").toInt()));
        const bool half = s.value("HalfConnection_bool").toBool();
        rows << qMakePair(tr("半二重 TCP コネクションモード"), half ? tr("はい (半二重モード)") : tr("いいえ (全二重モード)"));
        rows << qMakePair(tr("VoIP / QoS 対応機能"), s.value("QoS_bool").toBool() ? tr("有効 (使用中)") : tr("無効"));
        rows << qMakePair(tr("TCP コネクション数"), QString::number(s.value("NumTcpConnections_u32").toInt()));
        if (half) {
            rows << qMakePair(tr("上り方向 TCP コネクション数"), QString::number(s.value("NumTcpConnectionsUpload_u32").toInt()));
            rows << qMakePair(tr("下り方向 TCP コネクション数"), QString::number(s.value("NumTcpConnectionsDownload_u32").toInt()));
        }
        rows << qMakePair(tr("TCP コネクション数最大値"), QString::number(s.value("MaxTcpConnections_u32").toInt()));
        const int vlan = s.value("VLanId_u32").toInt();
        rows << qMakePair(tr("VLAN ID"), vlan == 0 ? tr("－") : QString::number(vlan));
        rows << qMakePair(tr("暗号化の使用"),
                          s.value("UseEncrypt_bool").toBool()
                              ? tr("はい (暗号化アルゴリズム: %1)").arg(s.value("CipherName_str").toString())
                              : tr("いいえ (暗号化なし)"));
        rows << qMakePair(tr("圧縮の使用"), s.value("UseCompress_bool").toBool() ? tr("はい") : tr("いいえ (圧縮無し)"));
        rows << qMakePair(tr("UDP 高速化機能をサポート"), yesNo(s.value("IsUdpAccelerationEnabled_bool").toBool()));
        rows << qMakePair(tr("UDP 高速化機能を使用中"), yesNo(s.value("IsUsingUdpAcceleration_bool").toBool()));
        rows << qMakePair(tr("TCP over UDP (NAT Traversal)"), yesNo(s.value("IsRUDPSession_bool").toBool()));
        rows << qMakePair(tr("物理通信に使用中のプロトコル"), s.value("UnderlayProtocol_str").toString());
        rows << qMakePair(tr("セッション名"), s.value("SessionName_str").toString());
        rows << qMakePair(tr("コネクション名"), s.value("ConnectionName_str").toString());
        rows << qMakePair(tr("セッションキー (160bit)"),
                          QString::fromLatin1(QByteArray::fromBase64(s.value("SessionKey_bin").toString().toLatin1()).toHex().toUpper()));
        rows << qMakePair(tr("ブリッジ / ルータモード"), yesNo(s.value("IsBridgeMode_bool").toBool()));
        rows << qMakePair(tr("モニタリングモード"), yesNo(s.value("IsMonitorMode_bool").toBool()));
        rows << qMakePair(tr("送信データサイズ"), bytes(s.value("TotalSendSize_u64").toDouble()));
        rows << qMakePair(tr("受信データサイズ"), bytes(s.value("TotalRecvSize_u64").toDouble()));
    }

    auto *table = InfoTable::makeTable(this);
    // CM_ST_COLUMN_1 / CM_ST_COLUMN_2
    table->setHorizontalHeaderLabels({tr("項目名"), tr("状況")});
    InfoTable::setRows(table, rows);

    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(heading);
    layout->addWidget(table, 1);
    layout->addLayout(buttonLayout);
    resize(560, 560);
}
