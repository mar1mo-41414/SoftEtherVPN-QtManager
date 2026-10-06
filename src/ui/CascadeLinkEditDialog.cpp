#include "CascadeLinkEditDialog.h"
#include "CertInfoDialog.h"
#include "PolicyDialog.h"
#include "ProxySettingsDialog.h"

#include "util/Sha0.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QSslCertificate>
#include <QSslKey>
#include <QVBoxLayout>

namespace {

// 公式Managerがパスワードを変更しないときに表示する伏せ字。
const QString kHiddenPassword = QStringLiteral("********");

QLabel *note(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}

bool readCertificate(QWidget *parent, QByteArray *der)
{
    const QString path = QFileDialog::getOpenFileName(
        parent, QObject::tr("証明書の指定"), QString(),
        QObject::tr("X.509 証明書ファイル (*.cer *.crt *.pem *.der);;すべてのファイル (*)"));
    if (path.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(parent, QObject::tr("エラー"), QObject::tr("ファイルを開けませんでした: %1").arg(file.errorString()));
        return false;
    }
    const QByteArray data = file.readAll();
    QSslCertificate cert(data, QSsl::Pem);
    if (cert.isNull()) {
        cert = QSslCertificate(data, QSsl::Der);
    }
    if (cert.isNull()) {
        QMessageBox::warning(parent, QObject::tr("エラー"), QObject::tr("X.509 証明書として読み込めませんでした。"));
        return false;
    }
    *der = cert.toDer();
    return true;
}

} // namespace

