#include "FarmDialog.h"

#include "util/RpcUiHelpers.h"

#include "util/DialogSizing.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QLabel *note(QWidget *parent, const QString &text)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}
} // namespace

FarmDialog::FarmDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
{
    // D_SM_FARM CAPTION
    setWindowTitle(tr("クラスタリング構成"));

    // S_TITLE / STATIC1
    auto *titleLabel = new QLabel(tr("VPN Server \"%1\" のクラスタリング構成を変更できます。").arg(serverName), this);
    titleLabel->setWordWrap(true);
    auto *introLabel = new QLabel(
        tr("複数台の VPN Server でクラスタを構成すると、ロードバランシング (負荷分散) およびフォールトトレランスの確保を"
           "実現することができます。"),
        this);
    introLabel->setWordWrap(true);

    // STATIC2 / S_CURRENT
    m_currentModeLabel = new QLabel(this);
    QFont currentModeFont = m_currentModeLabel->font();
    currentModeFont.setBold(true);
    m_currentModeLabel->setFont(currentModeFont);
    auto *currentModeRow = new QHBoxLayout;
    currentModeRow->addWidget(new QLabel(tr("現在の動作モード:"), this));
    currentModeRow->addWidget(m_currentModeLabel, 1);

    // R_STANDALONE / R_CONTROLLER / R_MEMBER
    m_standaloneRadio = new QRadioButton(tr("スタンドアロンサーバー (クラスタリング構成無し)(&S)"), this);
    m_controllerRadio = new QRadioButton(tr("クラスタコントローラ(&C)"), this);
    m_memberRadio = new QRadioButton(tr("クラスタメンバサーバー(&M)"), this);
    m_standaloneRadio->setChecked(true);
    connect(m_standaloneRadio, &QRadioButton::toggled, this, &FarmDialog::onModeChanged);
    connect(m_controllerRadio, &QRadioButton::toggled, this, &FarmDialog::onModeChanged);
    connect(m_memberRadio, &QRadioButton::toggled, this, &FarmDialog::onModeChanged);

    auto *modeGroup = new QGroupBox(tr("クラスタリング構成の設定(T):"), this);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    modeLayout->addWidget(m_standaloneRadio);
    modeLayout->addWidget(m_controllerRadio);
    modeLayout->addWidget(m_memberRadio);

    // 共通: S_1 / S_2 / R_CONTROLLER_ONLY
    m_weightSpin = new QSpinBox(this);
    m_weightSpin->setRange(1, 10000);
    m_weightSpin->setValue(100);
    m_controllerOnlyCheck = new QCheckBox(tr("コントローラ機能のみ (自身は VPN 通信を処理しない)"), this);
    auto *weightRow = new QHBoxLayout;
    auto *weightLabel = new QLabel(tr("クラスタ内での性能基準比(W):"), this);

    // S_IP_1 / S_PORT_1 / S_CONTROLLER / S_CONTROLLER_PORT / S_PASSWORD
    m_publicIpEdit = new QLineEdit(this);
    m_portsEdit = new QLineEdit(this);
    m_controllerHostEdit = new QLineEdit(this);
    m_controllerPortSpin = new QSpinBox(this);
    m_controllerPortSpin->setRange(1, 65535);
    m_controllerPortSpin->setValue(443);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    weightRow->addWidget(weightLabel);
    weightRow->addWidget(m_weightSpin);
    weightRow->addWidget(new QLabel(tr("(標準: 100)"), this));
    weightRow->addStretch();
    weightRow->addWidget(m_controllerOnlyCheck);

    // S_IP_1/2 / S_PORT_1/2/3 / S_CONTROLLER / S_CONTROLLER_PORT / S_PASSWORD
    auto *form = new QFormLayout;
    form->addRow(tr("公開 IP アドレス(I):"), m_publicIpEdit);
    form->addRow(QString(),
                 note(this,
                      tr("(公開 IP アドレスを入力しない場合は、クラスタコントローラへの接続の際に使用される"
                         "ネットワークインターフェイスの IP アドレスが自動的に使用されます。)")));
    form->addRow(tr("公開ポート一覧(P):"), m_portsEdit);
    form->addRow(QString(), note(this, tr("(複数入力する場合はスペースまたはカンマで区切ってください。)")));
    form->addRow(tr("コントローラのホスト名または IP アドレス(H):"), m_controllerHostEdit);
    form->addRow(tr("コントローラの\nポート番号(R):"), m_controllerPortSpin);
    form->addRow(tr("管理パスワード(D):"), m_passwordEdit);
    auto *optionGroup = new QGroupBox(tr("クラスタメンバサーバー時の設定項目(E):"), this);
    optionGroup->setLayout(form);

    auto *warnLabel = new QLabel(
        tr("クラスタリング構成を変更すると、VPN Server サービスが自動的に再起動します。その際、現在接続されているすべての"
           "セッションおよび管理用コネクションが切断されます。"),
        this);
    warnLabel->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FarmDialog::onOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(introLabel);
    layout->addLayout(currentModeRow);
    layout->addWidget(modeGroup);
    layout->addLayout(weightRow);
    layout->addWidget(optionGroup);
    layout->addWidget(warnLabel);
    layout->addWidget(buttonBox);

    DialogSizing::fitToWidth(this, 560);
    onModeChanged();

    m_rpc->call(
        QStringLiteral("GetFarmSetting"), {},
        RpcUi::guarded(this, [this](const QJsonObject &result) {
            const int type = result.value("ServerType_u32").toInt();
            m_controllerRadio->setChecked(type == 1);
            m_memberRadio->setChecked(type == 2);
            m_standaloneRadio->setChecked(type != 1 && type != 2);
            // SM_SERVER_STANDALONE / SM_FARM_CONTROLLER / SM_FARM_MEMBER
            m_currentModeLabel->setText(type == 1 ? tr("クラスタコントローラ")
                                        : type == 2 ? tr("クラスタメンバサーバー")
                                                     : tr("スタンドアロンサーバー"));

            m_publicIpEdit->setText(result.value("PublicIp_ip").toString());
            QStringList ports;
            for (const QJsonValue &port : result.value("Ports_u32").toArray()) {
                ports << QString::number(port.toInt());
            }
            m_portsEdit->setText(ports.join(QLatin1Char(' ')));
            m_controllerHostEdit->setText(result.value("ControllerName_str").toString());
            const int controllerPort = result.value("ControllerPort_u32").toInt();
            m_controllerPortSpin->setValue(controllerPort > 0 ? controllerPort : 443);
            m_passwordEdit->setText(result.value("MemberPasswordPlaintext_str").toString());
            const int weight = result.value("Weight_u32").toInt();
            m_weightSpin->setValue(weight > 0 ? weight : 100);
            m_controllerOnlyCheck->setChecked(result.value("ControllerOnly_bool").toBool());
        }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("クラスタリング構成の取得"), error); }));
}

