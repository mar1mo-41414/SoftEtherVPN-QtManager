#pragma once

#include "rpc/VpnServerRpc.h"

#include <QByteArray>
#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QDateEdit;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTimeEdit;

// 公式Manager「ユーザーの新規作成/編集」(D_SM_EDIT_USER) 相当。
// 認証方法 (匿名/パスワード/固有証明書/署名済み証明書/RADIUS/NTドメイン)・有効期限・
// グループ・セキュリティポリシーを扱う。rpc は「グループの参照」でのみ使う (借用)。
class UserEditDialog : public QDialog
{
    Q_OBJECT

public:
    UserEditDialog(VpnServerRpc *rpc, const QString &hubName, bool isNew, QWidget *parent = nullptr);

    // GetUser の結果をフォームに反映する (編集時)。
    void setUser(const QJsonObject &user);

    // CreateUser / SetUser にそのまま渡せる JSON (HubName_str は含まない)。
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;
    void updateState();
    void onSelectGroup();
    void onPolicy();
    void onLoadCert();
    void onViewCert();

private:
    int authType() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    bool m_isNew;

    QLineEdit *m_nameEdit;
    QLineEdit *m_realnameEdit;
    QLineEdit *m_noteEdit;
    QLineEdit *m_groupEdit;
    QCheckBox *m_expireCheck;
    QDateEdit *m_expireDate;
    QTimeEdit *m_expireTime;
    QListWidget *m_authList;

    QCheckBox *m_policyCheck;
    QPushButton *m_policyButton;
    QJsonObject m_policy;

    QGroupBox *m_passwordGroup;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_passwordConfirmEdit;
    QString m_originalPassword;

    QGroupBox *m_userCertGroup;
    QPushButton *m_viewCertButton;
    QByteArray m_certDer;

    QGroupBox *m_rootCertGroup;
    QCheckBox *m_cnCheck;
    QLineEdit *m_cnEdit;
    QCheckBox *m_serialCheck;
    QLineEdit *m_serialEdit;

    QGroupBox *m_radiusGroup;
    QCheckBox *m_radiusNameCheck;
    QLineEdit *m_radiusNameEdit;

    QPushButton *m_okButton;
};
