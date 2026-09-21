#include "SessionStatusDialog.h"

#include "util/SoftEtherLabels.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SessionStatusDialog::SessionStatusDialog(const QString &sessionName, const QJsonObject &status, QWidget *parent)
    : QDialog(parent)
{
    // SM_SESS_STATUS_CAPTION
    setWindowTitle(tr("VPN セッション \"%1\" の状況").arg(sessionName));

    auto *form = new QFormLayout;
    // SM_SESS_STATUS_USERNAME/REALUSER/GROUPNAME
    form->addRow(tr("ユーザー名 (認証):"), new QLabel(status.value("Username_str").toString(), this));
    form->addRow(tr("ユーザー名 (データベース):"), new QLabel(status.value("RealUsername_str").toString(), this));
    form->addRow(tr("グループ名:"), new QLabel(status.value("GroupName_str").toString(), this));
    // SM_CLIENT_IP/SM_CLIENT_HOSTNAME
    form->addRow(tr("クライアント IP アドレス:"), new QLabel(status.value("Client_Ip_Address_ip").toString(), this));
    form->addRow(tr("クライアントホスト名:"),
                 new QLabel(status.value("SessionStatus_ClientHostName_str").toString(), this));
    // SM_NODE_CLIENT_NAME/VER, SM_NODE_CLIENT_OS_NAME/VER
    form->addRow(tr("クライアント製品名 (申告):"), new QLabel(status.value("ClientProductName_str").toString(), this));
    form->addRow(tr("クライアントバージョン (申告):"),
                 new QLabel(QString::number(status.value("ClientProductVer_u32").toInt()), this));
    form->addRow(tr("クライアント OS 名 (申告):"), new QLabel(status.value("ClientOsName_str").toString(), this));
    form->addRow(tr("クライアント OS バージョン (申告):"), new QLabel(status.value("ClientOsVer_str").toString(), this));
    form->addRow(tr("接続開始時刻:"), new QLabel(SoftEtherLabels::dateTime(status.value("StartTime_dt").toString()), this));
    form->addRow(tr("現在の接続確立時刻:"),
                 new QLabel(SoftEtherLabels::dateTime(status.value("CurrentConnectionEstablishTime_dt").toString()), this));
    form->addRow(tr("TCP コネクション数:"), new QLabel(QString::number(status.value("NumTcpConnections_u32").toInt()), this));
    form->addRow(tr("暗号化アルゴリズム:"), new QLabel(status.value("CipherName_str").toString(), this));
    form->addRow(tr("送信バイト数 (合計):"), new QLabel(QString::number(status.value("TotalSendSize_u64").toDouble(), 'f', 0), this));
    form->addRow(tr("受信バイト数 (合計):"), new QLabel(QString::number(status.value("TotalRecvSize_u64").toDouble(), 'f', 0), this));

    // D_SM_STATUS IDCANCEL
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
}
