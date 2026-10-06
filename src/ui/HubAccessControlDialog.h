#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonArray>

class QPushButton;
class QTableWidget;

// 公式Manager「接続元 IP 制限リスト」(D_SM_AC_LIST) 相当。
// 公式と同じく、ルールの追加・編集・削除はメモリ上で行い [保存] で SetAcList に反映する。
class HubAccessControlDialog : public QDialog
{
    Q_OBJECT

public:
    HubAccessControlDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSave();
    void updateButtons();

private:
    void load();
    void refreshTable();
    bool editRule(QJsonObject *rule);
    int selectedRow() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QList<QJsonObject> m_rules;

    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
};
