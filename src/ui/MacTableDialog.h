#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;

// 公式Manager「MAC アドレステーブル」(D_SM_MAC) 相当。
class MacTableDialog : public QDialog
{
    Q_OBJECT

public:
    // filterSessionNameを指定すると、そのセッションのエントリのみ表示する
    // (D_SM_SESSIONの「このセッションのMACテーブル」相当)。
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    MacTableDialog(VpnServerRpc *rpc, QString hubName, QString filterSessionName = QString(),
                   QWidget *parent = nullptr);

private slots:
    void onDelete();

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QString m_filterSessionName;
    QTableWidget *m_table;
};
