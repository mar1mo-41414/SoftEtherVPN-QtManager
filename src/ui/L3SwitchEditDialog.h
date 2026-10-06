#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「仮想レイヤ 3 スイッチ "%S" の編集」(D_SM_L3_SW) 相当。
class L3SwitchEditDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    L3SwitchEditDialog(VpnServerRpc *rpc, QString switchName, QWidget *parent = nullptr);

private slots:
    void onAddInterface();
    void onDeleteInterface();
    void onAddTable();
    void onDeleteTable();
    void onStart();
    void onStop();

private:
    void reload();

    VpnServerRpc *m_rpc;
    QString m_switchName;
    QTableWidget *m_ifTable;
    QTableWidget *m_routeTable;
    QPushButton *m_deleteIfButton;
    QPushButton *m_deleteRouteButton;
};
