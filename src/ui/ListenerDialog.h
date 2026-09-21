#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QTableWidget;
class QPushButton;

// 公式Manager D_SM_SERVER 内の「リスナーの管理」セクション相当。
// 独立したダイアログとして実装している (公式はメイン画面に埋め込み)。
class ListenerDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    explicit ListenerDialog(VpnServerRpc *rpc, QWidget *parent = nullptr);

private slots:
    void onCreate();
    void onDelete();
    void onStart();
    void onStop();
    void onSelectionChanged();

private:
    void reload();
    int selectedPort(bool *ok) const;

    VpnServerRpc *m_rpc;
    QTableWidget *m_table;
    QPushButton *m_deleteButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
};
