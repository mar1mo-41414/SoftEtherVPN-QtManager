#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonArray>

class QPushButton;
class QTableWidget;
class QTextEdit;

// 公式Manager「仮想 HUB 管理オプション」(D_SM_ADMIN_OPTION) / 「仮想 HUB 拡張オプション」相当。
// どちらも「値の名前 / 設定値」の一覧 + 説明 + [保存] で構成される。
class HubOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Kind { Admin, Extended };

    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    HubOptionsDialog(VpnServerRpc *rpc, QString hubName, Kind kind, QWidget *parent = nullptr);

private slots:
    void onEdit();
    void onSave();
    void onSelectionChanged();

private:
    void load();
    int selectedRow() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    Kind m_kind;
    QJsonArray m_items;

    QTableWidget *m_table;
    QTextEdit *m_descriptionEdit;
    QPushButton *m_editButton;
};
