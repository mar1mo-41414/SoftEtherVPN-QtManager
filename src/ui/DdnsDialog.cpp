#include "DdnsDialog.h"
#include "DdnsProxyDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QJsonArray>
#include <QPointer>
#include <QTimer>
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

    auto note = [this](const QString &text, bool small = false) {
        auto *label = new QLabel(text, this);
        label->setWordWrap(true);
        if (small) {
            QFont font = label->font();
            font.setPointSizeF(font.pointSizeF() - 1);
            label->setFont(font);
        }
        return label;
    };

    // S_TITLE / S_BOLD / S_1 / S_22 / S_3
    auto *titleLabel = new QLabel(tr("ダイナミック DNS 機能"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleLabel->setFont(titleFont);
    auto *boldLabel = note(tr("このバージョンの VPN Server にはダイナミック DNS 機能が搭載されています。"));
    QFont boldFont = boldLabel->font();
    boldFont.setBold(true);
    boldLabel->setFont(boldFont);

    // S_4 / S_STATUS3〜5 / S_STATUS8
    m_fqdnLabel = new QLabel(this);
    m_ipv4Label = new QLabel(this);
    m_ipv6Label = new QLabel(this);
    m_keyLabel = new QLabel(this);
    m_keyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fqdnLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_hintButton = new QPushButton(tr("ヒント"), this);
    auto *keyHintButton = new QPushButton(tr("ヒント"), this);
    connect(m_hintButton, &QPushButton::clicked, this, &DdnsDialog::onHint);
    connect(keyHintButton, &QPushButton::clicked, this, &DdnsDialog::onKeyHint);

    auto *statusGroup = new QGroupBox(tr("現在の状態(S):"), this);
    auto *statusGrid = new QGridLayout(statusGroup);
    auto *fqdnCaption = new QLabel(tr("割当てられているダイナミック DNS ホスト名(H):"), this);
    QFont captionFont = fqdnCaption->font();
    captionFont.setBold(true);
    statusGrid->addWidget(fqdnCaption, 0, 0, 1, 2);
    statusGrid->addWidget(m_fqdnLabel, 1, 0);
    statusGrid->addWidget(m_hintButton, 1, 1);
    auto *ipv4Caption = new QLabel(tr("グローバル IPv4 アドレス:"), this);
    ipv4Caption->setFont(captionFont);
    statusGrid->addWidget(ipv4Caption, 2, 0, 1, 2);
    statusGrid->addWidget(m_ipv4Label, 3, 0, 1, 2);
    auto *ipv6Caption = new QLabel(tr("グローバル IPv6 アドレス:"), this);
    ipv6Caption->setFont(captionFont);
    statusGrid->addWidget(ipv6Caption, 4, 0, 1, 2);
    statusGrid->addWidget(m_ipv6Label, 5, 0, 1, 2);
    auto *keyRow = new QHBoxLayout;
    auto *keyCaption = new QLabel(tr("DNS 鍵:"), this);
    keyCaption->setFont(captionFont);
    keyRow->addWidget(keyCaption);
    keyRow->addWidget(m_keyLabel, 1);
    keyRow->addWidget(keyHintButton);
    statusGrid->addLayout(keyRow, 6, 0, 1, 2);
    statusGrid->setColumnStretch(0, 1);

    // S_5 / S_STATUS6 / S_STATUS7
    m_hostNameEdit = new QLineEdit(this);
    m_hostNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[A-Za-z0-9-]{0,31}")), this));
    m_suffixLabel = new QLabel(this);
    m_changeCaption = new QLabel(tr("ダイナミック DNS ホスト名の変更(C):"), this);
    m_changeCaption->setFont(captionFont);
    auto *hostLayout = new QHBoxLayout;
    hostLayout->addWidget(m_hostNameEdit, 1);
    hostLayout->addWidget(m_suffixLabel);
    m_changeButton = new QPushButton(tr("上記の DNS ホスト名に変更する(&A)"), this);
    m_restoreButton = new QPushButton(tr("変更前に戻す(&R)"), this);
    connect(m_changeButton, &QPushButton::clicked, this, &DdnsDialog::onChange);
    connect(m_restoreButton, &QPushButton::clicked, this, &DdnsDialog::onRestore);
    auto *changeButtons = new QHBoxLayout;
    changeButtons->addWidget(m_changeButton);
    changeButtons->addWidget(m_restoreButton);

    m_hostHint = note(tr("3 文字以上 31 文字以内の半角英数字およびハイフン '-' が使用できます。\n変更は何度でも可能です。"));
    auto *changeGroup = new QGroupBox(tr("設定の変更(M):"), this);
    auto *changeLayout = new QVBoxLayout(changeGroup);
    changeLayout->addWidget(m_changeCaption);
    changeLayout->addLayout(hostLayout);
    changeLayout->addWidget(m_hostHint);
    changeLayout->addLayout(changeButtons);
    changeLayout->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addWidget(statusGroup, 1);
    columns->addWidget(changeGroup, 1);

    // B_DISABLE / B_PROXY / IDCANCEL
    auto *disableButton = new QPushButton(tr("ダイナミック DNS 機能を無効にする(&D)"), this);
    m_proxyButton = new QPushButton(tr("プロキシサーバー経由で接続(&P)"), this);
    m_proxyButton->hide(); // b_support_ddns_proxy が真のサーバーでのみ表示する (公式Managerと同じ)
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(disableButton, &QPushButton::clicked, this, &DdnsDialog::onDisableHint);
    connect(m_proxyButton, &QPushButton::clicked, this, &DdnsDialog::onProxy);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(disableButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_proxyButton);
    bottomLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(boldLabel);
    layout->addWidget(note(tr("ダイナミック DNS により、この VPN Server コンピュータに永続的な固有の DNS ホスト名が割当てられます。これにより独自でドメインを所有していなくても、VPN Client や VPN Bridge などの設定画面上で VPN Server の IP アドレスの代わりに DNS ホスト名によって VPN Server を指定することができます。")));
    layout->addWidget(note(tr("また、IP アドレスが変化する可能性がある一般的な ISP を用いて VPN Server をインターネットに接続する場合でも、IP アドレスが変化すれば自動的に DNS ホストに対応する IP アドレスが更新されますので、可変 IP アドレスでも VPN Server を運用することができるようになります。\nこれにより、高価な月額料金が必要な固定グローバル IP アドレスのサービスを契約する必要がなくなります。"), true));
    layout->addWidget(note(tr("さらに、このバージョンの VPN Server は NAT トラバーサル機能をサポートしており、VPN Server が NAT の内側にありプライベート IP アドレスしか持っていない場合でも、NAT 上で特別な設定をすることなく、インターネット側からの VPN 接続を受付けることができます。")));
    layout->addLayout(columns);
    layout->addWidget(note(tr("IPv6 インターネットに接続されていない場合は上記の [IPv6 アドレス] の欄にエラーが表示されますが、異常ではありません。一部の国・地域では、行政機関による制限により、ダイナミック DNS サービスが利用できない場合があります。")));
    layout->addLayout(bottomLayout);

    resize(760, 700);
    reload();
    loadKey();
    loadCaps();

    // 公式Managerと同様に、状態は定期的に更新する (ホスト名入力欄は上書きしない)。
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { reload(/*silent=*/true); });
    timer->start(2000);
}

