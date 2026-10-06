#include "L3IfAddDialog.h"

#include "util/RpcUiHelpers.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

L3IfAddDialog::L3IfAddDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
{
    // D_SM_L3_SW_IF CAPTION
    setWindowTitle(tr("仮想インターフェイスの追加"));

    auto *introLabel = new QLabel(
        tr("新しい仮想インターフェイスを仮想レイヤ 3 スイッチに追加します。\n\n"
           "仮想インターフェイスが所属する IP ネットワーク空間とインターフェイス自身の IP アドレスを定義する必要があります。"
           "また、インターフェイスが接続する先の仮想 HUB 名を選択するか入力してください。"
           "仮想 HUB 名は現在存在していない仮想 HUB を指定することもできます。"),
        this);
    introLabel->setWordWrap(true);

    m_hubCombo = new QComboBox(this);
    m_hubCombo->setEditable(true);
    RpcUi::populateHubCombo(rpc, m_hubCombo);
    auto *hubForm = new QFormLayout;
    hubForm->addRow(tr("仮想 HUB(&H):"), m_hubCombo);
    auto *hubGroup = new QGroupBox(tr("接続先仮想 HUB (&A)"), this);
    hubGroup->setLayout(hubForm);

    m_ipEdit = new QLineEdit(this);
    m_maskEdit = new QLineEdit(this);
    m_maskEdit->setText(QStringLiteral("255.255.255.0"));
    auto *ipForm = new QFormLayout;
    ipForm->addRow(tr("IP アドレス(&I):"), m_ipEdit);
    ipForm->addRow(tr("サブネットマスク(&S):"), m_maskEdit);
    auto *ipGroup = new QGroupBox(tr("仮想インターフェイスの持つ IP アドレスと所属するサブネット空間(&D)"), this);
    ipGroup->setLayout(ipForm);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &L3IfAddDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(hubGroup);
    layout->addWidget(ipGroup);
    layout->addWidget(buttonBox);

    resize(520, sizeHint().height());
}

QJsonObject L3IfAddDialog::toRpcParams() const
{
    QJsonObject params;
    params["HubName_str"] = m_hubCombo->currentText().trimmed();
    params["IpAddress_ip"] = m_ipEdit->text().trimmed();
    params["SubnetMask_ip"] = m_maskEdit->text().trimmed();
    return params;
}

void L3IfAddDialog::accept()
{
    if (m_hubCombo->currentText().trimmed().isEmpty() || m_ipEdit->text().trimmed().isEmpty() ||
        m_maskEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("仮想 HUB 名・IP アドレス・サブネットマスクを入力してください。"));
        return;
    }
    QDialog::accept();
}
