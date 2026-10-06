#include "HubEditDialog.h"
#include "HubAccessControlDialog.h"
#include "HubMessageDialog.h"
#include "HubOptionsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
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

} // namespace

HubEditDialog::HubEditDialog(VpnServerRpc *rpc, bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_isNew(isNew)
{
    // SM_EDIT_HUB_CAPTION 相当 (編集時は setHub で "<名前> のプロパティ" に更新)
    setWindowTitle(isNew ? tr("仮想 HUB の新規作成") : tr("仮想 HUB のプロパティ"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);
    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(new QLabel(tr("仮想 HUB 名(N):"), this));
    m_nameEdit->setMaximumWidth(280);
    nameRow->addWidget(m_nameEdit);
    nameRow->addStretch();

    // セキュリティ設定
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit = new QLineEdit(this);
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);
    m_noEnumCheck = new QCheckBox(tr("匿名ユーザーに対してこの仮想 HUB を列挙しない(&U)"), this);
    auto *passwordForm = new QFormLayout;
    passwordForm->setLabelAlignment(Qt::AlignRight);
    passwordForm->addRow(tr("パスワード(P):"), m_passwordEdit);
    passwordForm->addRow(tr("確認入力(C):"), m_passwordConfirmEdit);
    auto *securityGroup = new QGroupBox(tr("セキュリティ設定(S):"), this);
    auto *securityLayout = new QVBoxLayout(securityGroup);
    auto *passwordCaption = new QLabel(tr("この仮想 HUB の管理用パスワード"), this);
    QFont bold = passwordCaption->font();
    bold.setBold(true);
    passwordCaption->setFont(bold);
    securityLayout->addWidget(passwordCaption);
    securityLayout->addLayout(passwordForm);
    securityLayout->addWidget(m_noEnumCheck);

    // 仮想 HUB の状態
    m_onlineRadio = new QRadioButton(tr("オンライン(E)"), this);
    m_offlineRadio = new QRadioButton(tr("オフライン(F)"), this);
    m_onlineRadio->setChecked(true);
    auto *stateGroup = new QGroupBox(tr("仮想 HUB の状態(J):"), this);
    auto *stateLayout = new QVBoxLayout(stateGroup);
    stateLayout->addWidget(new QLabel(tr("仮想 HUB の状態を選択してください。"), this));
    auto *stateRadios = new QHBoxLayout;
    stateRadios->addWidget(m_onlineRadio);
    stateRadios->addWidget(m_offlineRadio);
    stateRadios->addStretch();
    stateLayout->addLayout(stateRadios);

    // 仮想 HUB オプション
    m_limitMaxSessionCheck = new QCheckBox(tr("最大同時接続セッション数を制限する(&L)"), this);
    m_maxSessionSpin = new QSpinBox(this);
    m_maxSessionSpin->setRange(1, 999999);
    m_maxSessionSpin->setValue(255);
    auto *maxRow = new QHBoxLayout;
    maxRow->addWidget(new QLabel(tr("最大同時接続セッション数(Z):"), this));
    maxRow->addWidget(m_maxSessionSpin);
    maxRow->addWidget(new QLabel(tr("セッション"), this));
    maxRow->addStretch();
    auto *optionGroup = new QGroupBox(tr("仮想 HUB オプション(I):"), this);
    auto *optionLayout = new QVBoxLayout(optionGroup);
    optionLayout->addWidget(m_limitMaxSessionCheck);
    optionLayout->addLayout(maxRow);
    optionLayout->addWidget(note(tr("(ローカルブリッジ、仮想 NAT、カスケード接続などによって生成されるサーバー側の仮想セッション数は含まない)"), this));
    auto *extButton = new QPushButton(tr("仮想 HUB 拡張オプションの編集(&X)"), this);
    extButton->setEnabled(!isNew);
    if (!isNew) {
        optionLayout->addWidget(note(tr("仮想 HUB 拡張オプションを使用すると、この仮想 HUB に関するより詳細な設定を行うことができるようになります。"), this));
        optionLayout->addWidget(extButton);
    }

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addWidget(securityGroup);
    leftColumn->addWidget(stateGroup);
    leftColumn->addWidget(optionGroup);
    leftColumn->addStretch();

    // クラスタリング設定
    m_staticRadio = new QRadioButton(tr("スタティック仮想 HUB(&A)"), this);
    m_dynamicRadio = new QRadioButton(tr("ダイナミック仮想 HUB(&D)"), this);
    m_staticRadio->setEnabled(false);
    m_dynamicRadio->setEnabled(false);
    auto *clusterGroup = new QGroupBox(tr("クラスタリング設定(M):"), this);
    auto *clusterLayout = new QVBoxLayout(clusterGroup);
    clusterLayout->addWidget(note(tr("現在、サーバーはスタンドアロンモードで動作しています。この仮想 HUB はスタンドアロン HUB として動作します。"), this));
    auto *clusterRadios = new QHBoxLayout;
    clusterRadios->addWidget(m_staticRadio);
    clusterRadios->addWidget(m_dynamicRadio);
    clusterRadios->addStretch();
    clusterLayout->addLayout(clusterRadios);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(clusterGroup);
    if (!isNew) {
        auto *adminButton = new QPushButton(tr("仮想 HUB 管理オプション(&K)"), this);
        auto *adminGroup = new QGroupBox(tr("仮想 HUB 管理オプション(Y):"), this);
        auto *adminLayout = new QVBoxLayout(adminGroup);
        adminLayout->addWidget(note(tr("仮想 HUB の管理オプションを表示および編集できます。"), this));
        adminLayout->addWidget(adminButton, 0, Qt::AlignHCenter);

        auto *aclButton = new QPushButton(tr("接続元 IP 制限リスト(&T)"), this);
        auto *aclGroup = new QGroupBox(tr("接続元 IP 制限リスト(R):"), this);
        auto *aclLayout = new QVBoxLayout(aclGroup);
        aclLayout->addWidget(note(tr("クライアントコンピュータの IP アドレスによって、この仮想 HUB への VPN 接続を許可または拒否することができます。"), this));
        aclLayout->addWidget(aclButton, 0, Qt::AlignHCenter);

        auto *messageButton = new QPushButton(tr("メッセージの設定(&G)"), this);
        auto *messageGroup = new QGroupBox(tr("接続時にメッセージを表示"), this);
        auto *messageLayout = new QVBoxLayout(messageGroup);
        messageLayout->addWidget(note(tr("この仮想 HUB に VPN Client が接続した際に、ユーザーの画面にメッセージを表示できます。"), this));
        messageLayout->addWidget(messageButton, 0, Qt::AlignHCenter);

        rightColumn->addWidget(adminGroup);
        rightColumn->addWidget(aclGroup);
        rightColumn->addWidget(messageGroup);
        connect(adminButton, &QPushButton::clicked, this, &HubEditDialog::onAdminOptions);
        connect(aclButton, &QPushButton::clicked, this, &HubEditDialog::onAccessControl);
        connect(messageButton, &QPushButton::clicked, this, &HubEditDialog::onMessage);
        connect(extButton, &QPushButton::clicked, this, &HubEditDialog::onExtOptions);
    }
    rightColumn->addStretch();

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &HubEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rightColumn->addWidget(buttonBox);

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(nameRow);
    layout->addLayout(columns, 1);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &HubEditDialog::updateState);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &HubEditDialog::updateState);
    connect(m_passwordConfirmEdit, &QLineEdit::textChanged, this, &HubEditDialog::updateState);
    connect(m_limitMaxSessionCheck, &QCheckBox::toggled, this, &HubEditDialog::updateState);

    resize(isNew ? 800 : 800, isNew ? 520 : 600);
    updateState();
}