void DdnsDialog::loadCaps()
{
    QPointer<DdnsDialog> guard(this);
    m_rpc->call(
        QStringLiteral("GetCaps"), {},
        [this, guard](const QJsonObject &result) {
            if (!guard) {
                return;
            }
            for (const QJsonValue &value : result.value("CapsList").toArray()) {
                const QJsonObject cap = value.toObject();
                if (cap.value("CapsName_str").toString() == QLatin1String("b_support_ddns_proxy")) {
                    m_proxyButton->setVisible(cap.value("CapsValue_u32").toInt() != 0);
                }
            }
        },
        [](const RpcError &) {});
}

void DdnsDialog::onDisableHint()
{
    // SM_DISABLE_DDNS_HINT: 無効化のAPIは無く、設定ファイルの編集が必要
    QMessageBox::information(
        this, tr("ダイナミック DNS 機能を無効にする"),
        tr("ダイナミック DNS 機能を無効にするには、VPN Server の設定ファイルを編集します。\n\n"
           "\"declare root\" ディレクティブ内に \"declare DDnsClient\" ディレクティブがあります。この中にある \"bool Disable\" の値を "
           "true に設定して VPN Server を再起動することにより、ダイナミック DNS 機能が無効になります。"));
}

void DdnsDialog::loadKey()
{
    QPointer<DdnsDialog> guard(this);
    m_rpc->call(
        QStringLiteral("GetConfig"), {},
        [this, guard](const QJsonObject &result) {
            if (!guard) {
                return;
            }
            const QString config = QString::fromUtf8(QByteArray::fromBase64(result.value("FileData_bin").toString().toUtf8()));
            const QString key = extractDdnsKey(config);
            // SM_DDNS_KEY_ERR
            m_keyLabel->setText(key.isEmpty() ? tr("DNS 鍵の取得に失敗しました。") : key);
        },
        [this, guard](const RpcError &) {
            if (guard) {
                m_keyLabel->setText(tr("DNS 鍵の取得に失敗しました。"));
            }
        });
}

void DdnsDialog::reload(bool silent)
{
    QPointer<DdnsDialog> guard(this);
    m_rpc->call(
        QStringLiteral("GetDDnsClientStatus"), {},
        [this, guard](const QJsonObject &status) {
            if (!guard) {
                return;
            }
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
            if (!m_hostnameSet && !m_currentHostName.isEmpty()) {
                m_hostnameSet = true;
                m_hostNameEdit->setText(m_currentHostName);
            }
            const bool reachable = status.value("Err_IPv4_u32").toInt() == 0 || status.value("Err_IPv6_u32").toInt() == 0;
            m_suffixLabel->setText(reachable ? m_suffix : QString());
            for (QWidget *w : QList<QWidget *>{m_changeCaption, m_hostNameEdit, m_suffixLabel, m_hostHint, m_hintButton}) {
                w->setEnabled(reachable);
            }
            m_changeButton->setEnabled(reachable);
            m_restoreButton->setEnabled(reachable);
        },
        [this, silent, guard](const RpcError &error) {
            if (guard && !silent) {
                RpcUi::showError(this, tr("ダイナミック DNS 状態の取得"), error);
            }
        });
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
