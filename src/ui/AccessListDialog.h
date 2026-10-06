#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonObject>
#include <QList>

class QPushButton;
class QTableWidget;

// 公式Manager「アクセスリスト」(D_SM_ACCESS_LIST) 相当。
// 公式と同じく、一覧への追加・編集・削除などはまずメモリ上で行い、[保存] でまとめて
// SetAccessList に反映する ([キャンセル] なら何も変更しない)。
class AccessListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    AccessListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr);

private slots:
    void onAddIPv4();
    void onAddIPv6();
    void onEdit();
    void onDelete();
    void onClone();
    void onSetActive(bool active);
    void onSave();
    void updateButtons();

private:
    void load();
    void refreshTable();
    void addItem(bool ipv6, const QJsonObject &base);
    int selectedRow() const;
    int nextPriority() const;
    quint32 nextId() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    QList<QJsonObject> m_items;

    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_cloneButton;
    QPushButton *m_enableButton;
    QPushButton *m_disableButton;
};