CascadeLinkEditDialog::CascadeLinkEditDialog(bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_policy(PolicyDialog::defaultPolicy())
    , m_isNew(isNew)
{
    // 新規作成時は "新しい接続設定のプロパティ"、編集時は setValues で "<名前> のプロパティ"
    setWindowTitle(isNew ? tr("新しい接続設定のプロパティ") : tr("接続設定のプロパティ"));

    // ---------------- 左列 ----------------
    m_accountNameEdit = new QLineEdit(this);
    m_accountNameEdit->setEnabled(isNew);
    auto *nameForm = new QFormLayout;
    nameForm->addRow(tr("接続設定名(T):"), m_accountNameEdit);

    // 接続先 VPN Server の指定
    m_hostEdit = new QLineEdit(this);
    m_portCombo = new QComboBox(this);
    m_portCombo->setEditable(true);
    m_portCombo->addItems({QStringLiteral("443"), QStringLiteral("992"), QStringLiteral("1194"), QStringLiteral("5555")});
    m_portCombo->setValidator(new QIntValidator(1, 65535, this));
    m_hubCombo = new QComboBox(this);
    m_hubCombo->setEditable(true);
    m_hubCombo->setCurrentText(QString());
    auto *natT = new QCheckBox(tr("NAT-T 無効"), this);
    natT->setEnabled(false);
    auto *portRow = new QHBoxLayout;
    portRow->addWidget(m_portCombo, 1);
    portRow->addWidget(natT);
    auto *destForm = new QFormLayout;
    destForm->setLabelAlignment(Qt::AlignRight);
    destForm->addRow(tr("ホスト名(H):"), m_hostEdit);
    destForm->addRow(tr("ポート番号(P):"), portRow);
    destForm->addRow(tr("仮想 HUB 名(V):"), m_hubCombo);
    auto *destGroup = new QGroupBox(tr("接続先 VPN Server の指定(B):"), this);
    auto *destLayout = new QVBoxLayout(destGroup);
    destLayout->addWidget(note(
        tr("接続したい VPN Server が動作しているコンピュータのホスト名または IP アドレス、ポート番号、および仮想 HUB 名を指定してください。"),
        this));
    destLayout->addLayout(destForm);

    // 経由するプロキシサーバーの設定
    m_directRadio = new QRadioButton(tr("直接 TCP/IP 接続 (プロキシを使わない)(D)"), this);
    m_httpRadio = new QRadioButton(tr("HTTP プロキシサーバー経由接続(Q)"), this);
    m_socksRadio = new QRadioButton(tr("SOCKS プロキシサーバー経由接続(S)"), this);
    m_directRadio->setChecked(true);
    m_proxyButton = new QPushButton(tr("プロキシサーバーの接続設定(&2)"), this);
    auto *proxyGroup = new QGroupBox(tr("経由するプロキシサーバーの設定(X):"), this);
    auto *proxyLayout = new QVBoxLayout(proxyGroup);
    proxyLayout->addWidget(note(tr("プロキシサーバーを経由して VPN Server に接続することができます。"), this));
    proxyLayout->addWidget(new QLabel(tr("プロキシの種類(M):"), this));
    proxyLayout->addWidget(m_directRadio);
    proxyLayout->addWidget(m_httpRadio);
    proxyLayout->addWidget(m_socksRadio);
    proxyLayout->addWidget(m_proxyButton, 0, Qt::AlignHCenter);

    // サーバー証明書の検証オプション
    m_checkCertCheck = new QCheckBox(tr("サーバー証明書を必ず検証する(&3)"), this);
    auto *trustButton = new QPushButton(tr("信頼する証明機関の証明書の管理(&4)"), this);
    trustButton->setEnabled(false);
    trustButton->setToolTip(tr("未実装"));
    auto *loadServerCertButton = new QPushButton(tr("固有証明書の登録(&R)"), this);
    m_viewServerCertButton = new QPushButton(tr("固有証明書の表示(&5)"), this);
    auto *certButtons = new QHBoxLayout;
    certButtons->addWidget(loadServerCertButton);
    certButtons->addWidget(m_viewServerCertButton);
    auto *certGroup = new QGroupBox(tr("サーバー証明書の検証オプション(F):"), this);
    auto *certLayout = new QVBoxLayout(certGroup);
    certLayout->addWidget(m_checkCertCheck);
    certLayout->addWidget(trustButton);
    certLayout->addLayout(certButtons);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(note(tr("VPN Server への接続設定を行います。"), this));
    leftColumn->addLayout(nameForm);
    leftColumn->addWidget(destGroup);
    leftColumn->addWidget(proxyGroup);
    leftColumn->addWidget(certGroup);
    leftColumn->addStretch();

    // ---------------- 右列 ----------------
    // カスケード接続の設定 (セキュリティポリシー)
    auto *policyButton = new QPushButton(tr("セキュリティポリシー(&L)"), this);
    auto *policyGroup = new QGroupBox(tr("カスケード接続の設定"), this);
    auto *policyLayout = new QVBoxLayout(policyGroup);
    policyLayout->addWidget(note(
        tr("カスケード接続を行う際に、この仮想 HUB 側で生成されるセッションに適用するセキュリティポリシーを設定することができます。"),
        this));
    policyLayout->addWidget(policyButton, 0, Qt::AlignHCenter);

    // ユーザー認証
    m_authCombo = new QComboBox(this);
    m_authCombo->addItems({tr("匿名認証"), tr("標準パスワード認証"), tr("RADIUS または NT ドメイン認証"), tr("クライアント証明書認証")});
    m_authCombo->setCurrentIndex(1);
    m_usernameEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordLabel = new QLabel(tr("パスワード(Y):"), this);
    m_certInfoLabel = note(tr("ユーザー認証に使用するクライアント証明書を指定する必要があります。"), this);
    m_clientCertButton = new QPushButton(tr("クライアント証明書の指定(&8)"), this);
    auto *authGrid = new QGridLayout;
    authGrid->addWidget(new QLabel(tr("認証の種類(6):"), this), 0, 0, Qt::AlignRight);
    authGrid->addWidget(m_authCombo, 0, 1);
    authGrid->addWidget(new QLabel(tr("ユーザー名(U):"), this), 1, 0, Qt::AlignRight);
    authGrid->addWidget(m_usernameEdit, 1, 1);
    authGrid->addWidget(m_passwordLabel, 2, 0, Qt::AlignRight);
    authGrid->addWidget(m_passwordEdit, 2, 1);
    authGrid->addWidget(m_certInfoLabel, 3, 0, 1, 2);
    authGrid->addWidget(m_clientCertButton, 4, 0, 1, 2, Qt::AlignHCenter);
    authGrid->setColumnStretch(1, 1);
    auto *authGroup = new QGroupBox(tr("ユーザー認証(A):"), this);
    auto *authLayout = new QVBoxLayout(authGroup);
    authLayout->addWidget(note(tr("VPN Server に接続する際に必要なユーザー認証情報を設定してください。"), this));
    authLayout->addLayout(authGrid);

    // 通信の詳細設定 (再接続設定はカスケード接続では無効)
    auto *retryCheck = new QCheckBox(tr("VPN Server との通信が切断された場合は再接続する(Z)"), this);
    retryCheck->setChecked(true);
    auto *retryNum = new QLineEdit(this);
    auto *retrySpan = new QLineEdit(QStringLiteral("15"), this);
    auto *infiniteCheck = new QCheckBox(tr("無限に再接続を試行する (常時接続)(I)"), this);
    infiniteCheck->setChecked(true);
    auto *advancedButton = new QPushButton(tr("高度な通信設定(&N)..."), this);
    auto *retryGrid = new QGridLayout;
    retryGrid->addWidget(new QLabel(tr("再接続回数(C):"), this), 0, 0, Qt::AlignRight);
    retryGrid->addWidget(retryNum, 0, 1);
    retryGrid->addWidget(new QLabel(tr("回"), this), 0, 2);
    retryGrid->addWidget(new QLabel(tr("再接続間隔(K):"), this), 1, 0, Qt::AlignRight);
    retryGrid->addWidget(retrySpan, 1, 1);
    retryGrid->addWidget(new QLabel(tr("秒"), this), 1, 2);
    auto *advancedRow = new QHBoxLayout;
    advancedRow->addStretch();
    advancedRow->addWidget(advancedButton);
    for (QWidget *w : QList<QWidget *>{retryCheck, retryNum, retrySpan, infiniteCheck}) {
        w->setEnabled(false);
    }
    auto *commGroup = new QGroupBox(tr("通信の詳細設定(G):"), this);
    auto *commLayout = new QVBoxLayout(commGroup);
    commLayout->addWidget(retryCheck);
    commLayout->addLayout(retryGrid);
    commLayout->addWidget(infiniteCheck);
    commLayout->addLayout(advancedRow);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CascadeLinkEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(policyGroup);
    rightColumn->addWidget(authGroup);
    rightColumn->addWidget(commGroup);
    rightColumn->addStretch();
    rightColumn->addWidget(buttonBox);

    auto *columns = new QHBoxLayout(this);
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);

    connect(m_accountNameEdit, &QLineEdit::textChanged, this, &CascadeLinkEditDialog::updateState);
    connect(m_hostEdit, &QLineEdit::textChanged, this, &CascadeLinkEditDialog::updateState);
    connect(m_hubCombo, &QComboBox::editTextChanged, this, &CascadeLinkEditDialog::updateState);
    connect(m_authCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &CascadeLinkEditDialog::updateState);
    connect(m_usernameEdit, &QLineEdit::textChanged, this, &CascadeLinkEditDialog::updateState);
    for (QRadioButton *radio : {m_directRadio, m_httpRadio, m_socksRadio}) {
        connect(radio, &QRadioButton::toggled, this, &CascadeLinkEditDialog::updateState);
    }
    connect(m_proxyButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onProxySettings);
    connect(loadServerCertButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onLoadServerCert);
    connect(m_viewServerCertButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onViewServerCert);
    connect(policyButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onPolicy);
    connect(m_clientCertButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onLoadClientCert);
    connect(advancedButton, &QPushButton::clicked, this, &CascadeLinkEditDialog::onAdvanced);

    resize(900, 640);
    updateState();
}

