#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「グループの管理」(D_SM_GROUP) 相当。
class GroupListDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    // selectMode: 他の画面から「グループの参照」で開く選択用の表示 (編集→選択、閉じる→なし)。
    // accept された場合 selectedGroup() が選択結果 (空文字列は「なし」)。
    GroupListDialog(VpnServerRpc *rpc, QString hubName, QWidget *parent = nullptr, bool selectMode = false);

    QString selectedGroup() const { return m_pickedGroup; }

private slots:
    void onCreate();
    void onEdit();
    void onDelete();
    void onMembers();
    void onPick();
    void onPickNone();
    void updateButtons();

private:
    void reload();
    QString selectedGroupName() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    bool m_selectMode;
    QString m_pickedGroup;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_memberButton;
};
