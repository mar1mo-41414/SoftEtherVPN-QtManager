#include "IPsecSettingsDialog.h"
#include "EtherIpIdListDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

IPsecSettingsDialog::IPsecSettingsDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_IPSEC CAPTION
    setWindowTitle(tr("IPsec / L2TP / EtherIP / L2TPv3 設定"));

    auto *introLabel = new QLabel(
        tr("この VPN Server 上の仮想 HUB は、L2TP に対応した PC や Mac OS X、スマートフォン等からリモートアクセス VPN 接続を"
           "受け付けたり、EtherIP / L2TPv3 に対応した市販のルータ等から拠点間 VPN 接続を受け付けたりできます。"),
        this);
    introLabel->setWordWrap(true);

    // R_L2TP_OVER_IPSEC / R_L2TP_RAW / R_ETHERIP
    m_l2tpIpsecCheck = new QCheckBox(tr("L2TP サーバー機能を有効にする (L2TP over IPsec)(&S)"), this);
    m_l2tpRawCheck = new QCheckBox(tr("L2TP サーバー機能を有効にする (暗号化されていない L2TP)(&L)"), this);
    m_etherIpCheck = new QCheckBox(tr("EtherIP / L2TPv3 over IPsec サーバー機能有効(&E)"), this);
    auto *detailButton = new QPushButton(tr("サーバー機能の詳細設定(&D)"), this);
    connect(detailButton, &QPushButton::clicked, this, &IPsecSettingsDialog::onDetail);

    auto *l2tpGroup = new QGroupBox(tr("L2TP サーバー機能 (リモートアクセス VPN サーバー接続)"), this);
    auto *l2tpLayout = new QVBoxLayout(l2tpGroup);
    l2tpLayout->addWidget(m_l2tpIpsecCheck);
    l2tpLayout->addWidget(m_l2tpRawCheck);

    auto *etherIpGroup = new QGroupBox(tr("EtherIP / L2TPv3 サーバー機能 (拠点間接続 VPN サーバー機能)"), this);
    auto *etherIpLayout = new QVBoxLayout(etherIpGroup);
    etherIpLayout->addWidget(m_etherIpCheck);
    etherIpLayout->addWidget(detailButton);

    // S_1 / S_2
    m_defaultHubCombo = new QComboBox(this);
    m_defaultHubCombo->setEditable(true);
    auto *hubLabel = new QLabel(
        tr("L2TP、OpenVPN および MS-SSTP VPN 接続時のユーザー名は \"仮想HUB名\\ユーザー名\" または \"ユーザー名@仮想HUB名\" "
           "のように指定してください。仮想 HUB 名の指定が省略された場合に接続する仮想 HUB を選択します。"),
        this);
    hubLabel->setWordWrap(true);
    auto *hubForm = new QFormLayout;
    hubForm->addRow(tr("接続時のユーザー名で仮想 HUB 名が省略された場合に接続する仮想 HUB の選択(&H):"), m_defaultHubCombo);

    // S07 / S_PSK / S_PSK2
    m_pskEdit = new QLineEdit(this);
    auto *pskForm = new QFormLayout;
    pskForm->addRow(tr("IPsec 事前共有鍵(&P):"), m_pskEdit);
    auto *pskHint = new QLabel(
        tr("IPsec 事前共有鍵は、「PSK (Pre-Shared Key)」または「シークレット」と呼ばれることがあります。"
           "8 文字程度で設定し、VPN を利用するすべてのユーザーに配布してください。"),
        this);
    pskHint->setWordWrap(true);
    auto *pskGroup = new QGroupBox(tr("IPsec 共通設定(&C)"), this);
    auto *pskLayout = new QVBoxLayout(pskGroup);
    pskLayout->addLayout(pskForm);
    pskLayout->addWidget(pskHint);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IPsecSettingsDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(l2tpGroup);
    layout->addWidget(etherIpGroup);
    layout->addWidget(hubLabel);
    layout->addLayout(hubForm);
    layout->addWidget(pskGroup);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 560);

    m_rpc->call(
        QStringLiteral("GetIPsecServices"), {},
        [this](const QJsonObject &result) {
            m_l2tpIpsecCheck->setChecked(result.value("L2TP_IPsec_bool").toBool());
            m_l2tpRawCheck->setChecked(result.value("L2TP_Raw_bool").toBool());
            m_etherIpCheck->setChecked(result.value("EtherIP_IPsec_bool").toBool());
            m_pskEdit->setText(result.value("IPsec_Secret_str").toString());
            RpcUi::populateHubCombo(m_rpc, m_defaultHubCombo, result.value("L2TP_DefaultHub_str").toString());
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("IPsec 設定の取得"), error); });
}

void IPsecSettingsDialog::onDetail()
{
    EtherIpIdListDialog dialog(m_rpc, this);
    dialog.exec();
}

void IPsecSettingsDialog::onOk()
{
    // SM_IPSEC_PSK_TOO_LONG
    if (m_pskEdit->text().size() >= 10) {
        QMessageBox warnBox(QMessageBox::Warning, tr("警告"),
                             tr("事前共有鍵 (PSK) の長さが 10 文字以上に設定されています。\n\n"
                                "Android 携帯電話の一部のバージョンにはバグがあり、事前共有鍵が 10 文字以上の場合は VPN 接続が"
                                "行えない場合があります。そのため、事前共有鍵の長さは 9 文字以下とすることを推奨します。\n\n"
                                "事前共有鍵の設定を見直しますか?"),
                             QMessageBox::NoButton, this);
        QPushButton *reviewButton = warnBox.addButton(tr("はい"), QMessageBox::YesRole);
        warnBox.addButton(tr("いいえ"), QMessageBox::NoRole);
        warnBox.exec();
        if (warnBox.clickedButton() == reviewButton) {
            return;
        }
    }

    QJsonObject params;
    params["L2TP_Raw_bool"] = m_l2tpRawCheck->isChecked();
    params["L2TP_IPsec_bool"] = m_l2tpIpsecCheck->isChecked();
    params["EtherIP_IPsec_bool"] = m_etherIpCheck->isChecked();
    params["IPsec_Secret_str"] = m_pskEdit->text();
    params["L2TP_DefaultHub_str"] = m_defaultHubCombo->currentText().trimmed();

    m_rpc->call(
        QStringLiteral("SetIPsecServices"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("IPsec 設定の変更"), error); });
}