int CascadeLinkEditDialog::authType() const
{
    return m_authCombo->currentIndex();
}

int CascadeLinkEditDialog::proxyType() const
{
    return m_httpRadio->isChecked() ? 1 : (m_socksRadio->isChecked() ? 2 : 0);
}

void CascadeLinkEditDialog::updateState()
{
    const int type = authType();
    m_usernameEdit->setEnabled(type != 0);
    m_passwordEdit->setEnabled(type == 1 || type == 2);
    m_passwordLabel->setEnabled(type == 1 || type == 2);
    m_certInfoLabel->setVisible(type == 3);
    m_clientCertButton->setVisible(type == 3);
    m_proxyButton->setEnabled(proxyType() != 0);
    m_viewServerCertButton->setEnabled(!m_serverCert.isEmpty());

    bool ok = !m_accountNameEdit->text().trimmed().isEmpty() && !m_hostEdit->text().trimmed().isEmpty() &&
              !m_hubCombo->currentText().trimmed().isEmpty();
    if (type != 0) {
        ok = ok && !m_usernameEdit->text().trimmed().isEmpty();
    }
    if (type == 3) {
        ok = ok && !m_clientCert.isEmpty() && !m_clientKey.isEmpty();
    }
    if (proxyType() != 0) {
        ok = ok && !m_proxyHost.isEmpty();
    }
    m_okButton->setEnabled(ok);
}

