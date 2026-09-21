#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QTextEdit;

// 公式Manager「ユーザーの新規作成/編集」(D_SM_EDIT_USER) 相当。
// このフェーズではパスワード認証のみサポートする。証明書認証・RADIUS/NT認証・
// 有効期限・セキュリティポリシーは後続フェーズで追加する。
class UserEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserEditDialog(bool isNew, QWidget *parent = nullptr);

    void setValues(const QString &name, const QString &groupName, const QString &realname, const QString &note);

    // 新規作成時は必ずAuthType_u32=1(パスワード認証)を含める。
    // 編集時、パスワード欄が空ならAuth_Password_strは空文字列のまま送る
    // (SetHubのパスワードと同様、「変更しない」を意味する)。
    QJsonObject toRpcParams() const;

private slots:
    void accept() override;

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_groupNameEdit;
    QLineEdit *m_realnameEdit;
    QTextEdit *m_noteEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_passwordConfirmEdit;
};
