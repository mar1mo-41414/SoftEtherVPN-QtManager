#include "DdnsDialog.h"
#include "DdnsProxyDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStringView>
#include <QVBoxLayout>

namespace {

// 設定ファイル(.config)のテキストから "declare DDnsClient { byte Key ... }" のKey値を取り出す。
QString extractDdnsKey(const QString &config)
{
    const int declareIndex = config.indexOf(QRegularExpression(QStringLiteral("declare\\s+DDnsClient\\b")));
    if (declareIndex < 0) {
        return QString();
    }
    const int open = config.indexOf(QLatin1Char('{'), declareIndex);
    if (open < 0) {
        return QString();
    }
    int depth = 0;
    int close = -1;
    for (int i = open; i < config.size(); ++i) {
        if (config.at(i) == QLatin1Char('{')) {
            ++depth;
        } else if (config.at(i) == QLatin1Char('}') && --depth == 0) {
            close = i;
            break;
        }
    }
    if (close < 0) {
        return QString();
    }
    const QString block = config.mid(open, close - open);
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("^\\s*byte\\s+Key\\s+(\\S+)"), QRegularExpression::MultilineOption).match(block);
    return match.hasMatch() ? match.captured(1) : QString();
}

} // namespace

DdnsDialog::DdnsDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_DDNS CAPTION
    setWindowTitle(tr("ダイナミック DNS 機能"));

    auto *introLabel = new QLabel(
        tr("ダイナミック DNS により、この VPN Server コンピュータに永続的な固有の DNS ホスト名が割当てられます。"
           "これにより独自でドメインを所有していなくても、VPN Client や VPN Bridge などの設定画面上で VPN Server の "
           "IP アドレスの代わりに DNS ホスト名によって VPN Server を指定することができます。"),
        this);
    introLabel->setWordWrap(true);

    // S_4 / S_STATUS3〜5
    m_fqdnLabel = new QLabel(this);
    m_ipv4Label = new QLabel(this);
    m_ipv6Label = new QLabel(this);
    auto *hintButton = new QPushButton(tr("ヒント"), this);
    connect(hintButton, &QPushButton::clicked, this, &DdnsDialog::onHint);

    auto *statusForm = new QFormLayout;
    statusForm->addRow(tr("割当てられているダイナミック DNS ホスト名(&H):"), m_fqdnLabel);
    statusForm->addRow(tr("グローバル IPv&4 アドレス:"), m_ipv4Label);
    statusForm->addRow(tr("グローバル IPv&6 アドレス:"), m_ipv6Label);
    // S_STATUS8 / B_HINT2
    m_keyLabel = new QLabel(this);
    m_keyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *keyHintButton = new QPushButton(tr("ヒント"), this);
    connect(keyHintButton, &QPushButton::clicked, this, &DdnsDialog::onKeyHint);
    auto *keyLayout = new QHBoxLayout;
    keyLayout->addWidget(m_keyLabel, 1);
    keyLayout->addWidget(keyHintButton);
    statusForm->addRow(tr("DNS 鍵:"), keyLayout);
    auto *statusGroup = new QGroupBox(tr("現在の状態(&S)"), this);
    auto *statusLayout = new QVBoxLayout(statusGroup);
    statusLayout->addLayout(statusForm);
    statusLayout->addWidget(hintButton, 0, Qt::AlignRight);

    // S_5 / S_STATUS6 / S_STATUS7
    m_hostNameEdit = new QLineEdit(this);
    m_hostNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[A-Za-z0-9-]{0,31}")), this));
    m_suffixLabel = new QLabel(this);
    auto *hostLayout = new QHBoxLayout;
    hostLayout->addWidget(m_hostNameEdit);
    hostLayout->addWidget(m_suffixLabel);
    auto *changeButton = new QPushButton(tr("上記の DNS ホスト名に変更する(&A)"), this);
    auto *restoreButton = new QPushButton(tr("変更前に戻す(&R)"), this);
    connect(changeButton, &QPushButton::clicked, this, &DdnsDialog::onChange);
    connect(restoreButton, &QPushButton::clicked, this, &DdnsDialog::onRestore);
    auto *changeButtons = new QHBoxLayout;
    changeButtons->addStretch();
    changeButtons->addWidget(restoreButton);
    changeButtons->addWidget(changeButton);

    auto *hostHint = new QLabel(tr("3 文字以上 31 文字以内の半角英数字およびハイフン '-' が使用できます。変更は何度でも可能です。"), this);
    hostHint->setWordWrap(true);
    auto *changeGroup = new QGroupBox(tr("設定の変更(&M)"), this);
    auto *changeLayout = new QVBoxLayout(changeGroup);
    changeLayout->addWidget(new QLabel(tr("ダイナミック DNS ホスト名の変更:"), this));
    changeLayout->addLayout(hostLayout);
    changeLayout->addWidget(hostHint);
    changeLayout->addLayout(changeButtons);

    // S_2
    auto *noticeLabel = new QLabel(
        tr("IPv6 インターネットに接続されていない場合は上記の [IPv6 アドレス] の欄にエラーが表示されますが、異常ではありません。"
           "一部の国・地域では、行政機関による制限により、ダイナミック DNS サービスが利用できない場合があります。"),
        this);
    noticeLabel->setWordWrap(true);

    // B_PROXY / IDCANCEL
    auto *proxyButton = new QPushButton(tr("プロキシサーバー経由で接続(&P)"), this);
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(proxyButton, &QPushButton::clicked, this, &DdnsDialog::onProxy);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(proxyButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(statusGroup);
    layout->addWidget(changeGroup);
    layout->addWidget(noticeLabel);
    layout->addLayout(bottomLayout);

    DialogSizing::fitToWidth(this, 560);
    reload();
    loadKey();
}

