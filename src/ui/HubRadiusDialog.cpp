#include "HubRadiusDialog.h"

#include "util/RpcUiHelpers.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

HubRadiusDialog::HubRadiusDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_RADIUS CAPTION
    setWindowTitle(tr("認証サーバーの設定"));

    auto note = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        return label;
    };

    m_useCheck = new QCheckBox(tr("RADIUS 認証を使用する(&U)"), this);
    m_hostEdit = new QLineEdit(this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(1812);
    m_secretEdit = new QLineEdit(this);
    m_secretEdit->setEchoMode(QLineEdit::Password);
    m_secretConfirmEdit = new QLineEdit(this);
    m_secretConfirmEdit->setEchoMode(QLineEdit::Password);
    m_retrySpin = new QSpinBox(this);
    m_retrySpin->setRange(500, 9999);
    m_retrySpin->setValue(500);

    auto *grid = new QGridLayout;
    grid->addWidget(new QLabel(tr("RADIUS サーバーのホスト名または\nIP アドレス(S):"), this), 0, 0, Qt::AlignRight);
    grid->addWidget(m_hostEdit, 0, 1, 1, 2);
    grid->addWidget(note(tr("(, または ; で複数指定することができます。)")), 1, 1, 1, 2);
    grid->addWidget(new QLabel(tr("ポート番号(P):"), this), 2, 0, Qt::AlignRight);
    grid->addWidget(m_portSpin, 2, 1);
    grid->addWidget(new QLabel(tr("(UDP ポート)"), this), 2, 2);
    grid->addWidget(new QLabel(tr("共有シークレット(E):"), this), 3, 0, Qt::AlignRight);
    grid->addWidget(m_secretEdit, 3, 1, 1, 2);
    grid->addWidget(new QLabel(tr("共有シークレットの確認入力(C):"), this), 4, 0, Qt::AlignRight);
    grid->addWidget(m_secretConfirmEdit, 4, 1, 1, 2);
    grid->addWidget(new QLabel(tr("再試行間隔(R):"), this), 5, 0, Qt::AlignRight);
    grid->addWidget(m_retrySpin, 5, 1);
    grid->addWidget(new QLabel(tr("ミリ秒 (500 以上 10000 未満)"), this), 5, 2);
    grid->setColumnStretch(1, 1);

    auto *group = new QGroupBox(tr("RADIUS サーバーの設定(F):"), this);
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(m_useCheck);
    groupLayout->addLayout(grid);
    groupLayout->addWidget(note(tr("RADIUS サーバーは、この VPN Server の IP アドレスからの要求を受け付けるように設定しておく必要があります。また、Password Authentication Protocol (PAP) による認証が有効になっている必要があります。")));

    auto *title = note(tr("仮想 HUB \"%1\" にユーザーが RADIUS サーバー認証モードで接続した場合に、ユーザー名とパスワードを確認する外部の RADIUS サーバーを指定することができます。").arg(m_hubName));
    auto *footer = note(tr("外部認証サーバーとして Windows NT ドメインコントローラまたは Windows Server の Active Directory コントローラを使用する場合は、VPN Server を動作させているコンピュータをそのドメインに所属させておく必要があります。NT ドメイン認証を使用する場合は、設定する項目はありません。"));

    m_okButton = new QPushButton(tr("OK"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル"), this);
    connect(m_okButton, &QPushButton::clicked, this, &HubRadiusDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_okButton);
    buttons->addWidget(cancelButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(group);
    layout->addWidget(footer);
    layout->addLayout(buttons);

    connect(m_useCheck, &QCheckBox::toggled, this, &HubRadiusDialog::updateState);
    connect(m_hostEdit, &QLineEdit::textChanged, this, &HubRadiusDialog::updateState);
    connect(m_secretEdit, &QLineEdit::textChanged, this, &HubRadiusDialog::updateState);
    connect(m_secretConfirmEdit, &QLineEdit::textChanged, this, &HubRadiusDialog::updateState);
    resize(560, 520);
    updateState();

    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("GetHubRadius"), params,
        [this](const QJsonObject &radius) {
            const QString host = radius.value("RadiusServerName_str").toString();
            m_useCheck->setChecked(!host.isEmpty());
            m_hostEdit->setText(host);
            m_portSpin->setValue(radius.value("RadiusPort_u32").toInt(1812));
            m_secretEdit->setText(radius.value("RadiusSecret_str").toString());
            m_secretConfirmEdit->setText(radius.value("RadiusSecret_str").toString());
            m_retrySpin->setValue(qBound(500, radius.value("RadiusRetryInterval_u32").toInt(500), 9999));
            updateState();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("認証サーバー設定の取得"), error); });
}

void HubRadiusDialog::updateState()
{
    const bool use = m_useCheck->isChecked();
    for (QWidget *w : QList<QWidget *>{m_hostEdit, m_portSpin, m_secretEdit, m_secretConfirmEdit, m_retrySpin}) {
        w->setEnabled(use);
    }
    m_okButton->setEnabled(!use || (!m_hostEdit->text().trimmed().isEmpty() && m_secretEdit->text() == m_secretConfirmEdit->text()));
}

void HubRadiusDialog::onOk()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    const bool use = m_useCheck->isChecked();
    params["RadiusServerName_str"] = use ? m_hostEdit->text().trimmed() : QString();
    params["RadiusPort_u32"] = m_portSpin->value();
    params["RadiusSecret_str"] = use ? m_secretEdit->text() : QString();
    params["RadiusRetryInterval_u32"] = m_retrySpin->value();
    m_rpc->call(
        QStringLiteral("SetHubRadius"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("認証サーバー設定の保存"), error); });
}