void CascadeLinkEditDialog::setValues(const QJsonObject &link)
{
    const QString name = link.value("AccountName_utf").toString();
    setWindowTitle(tr("%1 のプロパティ").arg(name));
    m_accountNameEdit->setText(name);
    m_hostEdit->setText(link.value("Hostname_str").toString());
    m_portCombo->setCurrentText(QString::number(link.value("Port_u32").toInt(443)));
    m_hubCombo->setCurrentText(link.value("HubName_str").toString());

    switch (link.value("ProxyType_u32").toInt()) {
    case 1:
        m_httpRadio->setChecked(true);
        break;
    case 2:
        m_socksRadio->setChecked(true);
        break;
    default:
        m_directRadio->setChecked(true);
        break;
    }
    m_proxyHost = link.value("ProxyName_str").toString();
    m_proxyPort = link.value("ProxyPort_u32").toInt(8080);
    m_proxyUser = link.value("ProxyUsername_str").toString();
    m_proxyPassword = link.value("ProxyPassword_str").toString();

    m_checkCertCheck->setChecked(link.value("CheckServerCert_bool").toBool());
    m_serverCert = QByteArray::fromBase64(link.value("ServerCert_bin").toString().toLatin1());

    m_policy = PolicyDialog::defaultPolicy();
    const QJsonObject policy = PolicyDialog::extract(link);
    for (auto it = policy.begin(); it != policy.end(); ++it) {
        m_policy[it.key()] = it.value();
    }

    // AuthType: 0=匿名 1=SHA-0ハッシュ(標準パスワード) 2=平文(RADIUS/NT) 3=証明書
    const int type = link.value("AuthType_u32").toInt();
    m_authCombo->setCurrentIndex(qBound(0, type, 3));
    m_usernameEdit->setText(link.value("Username_str").toString());
    if (type == 1) {
        m_originalHashed = QByteArray::fromBase64(link.value("HashedPassword_bin").toString().toLatin1());
        m_passwordEdit->setText(kHiddenPassword);
    } else if (type == 2) {
        m_passwordEdit->setText(link.value("PlainPassword_str").toString());
    } else if (type == 3) {
        m_clientCert = QByteArray::fromBase64(link.value("ClientX_bin").toString().toLatin1());
        m_clientKey = QByteArray::fromBase64(link.value("ClientK_bin").toString().toLatin1());
    }

    m_maxConnection = link.value("MaxConnection_u32").toInt(8);
    m_interval = link.value("AdditionalConnectionInterval_u32").toInt(1);
    m_disconnectSpan = link.value("ConnectionDisconnectSpan_u32").toInt();
    m_halfConnection = link.value("HalfConnection_bool").toBool();
    m_disableQoS = link.value("DisableQoS_bool").toBool();
    m_useEncrypt = link.value("UseEncrypt_bool").toBool(true);
    m_useCompress = link.value("UseCompress_bool").toBool();
    m_noUdp = link.value("NoUdpAcceleration_bool").toBool();
    updateState();
}

