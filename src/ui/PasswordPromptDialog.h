#pragma once

#include <QDialog>

class QLineEdit;

// 公式Manager「%S へのログイン」(D_PASSWORD) 相当の、保存されていないパスワードを
// 接続時にその場で尋ねるための簡易ダイアログ。
class PasswordPromptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordPromptDialog(const QString &targetLabel, QWidget *parent = nullptr);

    QString password() const;

private:
    QLineEdit *m_passwordEdit;
};
