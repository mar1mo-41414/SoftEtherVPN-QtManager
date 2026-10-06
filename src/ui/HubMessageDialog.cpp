#include "HubMessageDialog.h"

#include "util/RpcUiHelpers.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

HubMessageDialog::HubMessageDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(std::move(hubName))
{
    // D_SM_MSG CAPTION
    setWindowTitle(tr("メッセージの設定"));

    auto *title = new QLabel(
        tr("仮想 HUB \"%1\" に VPN Client が接続した際に、ユーザーの画面にメッセージを表示できます。メッセージを表示する場合は、以下に表示したいメッセージの内容を入力してください。")
            .arg(m_hubName),
        this);
    title->setWordWrap(true);

    m_useCheck = new QCheckBox(tr("メッセージを表示する(&M)"), this);
    m_messageEdit = new QPlainTextEdit(this);
    m_messageEdit->setEnabled(false);
    connect(m_useCheck, &QCheckBox::toggled, m_messageEdit, &QPlainTextEdit::setEnabled);

    auto *infoGroup = new QGroupBox(tr("メッセージの表示機能について"), this);
    auto *infoLayout = new QVBoxLayout(infoGroup);
    auto *info = new QLabel(
        tr("接続元のユーザーが使用している VPN Client のバージョンが 3.0 以降である必要があります。\n\nメッセージに「http://」で始まる URL を 1 行だけ記載すると、メッセージを表示する代わりにその URL をデフォルトの Web ブラウザを起動して表示することができます。"),
        this);
    info->setWordWrap(true);
    infoLayout->addWidget(info);

    auto *okButton = new QPushButton(tr("OK"), this);
    auto *cancelButton = new QPushButton(tr("キャンセル"), this);
    connect(okButton, &QPushButton::clicked, this, &HubMessageDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okButton);
    buttons->addWidget(cancelButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(m_useCheck);
    layout->addWidget(m_messageEdit, 1);
    layout->addWidget(infoGroup);
    layout->addLayout(buttons);
    resize(500, 520);

    QJsonObject params;
    params["HubName_str"] = m_hubName;
    m_rpc->call(
        QStringLiteral("GetHubMsg"), params,
        [this](const QJsonObject &result) {
            const QString message = QString::fromUtf8(QByteArray::fromBase64(result.value("Msg_bin").toString().toLatin1()));
            m_useCheck->setChecked(!message.isEmpty());
            m_messageEdit->setPlainText(message);
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("メッセージの取得"), error); });
}

void HubMessageDialog::onOk()
{
    QJsonObject params;
    params["HubName_str"] = m_hubName;
    const QString message = m_useCheck->isChecked() ? m_messageEdit->toPlainText() : QString();
    params["Msg_bin"] = QString::fromLatin1(message.toUtf8().toBase64());
    m_rpc->call(
        QStringLiteral("SetHubMsg"), params, [this](const QJsonObject &) { accept(); },
        [this](const RpcError &error) { RpcUi::showError(this, tr("メッセージの保存"), error); });
}
