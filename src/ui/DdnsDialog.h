#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

// 公式Manager「ダイナミック DNS 機能」(D_SM_DDNS) 相当。サーバー全体の設定。
// DNS 鍵は専用APIが無いため、公式Manager同様 GetConfig で取得した設定ファイルから読み取る。
// 「無効にする」は対応するAPIが無いため、公式Manager同様に設定ファイルの編集方法を案内する。
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
    void onKeyHint();
    void onDisableHint();

private:
    void reload(bool silent = false);
    void loadKey();
    void loadCaps();

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
    QLabel *m_keyLabel;
    QLabel *m_changeCaption;
    QLabel *m_hostHint;
    QPushButton *m_hintButton;
    QPushButton *m_changeButton;
    QPushButton *m_restoreButton;
    QPushButton *m_proxyButton;
    bool m_hostnameSet = false;
};
