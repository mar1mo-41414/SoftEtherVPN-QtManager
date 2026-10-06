#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「仮想レイヤ 3 スイッチ設定」(D_SM_L3) 相当。サーバー全体の設定。
class L3SwitchListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit L3SwitchListDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onStart();
    void onStop();
    void onSelectionChanged();

private:
    void reload();
    QString selectedName() const;

    VpnServerRpc *m_rpc;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
};
