#include "UserEditDialog.h"
#include "CertInfoDialog.h"
#include "GroupListDialog.h"
#include "PolicyDialog.h"

#include "util/RpcUiHelpers.h"

#include <QCheckBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace {

// 公式Managerがパスワードを変更しないときに表示する伏せ字。
const QString kHiddenPassword = QStringLiteral("********");

QFrame *separator(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

QLabel *description(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    return label;
}

} // namespace

UserEditDialog::UserEditDialog(VpnServerRpc *rpc, const QString &hubName, bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_rpc(rpc)
    , m_hubName(hubName)
    , m_isNew(isNew)
    , m_policy(PolicyDialog::defaultPolicy())
{
    // SM_EDIT_USER_CAPTION_1 / _2 (編集時は setUser でユーザー名入りに更新)
    setWindowTitle(isNew ? tr("ユーザーの新規作成") : tr("ユーザーのプロパティ"));

    // --- 左列 上: 基本情報 ---
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setEnabled(isNew);
    m_realnameEdit = new QLineEdit(this);
    m_noteEdit = new QLineEdit(this);
    auto *basicForm = new QFormLayout;
    basicForm->setLabelAlignment(Qt::AlignRight);
    basicForm->addRow(tr("ユーザー名(U):"), m_nameEdit);
    basicForm->addRow(tr("本名(R):"), m_realnameEdit);
    basicForm->addRow(tr("説明(N):"), m_noteEdit);

    // --- 左列 中: グループ・有効期限 ---
    m_groupEdit = new QLineEdit(this);
    auto *groupButton = new QPushButton(tr("グループの参照(&J)..."), this);
    auto *groupRow = new QHBoxLayout;
    groupRow->addWidget(new QLabel(tr("グループ名\n(省略可能):"), this));
    groupRow->addWidget(m_groupEdit, 1);
    groupRow->addWidget(groupButton);

    m_expireCheck = new QCheckBox(tr("このアカウントの有効期限を設定する(&S)"), this);
    m_expireDate = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_expireDate->setCalendarPopup(true);
    m_expireDate->setDisplayFormat(QStringLiteral("yyyy年M月d日"));
    m_expireTime = new QTimeEdit(QTime(0, 0, 0), this);
    m_expireTime->setDisplayFormat(QStringLiteral("H:mm:ss"));
    auto *expireRow = new QHBoxLayout;
    expireRow->addSpacing(20);
    expireRow->addWidget(m_expireDate);
    expireRow->addWidget(m_expireTime);
    expireRow->addStretch();

    // --- 左列 下: 認証方法 ---
    m_authList = new QListWidget(this);
    // SM_AUTHTYPE_0〜5
    for (const QString &name : {tr("匿名認証"), tr("パスワード認証"), tr("固有証明書認証"), tr("署名済み証明書認証"),
                                 tr("RADIUS 認証"), tr("NT ドメイン認証")}) {
        m_authList->addItem(name);
    }
    m_authList->setCurrentRow(1);
    m_authList->setFixedHeight(m_authList->sizeHintForRow(0) * 6 + 6);
    auto *authForm = new QFormLayout;
    authForm->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    authForm->addRow(tr("認証方法(A):"), m_authList);

    // RADIUS または NT ドメイン認証
    m_radiusGroup = new QGroupBox(tr("RADIUS または NT ドメイン認証"), this);
    m_radiusNameCheck = new QCheckBox(tr("認証サーバー上のユーザー名を指定する(&K)"), this);
    m_radiusNameEdit = new QLineEdit(this);
    auto *radiusForm = new QFormLayout;
    radiusForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
    radiusForm->addRow(tr("認証サーバーにおけるユーザー名(T):"), m_radiusNameEdit);
    auto *radiusLayout = new QVBoxLayout(m_radiusGroup);
    radiusLayout->addWidget(description(
        tr("外部の RADIUS サーバー、Windows NT ドメインコントローラ、または Active Directory コントローラによってユーザーが入力したパスワードが検証されます。"),
        this));
    radiusLayout->addWidget(m_radiusNameCheck);
    radiusLayout->addLayout(radiusForm);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->addLayout(basicForm);
    leftColumn->addWidget(separator(this));
    leftColumn->addLayout(groupRow);
    leftColumn->addWidget(m_expireCheck);
    leftColumn->addLayout(expireRow);
    leftColumn->addWidget(separator(this));
    leftColumn->addLayout(authForm);
    leftColumn->addWidget(m_radiusGroup);
    leftColumn->addStretch();

    // --- 右列 1: セキュリティポリシー ---
    auto *policyGroup = new QGroupBox(tr("セキュリティポリシー"), this);
    m_policyCheck = new QCheckBox(tr("このユーザーのセキュリティポリシーを設定する(&Y)"), this);
    m_policyButton = new QPushButton(tr("セキュリティポリシー(&M)"), this);
    auto *policyLayout = new QHBoxLayout(policyGroup);
    policyLayout->addWidget(m_policyCheck, 1);
    policyLayout->addWidget(m_policyButton);

    // --- 右列 2: パスワード認証 ---
    m_passwordGroup = new QGroupBox(tr("パスワード認証"), this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit = new QLineEdit(this);
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);
    auto *passwordForm = new QFormLayout(m_passwordGroup);
    passwordForm->setLabelAlignment(Qt::AlignRight);
    passwordForm->addRow(tr("パスワード(P):"), m_passwordEdit);
    passwordForm->addRow(tr("パスワードの確認入力(C):"), m_passwordConfirmEdit);

    // --- 右列 3: 固有証明書認証 ---
    m_userCertGroup = new QGroupBox(tr("固有証明書認証"), this);
    auto *loadCertButton = new QPushButton(tr("証明書の指定(&E)"), this);
    m_viewCertButton = new QPushButton(tr("証明書の表示(&V)"), this);
    auto *makeCertButton = new QPushButton(tr("証明書作成ツール(&W)"), this);
    makeCertButton->setEnabled(false);
    makeCertButton->setToolTip(tr("未実装"));
    auto *certButtons = new QHBoxLayout;
    certButtons->addWidget(loadCertButton);
    certButtons->addWidget(m_viewCertButton);
    certButtons->addWidget(makeCertButton);
    auto *userCertLayout = new QVBoxLayout(m_userCertGroup);
    // SM_EDIT_USER_CERT_INFO
    userCertLayout->addWidget(description(
        tr("[固有証明書認証] が選択されているユーザーは、接続時に SSL クライアント証明書が予めユーザーごとに設定された証明書と完全に一致するかどうかで接続を許可または拒否されます。"),
        this));
    userCertLayout->addLayout(certButtons);

    // --- 右列 4: 署名済み証明書認証 ---
    m_rootCertGroup = new QGroupBox(tr("署名済み証明書認証"), this);
    m_cnCheck = new QCheckBox(tr("証明書の Common Name (CN) の値を限定する(&B)"), this);
    m_cnEdit = new QLineEdit(this);
    m_serialCheck = new QCheckBox(tr("証明書のシリアル番号の値を限定する(&L)"), this);
    m_serialEdit = new QLineEdit(this);
    auto *rootLayout = new QVBoxLayout(m_rootCertGroup);
    rootLayout->addWidget(
        description(tr("クライアント証明書がこの仮想 HUB の信頼する証明機関の証明書によって署名されているかどうかを検証します。"), this));
    rootLayout->addWidget(m_cnCheck);
    rootLayout->addWidget(m_cnEdit);
    rootLayout->addWidget(m_serialCheck);
    rootLayout->addWidget(m_serialEdit);
    rootLayout->addWidget(new QLabel(tr("※ 16 進数で入力してください。(例: 0155ABCDEF)"), this), 0, Qt::AlignRight);

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(policyGroup);
    rightColumn->addWidget(m_passwordGroup);
    rightColumn->addWidget(m_userCertGroup);
    rightColumn->addWidget(m_rootCertGroup);
    rightColumn->addStretch();

    auto *columns = new QHBoxLayout;
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);

    // --- 下段: ヒント + OK/キャンセル ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("OK"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("キャンセル"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &UserEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *bottom = new QHBoxLayout;
    // S_HINT (新規作成時のみ表示)
    auto *hint = description(
        tr("ヒント: ユーザー名が '*' (アスタリスク) のユーザーを作成すると、他に明示的に一致するユーザー名の定義がないユーザーが接続しようとした場合に外部認証サーバーを使用したパスワード認証による接続を許可できます。"),
        this);
    hint->setVisible(isNew);
    bottom->addWidget(hint, 1);
    bottom->addWidget(buttonBox, 0, Qt::AlignBottom);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(columns, 1);
    layout->addLayout(bottom);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(m_authList, &QListWidget::currentRowChanged, this, &UserEditDialog::updateState);
    connect(m_expireCheck, &QCheckBox::toggled, this, &UserEditDialog::updateState);
    connect(m_policyCheck, &QCheckBox::toggled, this, &UserEditDialog::updateState);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(m_passwordConfirmEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(m_cnCheck, &QCheckBox::toggled, this, &UserEditDialog::updateState);
    connect(m_cnEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(m_serialCheck, &QCheckBox::toggled, this, &UserEditDialog::updateState);
    connect(m_serialEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(m_radiusNameCheck, &QCheckBox::toggled, this, &UserEditDialog::updateState);
    connect(m_radiusNameEdit, &QLineEdit::textChanged, this, &UserEditDialog::updateState);
    connect(groupButton, &QPushButton::clicked, this, &UserEditDialog::onSelectGroup);
    connect(m_policyButton, &QPushButton::clicked, this, &UserEditDialog::onPolicy);
    connect(loadCertButton, &QPushButton::clicked, this, &UserEditDialog::onLoadCert);
    connect(m_viewCertButton, &QPushButton::clicked, this, &UserEditDialog::onViewCert);

    resize(820, 600);
    updateState();
}

int UserEditDialog::authType() const
{
    return m_authList->currentRow();
}

void UserEditDialog::updateState()
{
    const int type = authType();
    m_expireDate->setEnabled(m_expireCheck->isChecked());
    m_expireTime->setEnabled(m_expireCheck->isChecked());
    m_policyButton->setEnabled(m_policyCheck->isChecked());

    m_passwordGroup->setEnabled(type == 1);
    m_userCertGroup->setEnabled(type == 2);
    m_viewCertButton->setEnabled(type == 2 && !m_certDer.isEmpty());
    m_rootCertGroup->setEnabled(type == 3);
    m_cnEdit->setEnabled(type == 3 && m_cnCheck->isChecked());
    m_serialEdit->setEnabled(type == 3 && m_serialCheck->isChecked());
    m_radiusGroup->setEnabled(type == 4 || type == 5);
    m_radiusNameEdit->setEnabled((type == 4 || type == 5) && m_radiusNameCheck->isChecked());

    static const QRegularExpression hex(QStringLiteral("^[0-9A-Fa-f]*$"));
    bool ok = !m_nameEdit->text().trimmed().isEmpty();
    switch (type) {
    case 1:
        ok = ok && m_passwordEdit->text() == m_passwordConfirmEdit->text();
        break;
    case 2:
        ok = ok && !m_certDer.isEmpty();
        break;
    case 3: {
        const QString serial = m_serialEdit->text().remove(QLatin1Char(' '));
        ok = ok && (!m_cnCheck->isChecked() || !m_cnEdit->text().isEmpty()) &&
             (!m_serialCheck->isChecked() || (!serial.isEmpty() && hex.match(serial).hasMatch()));
        break;
    }
    case 4:
    case 5:
        ok = ok && (!m_radiusNameCheck->isChecked() || !m_radiusNameEdit->text().isEmpty());
        break;
    default:
        break;
    }
    m_okButton->setEnabled(ok);
}

void UserEditDialog::setUser(const QJsonObject &user)
{
    const QString name = user.value("Name_str").toString();
    // SM_EDIT_USER_CAPTION_2
    setWindowTitle(tr("ユーザー %1 のプロパティ").arg(name));
    m_nameEdit->setText(name);
    m_groupEdit->setText(user.value("GroupName_str").toString());
    m_realnameEdit->setText(user.value("Realname_utf").toString());
    m_noteEdit->setText(user.value("Note_utf").toString());

    const QDateTime expire = QDateTime::fromString(user.value("ExpireTime_dt").toString(), Qt::ISODateWithMs);
    if (expire.isValid() && expire.date().year() > 1971) {
        const QDateTime local = expire.toLocalTime();
        m_expireCheck->setChecked(true);
        m_expireDate->setDate(local.date());
        m_expireTime->setTime(local.time());
    }

    m_policy = PolicyDialog::defaultPolicy();
    const QJsonObject policy = PolicyDialog::extract(user);
    for (auto it = policy.begin(); it != policy.end(); ++it) {
        m_policy[it.key()] = it.value();
    }
    m_policyCheck->setChecked(user.value("UsePolicy_bool").toBool());

    const int type = user.value("AuthType_u32").toInt();
    m_authList->setCurrentRow(qBound(0, type, 5));
    switch (type) {
    case 1:
        // 公式Managerと同じく、既存パスワードは伏せ字を表示し、触らなければ変更しない。
        m_originalPassword = user.value("Auth_Password_str").toString();
        m_passwordEdit->setText(kHiddenPassword);
        m_passwordConfirmEdit->setText(kHiddenPassword);
        break;
    case 2:
        m_certDer = QByteArray::fromBase64(user.value("UserX_bin").toString().toLatin1());
        break;
    case 3: {
        const QString cn = user.value("CommonName_utf").toString();
        if (!cn.isEmpty()) {
            m_cnCheck->setChecked(true);
            m_cnEdit->setText(cn);
        }
        const QByteArray serial = QByteArray::fromBase64(user.value("Serial_bin").toString().toLatin1());
        if (!serial.isEmpty()) {
            m_serialCheck->setChecked(true);
            m_serialEdit->setText(QString::fromLatin1(serial.toHex().toUpper()));
        }
        break;
    }
    case 4: {
        const QString radius = user.value("RadiusUsername_utf").toString();
        if (!radius.isEmpty()) {
            m_radiusNameCheck->setChecked(true);
            m_radiusNameEdit->setText(radius);
        }
        break;
    }
    case 5: {
        const QString nt = user.value("NtUsername_utf").toString();
        if (!nt.isEmpty()) {
            m_radiusNameCheck->setChecked(true);
            m_radiusNameEdit->setText(nt);
        }
        break;
    }
    default:
        break;
    }
    updateState();
}

QJsonObject UserEditDialog::toRpcParams() const
{
    QJsonObject params;
    params["Name_str"] = m_nameEdit->text().trimmed();
    params["GroupName_str"] = m_groupEdit->text().trimmed();
    params["Realname_utf"] = m_realnameEdit->text();
    params["Note_utf"] = m_noteEdit->text();
    if (m_expireCheck->isChecked()) {
        params["ExpireTime_dt"] =
            QDateTime(m_expireDate->date(), m_expireTime->time()).toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzz"));
    }

    const int type = authType();
    params["AuthType_u32"] = type;
    switch (type) {
    case 1:
        if (!m_isNew && m_passwordEdit->text() == kHiddenPassword && m_passwordConfirmEdit->text() == kHiddenPassword) {
            params["Auth_Password_str"] = m_originalPassword;
        } else {
            params["Auth_Password_str"] = m_passwordEdit->text();
        }
        break;
    case 2:
        params["UserX_bin"] = QString::fromLatin1(m_certDer.toBase64());
        break;
    case 3:
        if (m_cnCheck->isChecked()) {
            params["CommonName_utf"] = m_cnEdit->text();
        }
        if (m_serialCheck->isChecked()) {
            QString serial = m_serialEdit->text().remove(QLatin1Char(' '));
            if (serial.size() % 2 != 0) {
                serial.prepend(QLatin1Char('0'));
            }
            params["Serial_bin"] = QString::fromLatin1(QByteArray::fromHex(serial.toLatin1()).toBase64());
        }
        break;
    case 4:
        if (m_radiusNameCheck->isChecked()) {
            params["RadiusUsername_utf"] = m_radiusNameEdit->text();
        }
        break;
    case 5:
        if (m_radiusNameCheck->isChecked()) {
            params["NtUsername_utf"] = m_radiusNameEdit->text();
        }
        break;
    default:
        break;
    }

    params["UsePolicy_bool"] = m_policyCheck->isChecked();
    if (m_policyCheck->isChecked()) {
        for (auto it = m_policy.begin(); it != m_policy.end(); ++it) {
            params[it.key()] = it.value();
        }
    }
    return params;
}

void UserEditDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("入力エラー"), tr("ユーザー名を入力してください。"));
        return;
    }
    QDialog::accept();
}

