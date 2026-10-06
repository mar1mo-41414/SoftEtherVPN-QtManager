#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

// 公式Manager「仮想HUBの新規作成 / プロパティ」(D_SM_EDIT_HUB) 相当。
// プロパティ表示のときだけ、管理オプション・接続元IP制限リスト・拡張オプション・
// メッセージの設定を開くことができる。
class HubEditDialog : public QDialog
{
    Q_OBJECT

public:
    // rpc は管理オプションなどのサブダイアログで使う (借用)。isNew なら新規作成。
    HubEditDialog(VpnServerRpc *rpc, bool isNew, QWidget *parent = nullptr);

    // GetHub の結果をフォームに反映する (プロパティ表示時)。
    void setHub(const QJsonObject &hub);

    // CreateHub/SetHub にそのまま渡せるパラメータを返す。
    // 既存編集でパスワード欄に触れていなければ AdminPasswordPlainText_str は省略する
    // (サーバー側で「変更しない」を意味する)。
    QJsonObject toRpcParams() const;
    QString hubName() const;

private slots:
    void accept() override;
    void updateState();
    void onAdminOptions();
    void onExtOptions();
    void onAccessControl();
    void onMessage();

private:
    VpnServerRpc *m_rpc;
    bool m_isNew;
    int m_hubType = 0;

    QLineEdit *m_nameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_passwordConfirmEdit;
    QCheckBox *m_noEnumCheck;
    QCheckBox *m_limitMaxSessionCheck;
    QSpinBox *m_maxSessionSpin;
    QRadioButton *m_onlineRadio;
    QRadioButton *m_offlineRadio;
    QRadioButton *m_staticRadio;
    QRadioButton *m_dynamicRadio;
    QPushButton *m_okButton;
};
