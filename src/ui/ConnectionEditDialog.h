#pragma once

#include "model/ConnectionProfile.h"

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QRadioButton;

// 公式Manager「新しい接続の設定」(D_SM_EDIT_SETTING) 相当の編集ダイアログ。
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
    void onHubAdminToggled(bool checked);
    void accept() override;

private:
    QStringList m_existingNames;
    QString m_originalName;

    QLineEdit *m_nameEdit;
    QLineEdit *m_hostEdit;
    QCheckBox *m_localhostCheck;
    QSpinBox *m_portSpin;
    QRadioButton *m_serverAdminRadio;
    QRadioButton *m_hubAdminRadio;
    QLineEdit *m_hubNameEdit;
    QLineEdit *m_passwordEdit;
    QCheckBox *m_noSaveCheck;
};
