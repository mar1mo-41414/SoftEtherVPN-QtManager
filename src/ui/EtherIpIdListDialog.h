#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「EtherIP / L2TPv3 サーバー機能の詳細設定」(D_SM_ETHERIP) 相当。
class EtherIpIdListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit EtherIpIdListDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSelectionChanged();

private:
    void reload();
    QString selectedId() const;

    VpnServerRpc *m_rpc;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
};
