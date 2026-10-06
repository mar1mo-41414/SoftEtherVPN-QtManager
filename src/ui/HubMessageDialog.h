#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QCheckBox;
class QPlainTextEdit;

// 公式Manager「メッセージの設定」(D_SM_MSG) 相当。VPN Client 接続時に表示するメッセージ。
class HubMessageDialog : public QDialog
{
    Q_OBJECT

public:
    HubMessageDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onOk();

private:
    VpnServerRpc *m_rpc;
    QString m_hubName;
    QCheckBox *m_useCheck;
    QPlainTextEdit *m_messageEdit;
};
