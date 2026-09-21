#include "ConnectDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("VPN Server への接続"));

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText(tr("例: vpn.example.com"));

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(443);

    m_hubModeCheck = new QCheckBox(tr("仮想 HUB 管理モードで接続する"), this);

    m_hubNameEdit = new QLineEdit(this);
    m_hubNameEdit->setEnabled(false);
    connect(m_hubModeCheck, &QCheckBox::toggled, m_hubNameEdit, &QLineEdit::setEnabled);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    auto *form = new QFormLayout;
    form->addRow(tr("ホスト名:"), m_hostEdit);
    form->addRow(tr("ポート番号:"), m_portSpin);
    form->addRow(QString(), m_hubModeCheck);
    form->addRow(tr("仮想 HUB 名:"), m_hubNameEdit);
    form->addRow(tr("管理接続用パスワード:"), m_passwordEdit);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);

    m_connectButton = new QPushButton(tr("接続"), this);
    m_connectButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("キャンセル"), this);

    auto *buttonBox = new QHBoxLayout;
    buttonBox->addStretch();
    buttonBox->addWidget(m_cancelButton);
    buttonBox->addWidget(m_connectButton);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_statusLabel);
    layout->addLayout(buttonBox);

    connect(m_connectButton, &QPushButton::clicked, this, &ConnectDialog::onConnectClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    resize(420, sizeHint().height());
}

void ConnectDialog::setBusy(bool busy)
{
    m_connectButton->setEnabled(!busy);
    m_hostEdit->setEnabled(!busy);
    m_portSpin->setEnabled(!busy);
    m_hubModeCheck->setEnabled(!busy);
    m_hubNameEdit->setEnabled(!busy && m_hubModeCheck->isChecked());
    m_passwordEdit->setEnabled(!busy);
}

void ConnectDialog::onConnectClicked()
{
    const QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ホスト名を入力してください。"));
        return;
    }

    const quint16 port = static_cast<quint16>(m_portSpin->value());
    const QString hubName = m_hubModeCheck->isChecked() ? m_hubNameEdit->text().trimmed() : QString();
    const QString password = m_passwordEdit->text();

    setBusy(true);
    m_statusLabel->setText(tr("接続を試みています…"));

    auto *rpc = new VpnServerRpc(this);
    rpc->connectToServer(host, port, hubName, password);

    // まず Test で疎通を確認し、成功したら GetServerInfo でサーバー情報を取得する。
    rpc->test(
        [this, rpc](const QJsonObject &) {
            rpc->getServerInfo(
                [this, rpc](const QJsonObject &info) {
                    m_rpc = rpc;
                    m_serverInfo = info;
                    accept();
                },
                [this, rpc](const RpcError &error) {
                    rpc->deleteLater();
                    setBusy(false);
                    m_statusLabel->setText(tr("サーバー情報の取得に失敗しました: %1 (code %2)")
                                                .arg(error.message)
                                                .arg(error.code));
                });
        },
        [this, rpc](const RpcError &error) {
            rpc->deleteLater();
            setBusy(false);
            m_statusLabel->setText(tr("接続に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

VpnServerRpc *ConnectDialog::takeConnectedRpc()
{
    if (m_rpc) {
        m_rpc->setParent(nullptr);
    }
    VpnServerRpc *rpc = m_rpc;
    m_rpc = nullptr;
    return rpc;
}
