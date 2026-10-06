#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「無効な証明書の一覧」(D_SM_CRL / D_SM_EDIT_CRL) 相当。
class HubCrlDialog : public QDialog
{
    Q_OBJECT

public:
    HubCrlDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void updateButtons();

private:
    void reload();
    int selectedRow() const;
    // 編集ダイアログを開く。true なら params に確定内容 (Key_u32/HubName_str を除く) を入れる。
    bool editEntry(QJsonObject *entry);

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
};
