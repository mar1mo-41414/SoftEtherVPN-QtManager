#pragma once

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLineEdit;

// 公式Manager「プロキシサーバーの接続設定」(接続設定編集画面から開くサブダイアログ) 相当。
class ProxySettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProxySettingsDialog(QWidget *parent = nullptr);

    void setValues(const QString &host, quint16 port, const QString &user, const QString &password);
    QString host() const;
    quint16 port() const;
    QString user() const;
    QString password() const;

private:
    void updateOkEnabled();

    QLineEdit *m_hostEdit;
    QComboBox *m_portCombo;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
    QDialogButtonBox *m_buttonBox;
};