void FarmDialog::onModeChanged()
{
    const bool isController = m_controllerRadio->isChecked();
    const bool isMember = m_memberRadio->isChecked();
    m_weightSpin->setEnabled(isController || isMember);
    m_controllerOnlyCheck->setEnabled(isController);
    m_publicIpEdit->setEnabled(isMember);
    m_portsEdit->setEnabled(isMember);
    m_controllerHostEdit->setEnabled(isMember);
    m_controllerPortSpin->setEnabled(isMember);
    m_passwordEdit->setEnabled(isMember);
}

void FarmDialog::onOk()
{
    const bool isMember = m_memberRadio->isChecked();
    QJsonArray ports;
    if (isMember) {
        const QStringList parts = m_portsEdit->text().split(QRegularExpression(QStringLiteral("[\\s,]+")), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            bool ok = false;
            const int port = part.toInt(&ok);
            if (!ok || port < 1 || port > 65535) {
                QMessageBox::warning(this, tr("入力エラー"), tr("公開ポート一覧には 1〜65535 の数値を指定してください。"));
                return;
            }
            ports.append(port);
        }
        if (m_controllerHostEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("入力エラー"), tr("コントローラのホスト名または IP アドレスを入力してください。"));
            return;
        }
    }

    // SM_FARM_REBOOT_MSG
    QMessageBox confirmBox(QMessageBox::Warning, tr("確認"),
                            tr("クラスタリング構成を変更しようとしています。\n\n"
                               "クラスタリング構成を変更すると、現在接続されているすべてのセッションおよび管理用コネクション "
                               "(この管理用コネクションを含む) がすべて切断され、サーバー プログラムが再起動します。"
                               "サーバーのユーザー数が多い場合は、再起動に数十秒かかる場合もあります。\n\n"
                               "続行すると、サーバーとの接続が切断されます。管理を継続するには、もう一度サーバーに接続し直してください。"),
                            QMessageBox::NoButton, this);
    QPushButton *okButton = confirmBox.addButton(tr("OK"), QMessageBox::AcceptRole);
    confirmBox.addButton(tr("キャンセル"), QMessageBox::RejectRole);
    confirmBox.exec();
    if (confirmBox.clickedButton() != okButton) {
        return;
    }

    QJsonObject params;
    params["ServerType_u32"] = m_controllerRadio->isChecked() ? 1 : (isMember ? 2 : 0);
    params["NumPort_u32"] = ports.size();
    params["Ports_u32"] = ports;
    params["PublicIp_ip"] = isMember ? m_publicIpEdit->text().trimmed() : QString();
    params["ControllerName_str"] = isMember ? m_controllerHostEdit->text().trimmed() : QString();
    params["ControllerPort_u32"] = m_controllerPortSpin->value();
    params["MemberPasswordPlaintext_str"] = isMember ? m_passwordEdit->text() : QString();
    params["Weight_u32"] = m_weightSpin->value();
    params["ControllerOnly_bool"] = m_controllerRadio->isChecked() && m_controllerOnlyCheck->isChecked();

    m_rpc->call(
        QStringLiteral("SetFarmSetting"), params, RpcUi::guarded(this, [this](const QJsonObject &) { accept(); }),
        RpcUi::guarded(this, [this](const RpcError &error) { RpcUi::showError(this, tr("クラスタリング構成の変更"), error); }));
}
