#include "OpenVpnSstpDialog.h"
#include "IPsecSettingsDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

OpenVpnSstpDialog::OpenVpnSstpDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_OPENVPN CAPTION
    setWindowTitle(tr("OpenVPN / MS-SSTP 設定"));

    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    // S_TITLE
    auto *titleLabel = new QLabel(tr("OpenVPN / MS-SSTP VPN サーバー機能設定"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleLabel->setFont(titleFont);

    // S_13 / S_1 / R_OPENVPN
    m_openVpnCheck = new QCheckBox(tr("OpenVPN サーバー機能を有効にする(&O)"), this);
    QFont boldFont = m_openVpnCheck->font();
    boldFont.setBold(true);
    m_openVpnCheck->setFont(boldFont);
    m_portsEdit = new QLineEdit(this);
    m_portsEdit->setPlaceholderText(tr("例: 1194"));
    auto *resetButton = new QPushButton(tr("標準に戻す(&D)"), this);
    connect(resetButton, &QPushButton::clicked, this, [this]() { m_portsEdit->setText(QStringLiteral("1194")); });
    auto *portRow = new QHBoxLayout;
    portRow->addWidget(m_portsEdit, 1);
    portRow->addWidget(resetButton);
    auto *portCaption = new QLabel(tr("OpenVPN サービスを提供する UDP ポート:"), this);

    // S_TOOL / S_TOOL2 / B_CONFIG
    m_configButton = new QPushButton(tr("OpenVPN クライアント用のサンプル設定ファイルを生成(&C)"), this);
    connect(m_configButton, &QPushButton::clicked, this, &OpenVpnSstpDialog::onGenerateConfig);
    auto *toolCaption = new QLabel(tr("OpenVPN クライアント用サンプル設定ファイル自動作成ツール"), this);
    toolCaption->setFont(boldFont);
    auto *toolLabel = note(
        tr("本来、OpenVPN クライアントを使うためには設定ファイルを手動で記述する必要があり、これは難易度が高い作業です。"
           "しかし、以下のボタンをクリックするだけでこの VPN Server に接続することができる基本的な OpenVPN クライアント用の"
           "設定ファイルを自動的に生成することができます。"));

    auto *openVpnGroup = new QGroupBox(tr("OpenVPN 互換サーバー機能"), this);
    auto *openVpnLayout = new QVBoxLayout(openVpnGroup);
    openVpnLayout->addWidget(note(tr("OpenVPN 社の OpenVPN ソフトウェア製品と同等の VPN サーバー機能を搭載しています。\n\nOpenVPN クライアントからこの VPN Server に接続できます。")));
    openVpnLayout->addWidget(m_openVpnCheck);
    openVpnLayout->addWidget(portCaption);
    openVpnLayout->addLayout(portRow);
    openVpnLayout->addWidget(note(tr("UDP ポートは複数指定できます。複数指定する場合はスペースまたはカンマで区切ってください。OpenVPN サーバー機能は TCP ポートでも有効になります。この場合、この VPN Server に現在作成されているすべての TCP リスナポートで OpenVPN プロトコルがサポートされます。")));
    openVpnLayout->addWidget(toolCaption);
    openVpnLayout->addWidget(toolLabel);
    openVpnLayout->addWidget(m_configButton);
    connect(m_openVpnCheck, &QCheckBox::toggled, this, [=](bool on) {
        for (QWidget *w : QList<QWidget *>{m_portsEdit, resetButton, portCaption, m_configButton, toolCaption, toolLabel}) {
            w->setEnabled(on);
        }
    });
    emit m_openVpnCheck->toggled(false);

    // S_2 / S_3 / R_SSTP / S_SSTP
    m_sstpCheck = new QCheckBox(tr("MS-SSTP VPN サーバー機能を有効にする(&M)"), this);
    m_sstpCheck->setFont(boldFont);
    auto *sstpGroup = new QGroupBox(tr("Microsoft SSTP VPN 互換サーバー機能"), this);
    auto *sstpLayout = new QVBoxLayout(sstpGroup);
    sstpLayout->addWidget(note(tr("Microsoft 社の Windows Server 2008 / 2012 製品に搭載されている MS-SSTP VPN サーバー機能と互換性がある機能を搭載しています。\n\nWindows Vista / 7 / 8 / RT / 10 に標準搭載の MS-SSTP クライアントからこの VPN Server に接続できます。")));
    sstpLayout->addWidget(m_sstpCheck);
    sstpLayout->addWidget(note(tr("VPN Server の SSL 証明書の CN の値がクライアント側で指定するホスト名と一致し、かつその証明書が信頼されている必要があります。詳しくは Microsoft 社のドキュメントを参照してください。")));

    // S_4 / B_IPSEC
    auto *ipsecButton = new QPushButton(tr("IPsec サーバー機能の設定(&P)"), this);
    connect(ipsecButton, &QPushButton::clicked, this, &OpenVpnSstpDialog::onIPsec);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &OpenVpnSstpDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *footer = new QHBoxLayout;
    footer->addWidget(note(tr("これらの互換サーバー機能で仮想 HUB に接続する場合のユーザー名の指定方法、およびデフォルト仮想 HUB の選択規則は、IPsec サーバー機能と同様です。")), 1);
    footer->addWidget(ipsecButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(openVpnGroup);
    layout->addWidget(sstpGroup);
    layout->addLayout(footer);
    layout->addWidget(buttonBox);

    resize(700, 760);

    m_rpc->call(
        QStringLiteral("GetOpenVpnSstpConfig"), {},
        [this](const QJsonObject &result) {
            m_openVpnCheck->setChecked(result.value("EnableOpenVPN_bool").toBool());
            m_portsEdit->setText(result.value("OpenVPNPortList_str").toString());
            m_sstpCheck->setChecked(result.value("EnableSSTP_bool").toBool());
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("OpenVPN / MS-SSTP 設定の取得"), error); });
}

void OpenVpnSstpDialog::onGenerateConfig()
{
    m_rpc->call(
        QStringLiteral("MakeOpenVpnConfigFile"), {},
        [this](const QJsonObject &result) {
            const QByteArray zip = QByteArray::fromBase64(result.value("Buffer_bin").toString().toUtf8());
            const QString path = QFileDialog::getSaveFileName(this, tr("OpenVPN 設定ファイルの保存先"),
                                                                QStringLiteral("openvpn_config.zip"),
                                                                tr("ZIP ファイル (*.zip)"));
            if (path.isEmpty()) {
                return;
            }
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write(zip) != zip.size()) {
                // SM_OPENVPN_CONFIG_SAVE_NG
                QMessageBox::warning(this, tr("エラー"), tr("ZIP ファイル '%1' の保存に失敗しました。").arg(path));
                return;
            }
            // SM_OPENVPN_CONFIG_SAVE_OK
            QMessageBox::information(
                this, tr("完了"),
                tr("OpenVPN 設定ファイルを格納した ZIP ファイルを '%1' に保存しました。\n\n"
                   "この ZIP ファイルを開くと、OpenVPN クライアントで使用できる設定ファイルのサンプルが展開できます。"
                   "なお、設定ファイルは実際に使用する前には環境に応じて修正する必要がある場合があります。\n\n"
                   "詳しくは ZIP ファイル内の 'readme.txt' ファイルをお読みください。")
                    .arg(path));
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("OpenVPN 設定ファイルの生成"), error); });
}

void OpenVpnSstpDialog::onIPsec()
{
    IPsecSettingsDialog dialog(m_rpc, this);
    dialog.exec();
}

void OpenVpnSstpDialog::onOk()
{
    QJsonObject params;
    params["EnableOpenVPN_bool"] = m_openVpnCheck->isChecked();
    params["OpenVPNPortList_str"] = m_portsEdit->text().trimmed();
    params["EnableSSTP_bool"] = m_sstpCheck->isChecked();
    m_rpc->call(
        QStringLiteral("SetOpenVpnSstpConfig"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("OpenVPN / MS-SSTP 設定の変更"), error); });
}