void DdnsDialog::loadKey()
{
    m_rpc->call(
        QStringLiteral("GetConfig"), {},
        [this](const QJsonObject &result) {
            const QString config = QString::fromUtf8(QByteArray::fromBase64(result.value("FileData_bin").toString().toUtf8()));
            const QString key = extractDdnsKey(config);
            // SM_DDNS_KEY_ERR
            m_keyLabel->setText(key.isEmpty() ? tr("DNS 鍵の取得に失敗しました。") : key);
        },
        [this](const RpcError &) { m_keyLabel->setText(tr("DNS 鍵の取得に失敗しました。")); });
}

void DdnsDialog::reload()
{
    m_rpc->call(
        QStringLiteral("GetDDnsClientStatus"), {},
        [this](const QJsonObject &status) {
            m_currentHostName = status.value("CurrentHostName_str").toString();
            m_suffix = status.value("DnsSuffix_str").toString();
            if (!m_suffix.isEmpty() && !m_suffix.startsWith(QLatin1Char('.'))) {
                m_suffix.prepend(QLatin1Char('.'));
            }
            m_ipv4 = status.value("CurrentIPv4_str").toString();
            m_ipv6 = status.value("CurrentIPv6_str").toString();

            const QString fqdn = status.value("CurrentFqdn_str").toString();
            // SM_DDNS_FQDN_EMPTY / SM_DDNS_IPV4_ERROR / SM_DDNS_IPV6_ERROR
            m_fqdnLabel->setText(fqdn.isEmpty() ? tr("(なし)") : fqdn);
            m_ipv4Label->setText(status.value("Err_IPv4_u32").toInt() != 0 ? tr("IPv4 の DDNS サーバーに到達できません。")
                                                                          : m_ipv4);
            m_ipv6Label->setText(status.value("Err_IPv6_u32").toInt() != 0 ? tr("IPv6 の DDNS サーバーに到達できません。")
                                                                          : m_ipv6);
            m_hostNameEdit->setText(m_currentHostName);
            m_suffixLabel->setText(m_suffix);
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("ダイナミック DNS 状態の取得"), error); });
}

void DdnsDialog::onRestore()
{
    m_hostNameEdit->setText(m_currentHostName);
}

