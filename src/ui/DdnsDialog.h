#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLabel;
class QLineEdit;

// 公式Manager「ダイナミック DNS 機能」(D_SM_DDNS) 相当。サーバー全体の設定。
// DNS 鍵の表示・ダイナミック DNS 機能の無効化は対応するAPIが無いため未対応。
class DdnsDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit DdnsDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onChange();
    void onRestore();
    void onProxy();
    void onHint();

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_currentHostName;
    QString m_suffix;
    QString m_ipv4;
    QString m_ipv6;
    QLineEdit *m_hostNameEdit;
    QLabel *m_suffixLabel;
    QLabel *m_fqdnLabel;
    QLabel *m_ipv4Label;
    QLabel *m_ipv6Label;
};
