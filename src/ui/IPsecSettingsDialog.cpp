#include "IPsecSettingsDialog.h"
#include "EtherIpIdListDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
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

    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    // S_TITLE / S_3
    auto *titleLabel = new QLabel(tr("IPsec / L2TP / EtherIP / L2TPv3 サーバー機能の設定"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);

    // R_L2TP_OVER_IPSEC / R_L2TP_RAW / R_ETHERIP
    m_l2tpIpsecCheck = new QCheckBox(tr("L2TP サーバー機能を有効にする (L2TP over IPsec)"), this);
    m_l2tpRawCheck = new QCheckBox(tr("L2TP サーバー機能を有効にする (暗号化されていない L2TP)"), this);
    m_etherIpCheck = new QCheckBox(tr("EtherIP / L2TPv3 over IPsec サーバー機能有効"), this);
    auto *detailButton = new QPushButton(tr("サーバー機能の詳細設定(&D)"), this);
    connect(detailButton, &QPushButton::clicked, this, &IPsecSettingsDialog::onDetail);
    QFont boldFont = m_l2tpIpsecCheck->font();
    boldFont.setBold(true);
    m_l2tpIpsecCheck->setFont(boldFont);
    m_l2tpRawCheck->setFont(boldFont);
    m_etherIpCheck->setFont(boldFont);

    // S_1 / S_2
    m_defaultHubCombo = new QComboBox(this);
    m_defaultHubCombo->setEditable(true);
    auto *hubRow = new QHBoxLayout;
    hubRow->addWidget(new QLabel(tr("接続時のユーザー名で仮想 HUB 名が省略された場合に接続する仮想 HUB の選択(H):"), this));
    hubRow->addWidget(m_defaultHubCombo, 1);

    auto *l2tpGroup = new QGroupBox(tr("L2TP サーバー機能 (リモートアクセス VPN サーバー接続)"), this);
    auto *l2tpLayout = new QVBoxLayout(l2tpGroup);
    l2tpLayout->addWidget(note(tr("iPhone、iPad、Android 等のスマートフォンや Mac OS X、Windows 等の OS 付属の標準 VPN クライアントから VPN 接続ができるようになります。")));
    l2tpLayout->addWidget(m_l2tpIpsecCheck);
    l2tpLayout->addWidget(note(tr("iPhone、iPad、Android、Windows、Mac OS X からの VPN 接続を受け付けることができます。")));
    l2tpLayout->addWidget(m_l2tpRawCheck);
    l2tpLayout->addWidget(note(tr("IPsec を用いない L2TP を使用する特殊なクライアントをサポートできます。")));
    l2tpLayout->addWidget(note(tr("L2TP、OpenVPN および MS-SSTP VPN 接続時のユーザー名は \"仮想HUB名\\ユーザー名\" または \"ユーザー名@仮想HUB名\" のように指定してください。なお、仮想 HUB 名の指定が省略された場合、デフォルトで接続する仮想 HUB を設定しておくことができます。")));
    l2tpLayout->addLayout(hubRow);

    detailButton->setEnabled(false);
    connect(m_etherIpCheck, &QCheckBox::toggled, detailButton, &QPushButton::setEnabled);
    auto *etherIpGroup = new QGroupBox(tr("EtherIP / L2TPv3 サーバー機能 (拠点間接続 VPN サーバー機能)"), this);
    auto *etherIpLayout = new QVBoxLayout(etherIpGroup);
    etherIpLayout->addWidget(note(tr("EtherIP / L2TPv3 over IPsec に対応した市販のルータ製品は、この VPN Server の仮想 HUB にレイヤ 2 (Ethernet) でブリッジ接続できます。")));
    auto *etherIpRow = new QHBoxLayout;
    etherIpRow->addWidget(m_etherIpCheck, 1);
    etherIpRow->addWidget(detailButton);
    etherIpLayout->addLayout(etherIpRow);

    // S07 / S_PSK / S_PSK2
    m_pskEdit = new QLineEdit(this);
    auto *pskRow = new QHBoxLayout;
    pskRow->addWidget(new QLabel(tr("IPsec 事前共有鍵(P):"), this));
    pskRow->addWidget(m_pskEdit, 1);
    auto *pskGroup = new QGroupBox(tr("IPsec 共通設定(C)"), this);
    auto *pskLayout = new QVBoxLayout(pskGroup);
    pskLayout->addLayout(pskRow);
    pskLayout->addWidget(note(tr("IPsec 事前共有鍵は、「PSK (Pre-Shared Key)」または「シークレット」と呼ばれることがあります。8 文字程度で設定し、VPN を利用するすべてのユーザーに配布してください。")));

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IPsecSettingsDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(note(tr("この VPN Server 上の仮想 HUB は、L2TP に対応した PC や Mac OS X、スマートフォン等からリモートアクセス VPN 接続を受け付けたり、EtherIP / L2TPv3 に対応した市販のルータ等から拠点間 VPN 接続を受け付けたりできます。")));
    layout->addWidget(l2tpGroup);
    layout->addWidget(etherIpGroup);
    layout->addWidget(pskGroup);
    layout->addWidget(buttonBox);

    resize(760, 640);

    m_rpc->call(
        QStringLiteral("GetIPsecServices"), {},
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            m_l2tpIpsecCheck->setChecked(result.value("L2TP_IPsec_bool").toBool());
            m_l2tpRawCheck->setChecked(result.value("L2TP_Raw_bool").toBool());
            m_etherIpCheck->setChecked(result.value("EtherIP_IPsec_bool").toBool());
            m_pskEdit->setText(result.value("IPsec_Secret_str").toString());
            RpcUi::populateHubCombo(m_rpc, m_defaultHubCombo, result.value("L2TP_DefaultHub_str").toString());
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("IPsec 設定の取得"), error); }));
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
        QStringLiteral("SetIPsecServices"), params, RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("IPsec 設定の変更"), error); }));
}
