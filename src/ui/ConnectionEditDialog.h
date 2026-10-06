#pragma once

#include "model/ConnectionProfile.h"

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QRadioButton;

// 公式Manager「接続設定の編集/新しい接続設定の作成」(D_SM_EDIT_SETTING) 相当。
// 左列に接続先とプロキシ、右列に管理モードとパスワードを並べる公式と同じ2カラム構成。
// このダイアログ自体は接続テストを行わず、ConnectionProfileの入力・検証のみを行う。
class ConnectionEditDialog : public QDialog
{
    Q_OBJECT

public:
    // existingNames: 重複チェック用の既存設定名一覧 (編集時は自分自身の元の名前を除いて渡す)。
    explicit ConnectionEditDialog(QStringList existingNames, QWidget *parent = nullptr);

    void setProfile(const ConnectionProfile &profile);
    ConnectionProfile profile() const;

private slots:
    void onLocalhostToggled(bool checked);
    void onModeChanged();
    void onProxyTypeChanged();
    void onProxyConfig();
    void updateOkEnabled();
    void accept() override;

private:
    int portValue() const;

    QStringList m_existingNames;

    // プロキシの詳細は「プロキシサーバーの接続設定」サブダイアログで編集し、ここに保持する。
    QString m_proxyHost;
    quint16 m_proxyPort = 8080;
    QString m_proxyUser;
    QString m_proxyPassword;

    QLineEdit *m_nameEdit;
    QLineEdit *m_hostEdit;
    QCheckBox *m_localhostCheck;
    QComboBox *m_portCombo;
    QRadioButton *m_directRadio;
    QRadioButton *m_httpRadio;
    QRadioButton *m_socksRadio;
    QPushButton *m_proxyConfigButton;
    QRadioButton *m_serverAdminRadio;
    QRadioButton *m_hubAdminRadio;
    QComboBox *m_hubNameCombo;
    QLineEdit *m_passwordEdit;
    QCheckBox *m_noSaveCheck;
    QDialogButtonBox *m_buttonBox;
};
