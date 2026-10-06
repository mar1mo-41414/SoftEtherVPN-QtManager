#include "ConfigEditDialog.h"

#include "util/RpcUiHelpers.h"

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ConfigEditDialog::ConfigEditDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_CONFIG CAPTION / IDC_INFO
    setWindowTitle(tr("Config ファイルの編集"));

    auto *infoLabel = new QLabel(
        tr("VPN Server \"%1\" の現在のコンフィグレーションファイルは下記のとおりです。\n"
           "このコンフィグレーションファイルの内容を編集して、VPN Server に書き込むこともできます。")
            .arg(serverName),
        this);
    infoLabel->setWordWrap(true);

    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(false); // 公式Managerと同様に編集可能 (反映は「ファイルに保存」→「ファイルからインポートして書き込み」)
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont mono(QStringLiteral("Menlo"));
    mono.setStyleHint(QFont::Monospace);
    m_text->setFont(mono);

    auto *noteLabel = new QLabel(
        tr("コンフィグレーションファイルは通常のテキストエディタ等で編集可能です。編集したコンフィグレーションファイルを "
           "VPN Server に書き込んだ場合、VPN Server は自動的に再起動し、新しいコンフィグレーションファイルに従って起動します。"
           "不正なコンフィグレーションファイルを書き込んだ場合はエラーが発生したり現在の設定内容が失われたりする可能性が"
           "ありますので、十分注意してください。"),
        this);
    noteLabel->setWordWrap(true);

    // B_EXPORT / B_IMPORT / B_FACTORY / IDCANCEL
    auto *exportButton = new QPushButton(tr("ファイルに保存(&S)"), this);
    auto *importButton = new QPushButton(tr("ファイルからインポートして書き込み(&I)"), this);
    auto *factoryButton = new QPushButton(tr("設定をリセットして初期化(&R)"), this);
    factoryButton->setEnabled(false);
    factoryButton->setToolTip(tr("未対応"));
    auto *closeButton = new QPushButton(tr("閉じる(&C)"), this);
    connect(exportButton, &QPushButton::clicked, this, &ConfigEditDialog::onExport);
    connect(importButton, &QPushButton::clicked, this, &ConfigEditDialog::onImport);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(importButton);
    buttonLayout->addWidget(factoryButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(infoLabel);
    layout->addWidget(m_text, 1);
    layout->addWidget(noteLabel);
    layout->addLayout(buttonLayout);

    resize(760, 620);

    m_rpc->call(
        QStringLiteral("GetConfig"), {},
        [this](const QJsonObject &result) {
            m_fileName = result.value("FileName_str").toString();
            m_text->setPlainText(QString::fromUtf8(QByteArray::fromBase64(result.value("FileData_bin").toString().toUtf8())));
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("コンフィグレーションファイルの取得"), error); });
}

void ConfigEditDialog::onExport()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("コンフィグレーションファイルの保存先"),
                                                        m_fileName.isEmpty() ? QStringLiteral("vpn_server.config") : m_fileName);
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    // SM_CONFIG_SAVED / SM_CONFIG_SAVE_FAILED
    if (file.open(QIODevice::WriteOnly) && file.write(m_text->toPlainText().toUtf8()) >= 0) {
        QMessageBox::information(this, tr("完了"), tr("コンフィグレーションファイルを保存しました。"));
    } else {
        QMessageBox::warning(this, tr("エラー"), tr("コンフィグレーションファイルの保存に失敗しました。"));
    }
}

void ConfigEditDialog::onImport()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("書き込むコンフィグレーションファイルを選択"), QString(),
                                                        tr("コンフィグレーションファイル (*.config);;すべてのファイル (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // SM_CONFIG_OPEN_FAILED
        QMessageBox::warning(this, tr("エラー"), tr("指定したファイルを開けませんでした。"));
        return;
    }
    const QByteArray data = file.readAll();

    // SM_CONFIG_CONFIRM
    QMessageBox confirmBox(
        QMessageBox::Warning, tr("確認"),
        tr("指定されたコンフィグレーションファイルを VPN Server に書き込みます。VPN Server は自動的に再起動し、新しい"
           "コンフィグレーションファイルの内容で起動します。現在 VPN Server に接続中のユーザーは一旦切断されます。"
           "この管理セッションも切断されますので、再度サーバーに接続し直してください。\n\n続行しますか?"),
        QMessageBox::NoButton, this);
    QPushButton *yesButton = confirmBox.addButton(tr("はい"), QMessageBox::YesRole);
    confirmBox.addButton(tr("いいえ"), QMessageBox::NoRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != yesButton) {
        return;
    }

    QJsonObject params;
    params["FileData_bin"] = QString::fromLatin1(data.toBase64());
    m_rpc->call(
        QStringLiteral("SetConfig"), params,
        [this](const QJsonObject &) {
            // SM_CONFIG_WRITE_OK
            QMessageBox::information(this, tr("完了"), tr("サーバー側のコンフィグレーションファイルを書き換えました。"));
            accept();
        },
        [this](const RpcError &error) { RpcUi::showError(this, tr("コンフィグレーションファイルの書き込み"), error); });
}