QJsonObject CascadeLinkEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["AccountName_utf"] = m_accountNameEdit->text().trimmed();
    params["Hostname_str"] = m_hostEdit->text().trimmed();
    params["Port_u32"] = m_portCombo->currentText().toInt();
    params["HubName_str"] = m_hubCombo->currentText().trimmed();

    params["ProxyType_u32"] = proxyType();
    if (proxyType() != 0) {
        params["ProxyName_str"] = m_proxyHost;
        params["ProxyPort_u32"] = m_proxyPort;
        params["ProxyUsername_str"] = m_proxyUser;
        params["ProxyPassword_str"] = m_proxyPassword;
    }

    params["CheckServerCert_bool"] = m_checkCertCheck->isChecked();
    if (!m_serverCert.isEmpty()) {
        params["ServerCert_bin"] = QString::fromLatin1(m_serverCert.toBase64());
    }

    const int type = authType();
    params["AuthType_u32"] = type;
    if (type != 0) {
        params["Username_str"] = m_usernameEdit->text().trimmed();
    }
    if (type == 1) {
        const bool unchanged = !m_isNew && m_passwordEdit->text() == kHiddenPassword && !m_originalHashed.isEmpty();
        const QByteArray hashed =
            unchanged ? m_originalHashed : Sha0::passwordHash(m_usernameEdit->text().trimmed(), m_passwordEdit->text());
        params["HashedPassword_bin"] = QString::fromLatin1(hashed.toBase64());
    } else if (type == 2) {
        params["PlainPassword_str"] = m_passwordEdit->text();
    } else if (type == 3) {
        params["ClientX_bin"] = QString::fromLatin1(m_clientCert.toBase64());
        params["ClientK_bin"] = QString::fromLatin1(m_clientKey.toBase64());
    }

    params["MaxConnection_u32"] = m_maxConnection;
    params["AdditionalConnectionInterval_u32"] = m_interval;
    params["ConnectionDisconnectSpan_u32"] = m_disconnectSpan;
    params["HalfConnection_bool"] = m_halfConnection;
    params["DisableQoS_bool"] = m_disableQoS;
    params["UseEncrypt_bool"] = m_useEncrypt;
    params["UseCompress_bool"] = m_useCompress;
    params["NoUdpAcceleration_bool"] = m_noUdp;

    for (auto it = m_policy.begin(); it != m_policy.end(); ++it) {
        params[it.key()] = it.value();
    }
    return params;
}

QString CascadeLinkEditDialog::accountName() const
{
    return m_accountNameEdit->text().trimmed();
}

void CascadeLinkEditDialog::accept()
{
    bool portOk = false;
    const int port = m_portCombo->currentText().toInt(&portOk);
    if (!portOk || port < 1 || port > 65535) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ポート番号は 1 ～ 65535 の範囲で指定してください。"));
        return;
    }
    QDialog::accept();
}

void CascadeLinkEditDialog::onProxySettings()
{
    ProxySettingsDialog dialog(this);
    dialog.setValues(m_proxyHost, static_cast<quint16>(m_proxyPort), m_proxyUser, m_proxyPassword);
    if (dialog.exec() == QDialog::Accepted) {
        m_proxyHost = dialog.host();
        m_proxyPort = dialog.port();
        m_proxyUser = dialog.user();
        m_proxyPassword = dialog.password();
        updateState();
    }
}

void CascadeLinkEditDialog::onLoadServerCert()
{
    QByteArray der;
    if (readCertificate(this, &der)) {
        m_serverCert = der;
        updateState();
    }
}

void CascadeLinkEditDialog::onViewServerCert()
{
    if (!m_serverCert.isEmpty()) {
        CertInfoDialog dialog(m_serverCert, this);
        dialog.exec();
    }
}

void CascadeLinkEditDialog::onPolicy()
{
    // SM_LINK_POLICY_CAPTION
    const QString title = tr("カスケードセッションに適用するセキュリティポリシーの設定");
    PolicyDialog dialog(title, title, m_policy, this, /*cascadeMode=*/true);
    if (dialog.exec() == QDialog::Accepted) {
        m_policy = dialog.policy();
    }
}

void CascadeLinkEditDialog::onLoadClientCert()
{
    QByteArray certDer;
    if (!readCertificate(this, &certDer)) {
        return;
    }
    const QString keyPath = QFileDialog::getOpenFileName(this, tr("秘密鍵の指定"), QString(),
                                                          tr("秘密鍵ファイル (*.key *.pem);;すべてのファイル (*)"));
    if (keyPath.isEmpty()) {
        return;
    }
    QFile file(keyPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを開けませんでした: %1").arg(file.errorString()));
        return;
    }
    const QByteArray data = file.readAll();
    QSslKey key(data, QSsl::Rsa, QSsl::Pem);
    if (key.isNull()) {
        key = QSslKey(data, QSsl::Ec, QSsl::Pem);
    }
    if (key.isNull()) {
        QMessageBox::warning(this, tr("エラー"), tr("秘密鍵として読み込めませんでした (暗号化された鍵には対応していません)。"));
        return;
    }
    m_clientCert = certDer;
    m_clientKey = key.toDer();
    updateState();
}

