#include "ServerSettingsDialog.h"
#include "AdminPasswordDialog.h"
#include "CertInfoDialog.h"
#include "SpecialListenerDialog.h"

#include "util/RpcUiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

ServerSettingsDialog::ServerSettingsDialog(VpnServerRpc *rpc, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_SSL CAPTION
    setWindowTitle(tr("暗号化と通信関係の設定"));

    auto *introLabel =
        new QLabel(tr("この VPN Server の暗号化、通信、およびセキュリティに関する設定を参照または変更することができます。"), this);
    introLabel->setWordWrap(true);

    // STATIC2〜4
    m_cipherCombo = new QComboBox(this);
    m_cipherCombo->setEditable(true);
    m_cipherCombo->addItems({QStringLiteral("ECDHE-RSA-AES128-GCM-SHA256"), QStringLiteral("ECDHE-RSA-AES256-GCM-SHA384"),
                              QStringLiteral("AES128-SHA"), QStringLiteral("AES256-SHA"), QStringLiteral("DHE-RSA-AES256-SHA")});
    auto *cipherForm = new QFormLayout;
    cipherForm->addRow(tr("暗号化アルゴリズム名(&C):"), m_cipherCombo);
    auto *cipherGroup = new QGroupBox(tr("使用する暗号化アルゴリズム(&A)"), this);
    cipherGroup->setLayout(cipherForm);

    // STATIC6/7 + B_IMPORT/B_EXPORT/B_VIEW/B_REGENERATE
    auto *importButton = new QPushButton(tr("インポート(&I)"), this);
    auto *exportButton = new QPushButton(tr("エクスポート(&X)"), this);
    auto *viewButton = new QPushButton(tr("証明書の表示(&V)"), this);
    auto *regenerateButton = new QPushButton(tr("新規作成(&N)"), this);
    connect(importButton, &QPushButton::clicked, this, &ServerSettingsDialog::onImportCert);
    connect(exportButton, &QPushButton::clicked, this, &ServerSettingsDialog::onExportCert);
    connect(viewButton, &QPushButton::clicked, this, &ServerSettingsDialog::onViewCert);
    connect(regenerateButton, &QPushButton::clicked, this, &ServerSettingsDialog::onRegenerateCert);

    auto *certButtonLayout = new QHBoxLayout;
    certButtonLayout->addWidget(importButton);
    certButtonLayout->addWidget(exportButton);
    certButtonLayout->addWidget(viewButton);
    certButtonLayout->addWidget(regenerateButton);
    auto *certGroup = new QGroupBox(tr("サーバー証明書(&E)"), this);
    auto *certGroupLayout = new QVBoxLayout(certGroup);
    certGroupLayout->addWidget(
        new QLabel(tr("この VPN Server がクライアントに対して提示する X509 証明書と秘密鍵を指定してください。"), this));
    certGroupLayout->addLayout(certButtonLayout);

    // STATIC12〜15
    m_syslogCombo = new QComboBox(this);
    // SM_SYSLOG_0〜3
    m_syslogCombo->addItem(tr("syslog 送信機能を使用しない"), 0);
    m_syslogCombo->addItem(tr("サーバーログを syslog で送信"), 1);
    m_syslogCombo->addItem(tr("サーバーおよび仮想 HUB セキュリティログを syslog で送信"), 2);
    m_syslogCombo->addItem(tr("サーバー、仮想 HUB セキュリティおよびパケットログを syslog で送信"), 3);
    m_syslogHostEdit = new QLineEdit(this);
    m_syslogPortSpin = new QSpinBox(this);
    m_syslogPortSpin->setRange(1, 65535);
    m_syslogPortSpin->setValue(514);

    auto *syslogForm = new QFormLayout;
    syslogForm->addRow(m_syslogCombo);
    syslogForm->addRow(tr("syslog サーバーホスト名(&S):"), m_syslogHostEdit);
    syslogForm->addRow(tr("ポート番号(&R):"), m_syslogPortSpin);
    auto *syslogGroup = new QGroupBox(tr("syslog 送信機能"), this);
    syslogGroup->setLayout(syslogForm);

    // STATIC8〜S_INFO: インターネット接続の維持機能
    m_keepCheck = new QCheckBox(tr("インターネット接続の維持機能を使用する(&K)"), this);
    m_keepHostEdit = new QLineEdit(this);
    m_keepPortSpin = new QSpinBox(this);
    m_keepPortSpin->setRange(1, 65535);
    m_keepPortSpin->setValue(80);
    m_keepIntervalSpin = new QSpinBox(this);
    m_keepIntervalSpin->setRange(1, 3600);
    m_keepIntervalSpin->setValue(50);
    m_keepIntervalSpin->setSuffix(tr(" 秒"));
    m_keepTcpRadio = new QRadioButton(tr("TCP/IP プロトコル(&T)"), this);
    m_keepUdpRadio = new QRadioButton(tr("UDP/IP プロトコル(&U)"), this);
    m_keepTcpRadio->setChecked(true);
    auto *keepProtocolLayout = new QHBoxLayout;
    keepProtocolLayout->addWidget(m_keepTcpRadio);
    keepProtocolLayout->addWidget(m_keepUdpRadio);
    auto *keepForm = new QFormLayout;
    keepForm->addRow(m_keepCheck);
    keepForm->addRow(tr("ホスト名(&H):"), m_keepHostEdit);
    keepForm->addRow(tr("ポート番号(&F):"), m_keepPortSpin);
    keepForm->addRow(tr("パケット送出間隔(&D):"), m_keepIntervalSpin);
    keepForm->addRow(tr("プロトコル(&L):"), keepProtocolLayout);
    auto *keepHint = new QLabel(
        tr("一定期間無通信状態が続くと接続が自動的に切断されるようなネットワーク接続環境の場合、インターネット上の任意のサーバーに対して"
           "一定間隔ごとにパケットを送信することにより、インターネット接続を維持することができます。"
           "送信されるパケットはランダムな内容であり、個人情報などが送信されることはありません。"),
        this);
    keepHint->setWordWrap(true);
    auto *keepGroup = new QGroupBox(tr("インターネット接続の維持機能"), this);
    auto *keepLayout = new QVBoxLayout(keepGroup);
    keepLayout->addWidget(keepHint);
    keepLayout->addLayout(keepForm);

    // B_PASSWORD / B_SPECIALLISTENER
    auto *passwordButton = new QPushButton(tr("管理者パスワードの変更(&P)"), this);
    auto *specialButton = new QPushButton(tr("VPN over ICMP / DNS 設定"), this);
    connect(passwordButton, &QPushButton::clicked, this, &ServerSettingsDialog::onChangePassword);
    connect(specialButton, &QPushButton::clicked, this, &ServerSettingsDialog::onSpecialListener);
    auto *extraButtons = new QHBoxLayout;
    extraButtons->addWidget(passwordButton);
    extraButtons->addWidget(specialButton);
    extraButtons->addStretch();

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerSettingsDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(introLabel);
    layout->addWidget(cipherGroup);
    layout->addWidget(certGroup);
    layout->addWidget(syslogGroup);
    layout->addWidget(keepGroup);
    layout->addLayout(extraButtons);
    layout->addWidget(buttonBox);

    resize(480, sizeHint().height());
    reload();
}

