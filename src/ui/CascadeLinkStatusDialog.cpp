#include "CascadeLinkStatusDialog.h"

#include "util/SoftEtherLabels.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CascadeLinkStatusDialog::CascadeLinkStatusDialog(const QString &accountName, const QJsonObject &status, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("カスケード接続 \"%1\" の状態").arg(accountName));

    auto *form = new QFormLayout;
    form->addRow(tr("接続中:"), new QLabel(status.value("Connected_bool").toBool() ? tr("はい") : tr("いいえ"), this));
    form->addRow(tr("接続先サーバー名:"), new QLabel(status.value("ServerName_str").toString(), this));
    form->addRow(tr("接続先ポート番号:"), new QLabel(QString::number(status.value("ServerPort_u32").toInt()), this));
    form->addRow(tr("サーバー製品名:"), new QLabel(status.value("ServerProductName_str").toString(), this));
    form->addRow(tr("接続開始時刻:"), new QLabel(SoftEtherLabels::dateTime(status.value("StartTime_dt").toString()), this));
    form->addRow(tr("現在の接続確立時刻:"),
                 new QLabel(SoftEtherLabels::dateTime(status.value("CurrentConnectionEstablishTime_dt").toString()), this));
    form->addRow(tr("TCP コネクション数:"), new QLabel(QString::number(status.value("NumTcpConnections_u32").toInt()), this));
    form->addRow(tr("暗号化アルゴリズム:"), new QLabel(status.value("CipherName_str").toString(), this));

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("閉じる(&X)"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
}