// D_CM_DETAIL
void CascadeLinkEditDialog::onAdvanced()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("高度な通信設定"));

    auto *connectionCombo = new QComboBox(&dialog);
    for (int i = 1; i <= 32; ++i) {
        connectionCombo->addItem(QString::number(i), i);
    }
    connectionCombo->setCurrentIndex(qBound(0, m_maxConnection - 1, 31));
    auto *intervalSpin = new QSpinBox(&dialog);
    intervalSpin->setRange(1, 4294967);
    intervalSpin->setValue(m_interval);
    auto *lifetimeCheck = new QCheckBox(tr("各 TCP コネクションの寿命を設定する(&A)"), &dialog);
    auto *lifetimeSpin = new QSpinBox(&dialog);
    lifetimeSpin->setRange(5, 4294967);
    lifetimeCheck->setChecked(m_disconnectSpan > 0);
    lifetimeSpin->setValue(m_disconnectSpan > 0 ? m_disconnectSpan : 600);
    lifetimeSpin->setEnabled(m_disconnectSpan > 0);
    connect(lifetimeCheck, &QCheckBox::toggled, lifetimeSpin, &QSpinBox::setEnabled);
    auto *halfCheck = new QCheckBox(tr("半二重モードを使用する(&H)"), &dialog);
    halfCheck->setChecked(m_halfConnection);
    auto *qosCheck = new QCheckBox(tr("VoIP / QoS 対応機能を無効にする(&Q)"), &dialog);
    qosCheck->setChecked(m_disableQoS);
    auto *encryptCheck = new QCheckBox(tr("VPN Server との間の通信を SSL で暗号化する(&E)"), &dialog);
    encryptCheck->setChecked(m_useEncrypt);
    auto *compressCheck = new QCheckBox(tr("データ圧縮を使用する(&U)"), &dialog);
    compressCheck->setChecked(m_useCompress);
    auto *udpCheck = new QCheckBox(tr("UDP 高速化機能を無効にする"), &dialog);
    udpCheck->setChecked(m_noUdp);

    auto *header = new QLabel(
        tr("ネットワーク、通信プロトコル、およびセキュリティに関する詳しい知識をお持ちの方とシステム管理者向けのオプションです。VPN プロトコルの通信設定をカスタマイズできます。"),
        &dialog);
    header->setWordWrap(true);

    // 左: VPN 通信の最適化
    auto *optimizeGroup = new QGroupBox(tr("VPN 通信の最適化(T):"), &dialog);
    auto *optimizeLayout = new QVBoxLayout(optimizeGroup);
    optimizeLayout->addWidget(note(
        tr("VPN Server との間の VPN 通信セッションにおけるデータ伝送に複数本の TCP コネクションを束ねて使用することにより、通信速度を向上できる場合があります。"),
        &dialog));
    auto *connectionRow = new QHBoxLayout;
    connectionRow->addWidget(new QLabel(tr("VPN 通信に使用する TCP コネクション数(N):"), &dialog));
    optimizeLayout->addLayout(connectionRow);
    auto *connectionRow2 = new QHBoxLayout;
    connectionRow2->addWidget(connectionCombo);
    connectionRow2->addWidget(new QLabel(tr("本"), &dialog));
    connectionRow2->addStretch();
    optimizeLayout->addLayout(connectionRow2);
    optimizeLayout->addWidget(note(tr("※ サーバーへの接続回線が高速な場合は 8 本程度を、ダイヤルアップ等の低速な場合は 1 本をお勧めします。"), &dialog));
    optimizeLayout->addWidget(new QLabel(tr("詳細設定:"), &dialog));
    auto *intervalRow = new QHBoxLayout;
    intervalRow->addWidget(new QLabel(tr("各 TCP コネクションの確立間隔(S):"), &dialog));
    intervalRow->addWidget(intervalSpin);
    intervalRow->addWidget(new QLabel(tr("秒"), &dialog));
    intervalRow->addStretch();
    optimizeLayout->addLayout(intervalRow);
    auto *lifetimeRow = new QHBoxLayout;
    lifetimeRow->addWidget(lifetimeCheck);
    lifetimeRow->addWidget(lifetimeSpin);
    lifetimeRow->addWidget(new QLabel(tr("秒"), &dialog));
    lifetimeRow->addStretch();
    optimizeLayout->addLayout(lifetimeRow);
    optimizeLayout->addWidget(note(
        tr("2 本以上の TCP コネクションを束ねて VPN 通信を行う際、「半二重モード」を使用することができます。\n半二重モードを有効にすると、自動的に各 TCP コネクションのデータ伝送方向を半数ずつ固定することができます。\n\nたとえば、8 本の TCP コネクションを使用して VPN セッションを確立した場合、半二重モードを有効にすると、4 本の TCP コネクションはアップロード方向専用、残りの 4 本のコネクションはダウンロード方向専用に固定され通信が行われます。"),
        &dialog));
    optimizeLayout->addWidget(halfCheck);
    optimizeLayout->addWidget(note(
        tr("VoIP / QoS 対応機能を使用すると、IP 電話パケットなどの優先度の高いパケットを VPN 内で高速に伝送できます。"), &dialog));
    optimizeLayout->addWidget(qosCheck);

    // 右: 暗号化と圧縮 / 接続モード / その他
    auto *encryptGroup = new QGroupBox(tr("暗号化と圧縮(C):"), &dialog);
    auto *encryptLayout = new QVBoxLayout(encryptGroup);
    encryptLayout->addWidget(note(
        tr("通常は、VPN Server との間の通信を SSL で暗号化して、情報の盗聴や改ざんを防止します。暗号化を無効にすることもできます。暗号化を無効にすると、通信のスループットが向上しますが、通信データは平文でネットワーク上を流れます。"),
        &dialog));
    encryptLayout->addWidget(encryptCheck);
    encryptLayout->addWidget(note(
        tr("データ圧縮技術を使用して、VPN 通信を圧縮することができます。ダイヤルアップ接続やモバイル接続など、低速回線でのみ使用してください。"),
        &dialog));
    encryptLayout->addWidget(compressCheck);
    encryptLayout->addWidget(udpCheck);

    auto *bridgeCheck = new QCheckBox(tr("ブリッジ / ルータモードで接続(&B)"), &dialog);
    bridgeCheck->setChecked(true);
    bridgeCheck->setEnabled(false);
    auto *monitorCheck = new QCheckBox(tr("モニタリングモードで接続(&D)"), &dialog);
    monitorCheck->setEnabled(false);
    auto *modeGroup = new QGroupBox(tr("接続モードの選択(M):"), &dialog);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    modeLayout->addWidget(note(tr("カスケード接続では、常に [ブリッジ / ルータモードで接続] が有効になっています。"), &dialog));
    modeLayout->addWidget(bridgeCheck);
    modeLayout->addWidget(monitorCheck);

    auto *routingCheck = new QCheckBox(tr("ルーティングテーブルの調整処理を行わない(&R)"), &dialog);
    routingCheck->setEnabled(false);
    auto *otherGroup = new QGroupBox(tr("その他の設定(G):"), &dialog);
    auto *otherLayout = new QVBoxLayout(otherGroup);
    otherLayout->addWidget(routingCheck);

    auto *warning = note(tr("この設定画面の設定項目は、システム管理者から指示があった場合や、ネットワークやセキュリティに関して詳しい知識をお持ちの場合以外は変更しないでください。"), &dialog);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(encryptGroup);
    rightColumn->addWidget(modeGroup);
    rightColumn->addWidget(otherGroup);
    rightColumn->addWidget(warning);
    rightColumn->addStretch();
    rightColumn->addWidget(buttonBox);

    auto *columns = new QHBoxLayout;
    columns->addWidget(optimizeGroup, 1);
    columns->addLayout(rightColumn, 1);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(header);
    layout->addLayout(columns, 1);
    dialog.resize(900, 700);

    if (dialog.exec() == QDialog::Accepted) {
        m_maxConnection = connectionCombo->currentData().toInt();
        m_interval = intervalSpin->value();
        m_disconnectSpan = lifetimeCheck->isChecked() ? lifetimeSpin->value() : 0;
        m_halfConnection = halfCheck->isChecked();
        m_disableQoS = qosCheck->isChecked();
        m_useEncrypt = encryptCheck->isChecked();
        m_useCompress = compressCheck->isChecked();
        m_noUdp = udpCheck->isChecked();
    }
}