void ServerSettingsDialog::reload()
{
    m_rpc->getServerCipher(
        [this](const QJsonObject &result) { m_cipherCombo->setCurrentText(result.value("String_str").toString()); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("暗号化アルゴリズムの取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });

    m_rpc->getServerCert(
        [this](const QJsonObject &result) {
            m_certDer = QByteArray::fromBase64(result.value("Cert_bin").toString().toUtf8());
            m_keyDer = QByteArray::fromBase64(result.value("Key_bin").toString().toUtf8());
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("サーバー証明書の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });

    m_rpc->call(
        QStringLiteral("GetKeep"), {},
        [this](const QJsonObject &result) {
            m_keepCheck->setChecked(result.value("UseKeepConnect_bool").toBool());
            m_keepHostEdit->setText(result.value("KeepConnectHost_str").toString());
            const int port = result.value("KeepConnectPort_u32").toInt();
            m_keepPortSpin->setValue(port > 0 ? port : 80);
            const int interval = result.value("KeepConnectInterval_u32").toInt();
            m_keepIntervalSpin->setValue(interval > 0 ? interval : 50);
            const bool udp = result.value("KeepConnectProtocol_u32").toInt() == 1;
            m_keepUdpRadio->setChecked(udp);
            m_keepTcpRadio->setChecked(!udp);
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("インターネット接続維持機能の設定取得"), error); });

    m_rpc->getSysLog(
        [this](const QJsonObject &result) {
            const int index = m_syslogCombo->findData(result.value("SaveType_u32").toInt());
            m_syslogCombo->setCurrentIndex(index >= 0 ? index : 0);
            m_syslogHostEdit->setText(result.value("Hostname_str").toString());
            const int port = result.value("Port_u32").toInt();
            m_syslogPortSpin->setValue(port > 0 ? port : 514);
        },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("syslog 設定の取得に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ServerSettingsDialog::onImportCert()
{
    // SoftEtherのManagerは.CER(DER形式のX509証明書)/.KEY(DER形式の秘密鍵)の組を扱う。
    // PEM形式のファイルをそのまま読み込むことはできない (将来対応)。
    const QString certPath = QFileDialog::getOpenFileName(this, tr("証明書ファイルを選択"), QString(),
                                                            tr("証明書ファイル (*.cer *.crt);;すべてのファイル (*.*)"));
    if (certPath.isEmpty()) {
        return;
    }
    const QString keyPath = QFileDialog::getOpenFileName(this, tr("秘密鍵ファイルを選択"), QString(),
                                                           tr("秘密鍵ファイル (*.key);;すべてのファイル (*.*)"));
    if (keyPath.isEmpty()) {
        return;
    }

    QFile certFile(certPath);
    QFile keyFile(keyPath);
    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを開けませんでした。"));
        return;
    }

    m_certDer = certFile.readAll();
    m_keyDer = keyFile.readAll();
    m_certChanged = true;
    QMessageBox::information(this, tr("情報"), tr("証明書を読み込みました。OK を押すと VPN Server に反映されます。"));
}

void ServerSettingsDialog::onExportCert()
{
    if (m_certDer.isEmpty()) {
        return;
    }
    const QString certPath = QFileDialog::getSaveFileName(this, tr("証明書の保存先"), QStringLiteral("server.cer"),
                                                            tr("証明書ファイル (*.cer)"));
    if (certPath.isEmpty()) {
        return;
    }
    const QString keyPath = QFileDialog::getSaveFileName(this, tr("秘密鍵の保存先"), QStringLiteral("server.key"),
                                                           tr("秘密鍵ファイル (*.key)"));
    if (keyPath.isEmpty()) {
        return;
    }

    QFile certFile(certPath);
    QFile keyFile(keyPath);
    if (!certFile.open(QIODevice::WriteOnly) || !keyFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを保存できませんでした。"));
        return;
    }
    certFile.write(m_certDer);
    keyFile.write(m_keyDer);
}

void ServerSettingsDialog::onViewCert()
{
    if (m_certDer.isEmpty()) {
        return;
    }
    CertInfoDialog dialog(m_certDer, this);
    dialog.exec();
}

void ServerSettingsDialog::onRegenerateCert()
{
    bool ok = false;
    const QString cn = QInputDialog::getText(this, tr("新しい証明書の作成"), tr("コモンネーム (CN):"), QLineEdit::Normal,
                                              QString(), &ok);
    if (!ok || cn.trimmed().isEmpty()) {
        return;
    }

    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("現在のサーバー証明書を破棄し、CN \"%1\" の新しい自己署名証明書を作成します。よろしいですか?")
                                .arg(cn),
                            QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    m_rpc->regenerateServerCert(
        cn, [this](const QJsonObject &) { reload(); },
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("証明書の新規作成に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });
}

void ServerSettingsDialog::onOk()
{
    m_rpc->setServerCipher(
        m_cipherCombo->currentText().trimmed(), [](const QJsonObject &) {},
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("暗号化アルゴリズムの設定に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });

    if (m_certChanged) {
        m_rpc->setServerCert(
            QString::fromUtf8(m_certDer.toBase64()), QString::fromUtf8(m_keyDer.toBase64()), [](const QJsonObject &) {},
            [this](const RpcError &error) {
                QMessageBox::warning(this, tr("エラー"),
                                      tr("サーバー証明書の設定に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
            });
    }

    m_rpc->setSysLog(
        m_syslogCombo->currentData().toInt(), m_syslogHostEdit->text().trimmed(),
        static_cast<quint16>(m_syslogPortSpin->value()), [](const QJsonObject &) {},
        [this](const RpcError &error) {
            QMessageBox::warning(this, tr("エラー"),
                                  tr("syslog 設定の変更に失敗しました: %1 (code %2)").arg(error.message).arg(error.code));
        });

    QJsonObject keepParams;
    keepParams["UseKeepConnect_bool"] = m_keepCheck->isChecked();
    keepParams["KeepConnectHost_str"] = m_keepHostEdit->text().trimmed();
    keepParams["KeepConnectPort_u32"] = m_keepPortSpin->value();
    keepParams["KeepConnectProtocol_u32"] = m_keepUdpRadio->isChecked() ? 1 : 0;
    keepParams["KeepConnectInterval_u32"] = m_keepIntervalSpin->value();
    m_rpc->call(
        QStringLiteral("SetKeep"), keepParams, [](const QJsonObject &) {},
        [this](const RpcError &error) { RpcUi::showError(this, tr("インターネット接続維持機能の設定変更"), error); });

    accept();
}

void ServerSettingsDialog::onChangePassword()
{
    AdminPasswordDialog dialog(m_rpc, this);
    dialog.exec();
}

void ServerSettingsDialog::onSpecialListener()
{
    SpecialListenerDialog dialog(m_rpc, this);
    dialog.exec();
}