void DdnsDialog::onChange()
{
    const QString newName = m_hostNameEdit->text().trimmed();
    if (newName.size() < 3) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ダイナミック DNS ホスト名は 3 文字以上で指定してください。"));
        return;
    }

    QJsonObject params;
    params["StrValue_str"] = newName;
    m_rpc->call(
        QStringLiteral("ChangeDDnsClientHostname"), params,
        [this, newName](const QJsonObject &) {
            reload();
            const QString fqdn = newName + m_suffix;

            // SM_DDNS_SERVER_CERT_MSG: SSTPのためにCNを新しいホスト名に合わせた証明書を作るか尋ねる。
            QMessageBox certBox(
                QMessageBox::Question, tr("ダイナミック DNS 機能"),
                tr("DDNS ホスト名を \"%1\" に変更しました。\n\n"
                   "この VPN Server に Microsoft SSTP VPN プロトコルを用いて Windows の組み込み SSTP VPN クライアントから"
                   "接続する場合は、VPN クライアント側で接続先 VPN サーバー名として指定するホスト名の文字列と、"
                   "この VPN Server の SSL サーバー証明書の CN の文字列とが完全一致している必要があります。\n\n"
                   "この VPN Server の SSL サーバー証明書の CN が \"%1\" となるようにサーバー証明書を再生成しますか?\n"
                   "(「いいえ」を選ぶと、現在の SSL サーバー証明書が引き続き使用されます。再生成すると現在の証明書は破棄されます。)")
                    .arg(fqdn),
                QMessageBox::NoButton, this);
            QPushButton *yesButton = certBox.addButton(tr("はい"), QMessageBox::YesRole);
            certBox.addButton(tr("いいえ"), QMessageBox::NoRole);
            certBox.exec();
            if (certBox.clickedButton() != yesButton) {
                return;
            }

            QJsonObject certParams;
            certParams["StrValue_str"] = fqdn;
            m_rpc->call(
                QStringLiteral("RegenerateServerCert"), certParams, [](const QJsonObject &) {},
                [this](const RpcError &error) { RpcUi::showError(this, tr("サーバー証明書の再生成"), error); });
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("ダイナミック DNS ホスト名の変更"), error); });
}

void DdnsDialog::onProxy()
{
    DdnsProxyDialog dialog(m_rpc, this);
    dialog.exec();
}

void DdnsDialog::onKeyHint()
{
    // SM_DDNS_KEY_MSG
    QMessageBox::information(
        this, tr("ダイナミック DNS 秘密鍵"),
        tr("ダイナミック DNS 秘密鍵: %1\n\n"
           "この秘密鍵は、現在使用している DDNS 名と対応付けられています。現在 VPN Server として使用している PC が破損するなどして、"
           "この秘密鍵が失われると、その時設定されていた DDNS 名は占有されたままの状態となり、他の VPN Server で使用できなくなります。"
           "同じ名前を継続して使用したい場合は、秘密鍵を他の PC やインターネット上のストレージ、メモ用紙などに保管しておいてください。\n"
           "秘密鍵を新しい VPN Server に設定する際は、VPN Server の設定ファイルを編集します。\"declare DDnsClient\" ディレクティブ中にある "
           "\"byte Key\" に続く値を、保管しておいた秘密鍵の文字列で置き換えてください。\n"
           "なお、同時に複数の VPN Server で同じ秘密鍵を設定すると正常に動作しなくなりますので注意してください。")
            .arg(m_keyLabel->text()));
}

void DdnsDialog::onHint()
{
    // SM_DDNS_OK_MSG
    QMessageBox::information(
        this, tr("ダイナミック DNS 機能"),
        tr("ダイナミック DNS ホスト名: %1%2\n\n"
           "上記の DNS ホスト名を指定することにより、この VPN Server のグローバル IP アドレスである以下の IP アドレスに"
           "アクセスすることができます。\n\nIPv4 アドレス: %3\nIPv6 アドレス: %4\n\n"
           "なお、以下のような DNS ホスト名を指定することにより、IPv4 アドレスまたは IPv6 アドレスのいずれかのみを"
           "明示的に応答させることができます。\n\nIPv4 のみを応答するホスト名: %1.v4%2\nIPv6 のみを応答するホスト名: %1.v6%2")
            .arg(m_currentHostName, m_suffix, m_ipv4, m_ipv6));
}
