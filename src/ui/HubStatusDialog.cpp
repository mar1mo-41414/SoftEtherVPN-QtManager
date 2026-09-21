#include "HubStatusDialog.h"

#include "util/SoftEtherLabels.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

HubStatusDialog::HubStatusDialog(const QString &hubName, const QJsonObject &status, QWidget *parent)
    : QDialog(parent)
{
    // SM_HUB_STATUS_CAPTION
    setWindowTitle(tr("仮想 HUB \"%1\" の状態").arg(hubName));

    auto *form = new QFormLayout;
    // SM_HUB_STATUS_HUBNAME / SM_HUB_STATUS_ONLINE / SM_HUB_TYPE ...
    form->addRow(tr("仮想 HUB 名:"), new QLabel(status.value("HubName_str").toString(), this));
    form->addRow(tr("状態:"), new QLabel(SoftEtherLabels::onlineStatus(status.value("Online_bool").toBool()), this));
    form->addRow(tr("種類:"), new QLabel(SoftEtherLabels::hubType(status.value("HubType_u32").toInt()), this));
    form->addRow(tr("セッション数:"), new QLabel(QString::number(status.value("NumSessions_u32").toInt()), this));
    form->addRow(tr("セッション数 (クライアント):"),
                 new QLabel(QString::number(status.value("NumSessionsClient_u32").toInt()), this));
    form->addRow(tr("セッション数 (ブリッジ):"),
                 new QLabel(QString::number(status.value("NumSessionsBridge_u32").toInt()), this));
    form->addRow(tr("アクセスリスト数:"), new QLabel(QString::number(status.value("NumAccessLists_u32").toInt()), this));
    form->addRow(tr("ユーザー数:"), new QLabel(QString::number(status.value("NumUsers_u32").toInt()), this));
    form->addRow(tr("グループ数:"), new QLabel(QString::number(status.value("NumGroups_u32").toInt()), this));
    form->addRow(tr("MAC テーブル数:"), new QLabel(QString::number(status.value("NumMacTables_u32").toInt()), this));
    form->addRow(tr("IP テーブル数:"), new QLabel(QString::number(status.value("NumIpTables_u32").toInt()), this));
    form->addRow(tr("SecureNAT 機能:"),
                 new QLabel(SoftEtherLabels::secureNatEnabled(status.value("SecureNATEnabled_bool").toBool()), this));
    form->addRow(tr("ログイン回数:"), new QLabel(QString::number(status.value("NumLogin_u32").toInt()), this));
    form->addRow(tr("最終ログイン日時:"),
                 new QLabel(SoftEtherLabels::dateTime(status.value("LastLoginTime_dt").toString()), this));
    form->addRow(tr("最終通信日時:"),
                 new QLabel(SoftEtherLabels::dateTime(status.value("LastCommTime_dt").toString()), this));
    form->addRow(tr("作成日時:"), new QLabel(SoftEtherLabels::dateTime(status.value("CreatedTime_dt").toString()), this));

    // D_SM_STATUS IDCANCEL
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
}