void HubEditDialog::updateState()
{
    m_maxSessionSpin->setEnabled(m_limitMaxSessionCheck->isChecked());
    m_okButton->setEnabled(!m_nameEdit->text().trimmed().isEmpty() &&
                           m_passwordEdit->text() == m_passwordConfirmEdit->text());
}

void HubEditDialog::setHub(const QJsonObject &hub)
{
    const QString name = hub.value("HubName_str").toString();
    // SM_EDIT_HUB_CAPTION_2 相当
    setWindowTitle(tr("%1 のプロパティ").arg(name));
    m_nameEdit->setText(name);
    m_noEnumCheck->setChecked(hub.value("NoEnum_bool").toBool());
    (hub.value("Online_bool").toBool() ? m_onlineRadio : m_offlineRadio)->setChecked(true);
    const quint32 maxSession = static_cast<quint32>(hub.value("MaxSession_u32").toDouble());
    m_limitMaxSessionCheck->setChecked(maxSession > 0);
    if (maxSession > 0) {
        m_maxSessionSpin->setValue(static_cast<int>(maxSession));
    }
    m_hubType = hub.value("HubType_u32").toInt();
    (m_hubType == 2 ? m_dynamicRadio : m_staticRadio)->setChecked(m_hubType != 0);
    // 公式Managerと同様、既存パスワードは伏せ字で表示し、触らなければ変更しない。
    m_passwordEdit->setText(kHiddenPassword);
    m_passwordConfirmEdit->setText(kHiddenPassword);
    updateState();
}

QJsonObject HubEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["HubName_str"] = m_nameEdit->text().trimmed();
    params["Online_bool"] = m_onlineRadio->isChecked();
    params["NoEnum_bool"] = m_noEnumCheck->isChecked();
    params["MaxSession_u32"] = m_limitMaxSessionCheck->isChecked() ? m_maxSessionSpin->value() : 0;
    params["HubType_u32"] = m_hubType;
    const bool passwordUntouched =
        !m_isNew && m_passwordEdit->text() == kHiddenPassword && m_passwordConfirmEdit->text() == kHiddenPassword;
    if (!passwordUntouched && !m_passwordEdit->text().isEmpty()) {
        params["AdminPasswordPlainText_str"] = m_passwordEdit->text();
    }
    return params;
}

QString HubEditDialog::hubName() const
{
    return m_nameEdit->text().trimmed();
}

void HubEditDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("仮想 HUB 名を入力してください。"));
        return;
    }
    QDialog::accept();
}

void HubEditDialog::onAdminOptions()
{
    HubOptionsDialog dialog(m_rpc, hubName(), HubOptionsDialog::Kind::Admin, this);
    dialog.exec();
}

void HubEditDialog::onExtOptions()
{
    HubOptionsDialog dialog(m_rpc, hubName(), HubOptionsDialog::Kind::Extended, this);
    dialog.exec();
}

void HubEditDialog::onAccessControl()
{
    HubAccessControlDialog dialog(m_rpc, hubName(), this);
    dialog.exec();
}

void HubEditDialog::onMessage()
{
    HubMessageDialog dialog(m_rpc, hubName(), this);
    dialog.exec();
}
