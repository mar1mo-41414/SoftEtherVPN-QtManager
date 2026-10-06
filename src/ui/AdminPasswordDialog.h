#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLineEdit;

// 公式Manager「管理者パスワードの設定」(D_SM_CHANGE_PASSWORD) 相当。
// JSON-RPCでは毎回パスワードをヘッダーで送るため、変更成功後は接続中のrpcの認証情報も更新する。
// 保存済みの接続設定のパスワードは自動では更新されない。
class AdminPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit AdminPasswordDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmEdit;
};
