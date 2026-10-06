#include "DdnsProxyDialog.h"

#include "util/RpcUiHelpers.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

DdnsProxyDialog::DdnsProxyDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_PROXY CAPTION
    setWindowTitle(tr("プロキシサーバー経由の接続"));

    auto *introLabel = new QLabel(tr("プロキシサーバーを経由してサーバーに接続することができます。"), this);

    // R_DIRECT_TCP / R_HTTPS / R_SOCKS
    m_directRadio = new QRadioButton(tr("直接 TCP/IP 接続 (プロキシを使わない)(&D)"), this);
    m_httpRadio = new QRadioButton(tr("HTTP プロキシサーバー経由接続(&T)"), this);
    m_socksRadio = new QRadioButton(tr("SOCKS プロキシサーバー経由接続(&K)"), this);
    m_directRadio->setChecked(true);

    m_hostEdit = new QLineEdit(this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(8080);
    m_userEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    auto *proxyForm = new QFormLayout;
    proxyForm->addRow(tr("ホスト名(&H):"), m_hostEdit);
    proxyForm->addRow(tr("ポート番号(&P):"), m_portSpin);
    proxyForm->addRow(tr("ユーザー名(&U):"), m_userEdit);
    proxyForm->addRow(tr("パスワード(&W):"), m_passwordEdit);

    auto *typeGroup = new QGroupBox(tr("プロキシの種類(&Y)"), this);
    auto *typeLayout = new QVBoxLayout(typeGroup);
    typeLayout->addWidget(m_directRadio);
    typeLayout->addWidget(m_httpRadio);
    typeLayout->addWidget(m_socksRadio);
    typeLayout->addLayout(proxyForm);

    connect(m_directRadio, &QRadioButton::toggled, this, &DdnsProxyDialog::onTypeChanged);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DdnsProxyDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(typeGroup);
    layout->addWidget(buttonBox);

    resize(440, sizeHint().height());
    onTypeChanged();

    m_rpc->call(
        QStringLiteral("GetDDnsInternetSetting"), {},
        [this](const QJsonObject &result) {
            const int type = result.value("ProxyType_u32").toInt();
            m_httpRadio->setChecked(type == 1);
            m_socksRadio->setChecked(type == 2);
            m_directRadio->setChecked(type != 1 && type != 2);
            m_hostEdit->setText(result.value("ProxyHostName_str").toString());
            const int port = result.value("ProxyPort_u32").toInt();
            m_portSpin->setValue(port > 0 ? port : 8080);
            m_userEdit->setText(result.value("ProxyUsername_str").toString());
            m_passwordEdit->setText(result.value("ProxyPassword_str").toString());
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("プロキシ設定の取得"), error); });
}

void DdnsProxyDialog::onTypeChanged()
{
    const bool useProxy = !m_directRadio->isChecked();
    m_hostEdit->setEnabled(useProxy);
    m_portSpin->setEnabled(useProxy);
    m_userEdit->setEnabled(useProxy);
    m_passwordEdit->setEnabled(useProxy);
}

void DdnsProxyDialog::onOk()
{
    QJsonObject params;
    params["ProxyType_u32"] = m_httpRadio->isChecked() ? 1 : (m_socksRadio->isChecked() ? 2 : 0);
    params["ProxyHostName_str"] = m_hostEdit->text().trimmed();
    params["ProxyPort_u32"] = m_portSpin->value();
    params["ProxyUsername_str"] = m_userEdit->text().trimmed();
    params["ProxyPassword_str"] = m_passwordEdit->text();
    m_rpc->call(
        QStringLiteral("SetDDnsInternetSetting"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("プロキシ設定の変更"), error); });
}
