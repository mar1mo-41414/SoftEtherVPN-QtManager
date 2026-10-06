#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QLabel;
class QRadioButton;

// 公式Manager「VPN Azure サービスの設定」(D_SM_AZURE) 相当。サーバー全体の設定。
class AzureDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit AzureDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onChangeHostName();
    void onOpenWeb();
    void onSetStatus();

private:
    void reload();

    VpnServerRpc *m_rpc;
    bool m_settingStatus = false;
    QRadioButton *m_enableRadio;
    QRadioButton *m_disableRadio;
    QLabel *m_statusLabel;
    QLabel *m_hostNameLabel;
};
