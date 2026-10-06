#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「信頼する証明機関の証明書の管理」(D_SM_CA) 相当。
class HubCaDialog : public QDialog
{
    Q_OBJECT

public:
    HubCaDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onDelete();
    void onView();
    void updateButtons();

private:
    void reload();
    int selectedRow() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
    QPushButton *m_deleteButton;
    QPushButton *m_viewButton;
};