void UserEditDialog::onSelectGroup()
{
    // 公式Managerと同様、グループの管理画面を選択モードで開く。
    GroupListDialog dialog(m_rpc, m_hubName, this, /*selectMode=*/true);
    if (dialog.exec() == QDialog::Accepted) {
        m_groupEdit->setText(dialog.selectedGroup());
    }
}

void UserEditDialog::onPolicy()
{
    const QString name = m_nameEdit->text().trimmed();
    // SM_EDIT_USER_POL_DLG
    const QString title = tr("ユーザー %1 のセキュリティポリシー").arg(name);
    PolicyDialog dialog(title, title, m_policy, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_policy = dialog.policy();
    }
}

void UserEditDialog::onLoadCert()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("証明書の指定"), QString(), tr("X.509 証明書ファイル (*.cer *.crt *.pem *.der);;すべてのファイル (*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("エラー"), tr("ファイルを開けませんでした: %1").arg(file.errorString()));
        return;
    }
    const QByteArray data = file.readAll();
    QSslCertificate cert(data, QSsl::Pem);
    if (cert.isNull()) {
        cert = QSslCertificate(data, QSsl::Der);
    }
    if (cert.isNull()) {
        QMessageBox::warning(this, tr("エラー"), tr("X.509 証明書として読み込めませんでした。"));
        return;
    }
    m_certDer = cert.toDer();
    updateState();
}

void UserEditDialog::onViewCert()
{
    if (m_certDer.isEmpty()) {
        return;
    }
    CertInfoDialog dialog(m_certDer, this);
    dialog.exec();
}
